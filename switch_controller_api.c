/*
 * Nintendo Switch Controller Emulator with Socket API
 * Allows external programs to control the virtual controller via Unix socket
 *
 * Compile: gcc -o switch_controller_api switch_controller_api.c -lpthread
 * Run: sudo ./switch_controller_api
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include "switch_descriptors.h"

#define CONFIGFS_PATH "/sys/kernel/config/usb_gadget"
#define GADGET_NAME "switch_controller"
#define SOCKET_PATH "/tmp/switch_controller.sock"

/* Global state */
static int hidg_fd = -1;
static volatile int running = 1;
static switch_input_report_t current_state = {0};
static pthread_mutex_t state_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Function prototypes */
static int setup_gadget(void);
static void cleanup_gadget(void);
static int open_hidg_device(void);
static int send_report(const switch_input_report_t *report);
static void *report_sender_thread(void *arg);
static void *socket_server_thread(void *arg);

/* Write HID report descriptor */
static int write_report_desc(const char *path) {
    int fd = open(path, O_WRONLY);
    if (fd < 0) {
        perror("Failed to open report_desc");
        return -1;
    }

    if (write(fd, switch_hid_report_desc, sizeof(switch_hid_report_desc)) < 0) {
        perror("Failed to write report descriptor");
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}

/* Write string to file */
static int write_file(const char *path, const char *value) {
    int fd = open(path, O_WRONLY);
    if (fd < 0) {
        fprintf(stderr, "Failed to open %s: %s\n", path, strerror(errno));
        return -1;
    }

    int len = strlen(value);
    if (write(fd, value, len) != len) {
        fprintf(stderr, "Failed to write to %s: %s\n", path, strerror(errno));
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}

/* Get UDC name */
static char *get_udc_name(void) {
    static char udc_name[256];
    FILE *fp = popen("ls /sys/class/udc", "r");
    if (!fp) {
        perror("Failed to list UDC");
        return NULL;
    }

    if (fgets(udc_name, sizeof(udc_name), fp) == NULL) {
        fprintf(stderr, "No UDC found\n");
        pclose(fp);
        return NULL;
    }

    udc_name[strcspn(udc_name, "\n")] = 0;
    pclose(fp);
    return udc_name;
}

/* Setup USB gadget */
static int setup_gadget(void) {
    char path[512];
    char value[256];

    printf("Setting up USB gadget...\n");

    snprintf(path, sizeof(path), "%s/%s", CONFIGFS_PATH, GADGET_NAME);
    if (mkdir(path, 0755) < 0 && errno != EEXIST) {
        perror("Failed to create gadget directory");
        return -1;
    }

    snprintf(path, sizeof(path), "%s/%s/idVendor", CONFIGFS_PATH, GADGET_NAME);
    snprintf(value, sizeof(value), "0x%04X", USB_VID);
    if (write_file(path, value) < 0) return -1;

    snprintf(path, sizeof(path), "%s/%s/idProduct", CONFIGFS_PATH, GADGET_NAME);
    snprintf(value, sizeof(value), "0x%04X", USB_PID);
    if (write_file(path, value) < 0) return -1;

    snprintf(path, sizeof(path), "%s/%s/bcdUSB", CONFIGFS_PATH, GADGET_NAME);
    if (write_file(path, "0x0200") < 0) return -1;

    snprintf(path, sizeof(path), "%s/%s/bcdDevice", CONFIGFS_PATH, GADGET_NAME);
    if (write_file(path, "0x0100") < 0) return -1;

    snprintf(path, sizeof(path), "%s/%s/strings/0x409", CONFIGFS_PATH, GADGET_NAME);
    if (mkdir(path, 0755) < 0 && errno != EEXIST) {
        perror("Failed to create strings directory");
        return -1;
    }

    snprintf(path, sizeof(path), "%s/%s/strings/0x409/manufacturer", CONFIGFS_PATH, GADGET_NAME);
    if (write_file(path, "HORI CO.,LTD.") < 0) return -1;

    snprintf(path, sizeof(path), "%s/%s/strings/0x409/product", CONFIGFS_PATH, GADGET_NAME);
    if (write_file(path, "POKKEN CONTROLLER") < 0) return -1;

    snprintf(path, sizeof(path), "%s/%s/strings/0x409/serialnumber", CONFIGFS_PATH, GADGET_NAME);
    if (write_file(path, "000000000001") < 0) return -1;

    snprintf(path, sizeof(path), "%s/%s/configs/c.1", CONFIGFS_PATH, GADGET_NAME);
    if (mkdir(path, 0755) < 0 && errno != EEXIST) {
        perror("Failed to create config directory");
        return -1;
    }

    snprintf(path, sizeof(path), "%s/%s/configs/c.1/strings/0x409", CONFIGFS_PATH, GADGET_NAME);
    if (mkdir(path, 0755) < 0 && errno != EEXIST) {
        perror("Failed to create config strings directory");
        return -1;
    }

    snprintf(path, sizeof(path), "%s/%s/configs/c.1/strings/0x409/configuration", CONFIGFS_PATH, GADGET_NAME);
    if (write_file(path, "Controller") < 0) return -1;

    snprintf(path, sizeof(path), "%s/%s/configs/c.1/MaxPower", CONFIGFS_PATH, GADGET_NAME);
    if (write_file(path, "500") < 0) return -1;

    snprintf(path, sizeof(path), "%s/%s/functions/hid.usb0", CONFIGFS_PATH, GADGET_NAME);
    if (mkdir(path, 0755) < 0 && errno != EEXIST) {
        perror("Failed to create HID function directory");
        return -1;
    }

    snprintf(path, sizeof(path), "%s/%s/functions/hid.usb0/protocol", CONFIGFS_PATH, GADGET_NAME);
    if (write_file(path, "0") < 0) return -1;

    snprintf(path, sizeof(path), "%s/%s/functions/hid.usb0/subclass", CONFIGFS_PATH, GADGET_NAME);
    if (write_file(path, "0") < 0) return -1;

    snprintf(path, sizeof(path), "%s/%s/functions/hid.usb0/report_length", CONFIGFS_PATH, GADGET_NAME);
    snprintf(value, sizeof(value), "%d", SWITCH_INPUT_REPORT_SIZE);
    if (write_file(path, value) < 0) return -1;

    snprintf(path, sizeof(path), "%s/%s/functions/hid.usb0/report_desc", CONFIGFS_PATH, GADGET_NAME);
    if (write_report_desc(path) < 0) return -1;

    char link_src[512], link_dst[512];
    snprintf(link_src, sizeof(link_src), "%s/%s/functions/hid.usb0", CONFIGFS_PATH, GADGET_NAME);
    snprintf(link_dst, sizeof(link_dst), "%s/%s/configs/c.1/hid.usb0", CONFIGFS_PATH, GADGET_NAME);
    if (symlink(link_src, link_dst) < 0 && errno != EEXIST) {
        perror("Failed to link function to config");
        return -1;
    }

    char *udc = get_udc_name();
    if (!udc) return -1;

    /* First, unbind from UDC if already bound (prevents "Device or resource busy") */
    snprintf(path, sizeof(path), "%s/%s/UDC", CONFIGFS_PATH, GADGET_NAME);
    write_file(path, "");  /* Ignore errors - might not be bound yet */
    usleep(100000);  /* Wait 100ms for unbind to complete */

    printf("Binding to UDC: %s\n", udc);
    if (write_file(path, udc) < 0) return -1;

    printf("USB gadget setup complete!\n");
    return 0;
}

/* Cleanup gadget */
static void cleanup_gadget(void) {
    char path[512];

    printf("Cleaning up USB gadget...\n");

    snprintf(path, sizeof(path), "%s/%s/UDC", CONFIGFS_PATH, GADGET_NAME);
    write_file(path, "");

    snprintf(path, sizeof(path), "%s/%s/configs/c.1/hid.usb0", CONFIGFS_PATH, GADGET_NAME);
    unlink(path);

    snprintf(path, sizeof(path), "%s/%s/configs/c.1/strings/0x409", CONFIGFS_PATH, GADGET_NAME);
    rmdir(path);

    snprintf(path, sizeof(path), "%s/%s/configs/c.1", CONFIGFS_PATH, GADGET_NAME);
    rmdir(path);

    snprintf(path, sizeof(path), "%s/%s/functions/hid.usb0", CONFIGFS_PATH, GADGET_NAME);
    rmdir(path);

    snprintf(path, sizeof(path), "%s/%s/strings/0x409", CONFIGFS_PATH, GADGET_NAME);
    rmdir(path);

    snprintf(path, sizeof(path), "%s/%s", CONFIGFS_PATH, GADGET_NAME);
    rmdir(path);

    printf("Cleanup complete\n");
}

/* Open HID device */
static int open_hidg_device(void) {
    char path[256];
    int fd;

    for (int i = 0; i < 10; i++) {
        snprintf(path, sizeof(path), "/dev/hidg%d", i);
        fd = open(path, O_RDWR);
        if (fd >= 0) {
            printf("Opened HID device: %s\n", path);
            return fd;
        }
    }

    fprintf(stderr, "Failed to open any HID device\n");
    return -1;
}

/* Send controller report */
static int send_report(const switch_input_report_t *report) {
    if (hidg_fd < 0) return -1;

    ssize_t written = write(hidg_fd, report, SWITCH_INPUT_REPORT_SIZE);
    if (written != SWITCH_INPUT_REPORT_SIZE) {
        return -1;
    }

    return 0;
}

/* Thread to continuously send reports at 20Hz */
static void *report_sender_thread(void *arg) {
    (void)arg;
    switch_input_report_t local_state;

    while (running) {
        pthread_mutex_lock(&state_mutex);
        memcpy(&local_state, &current_state, sizeof(local_state));
        pthread_mutex_unlock(&state_mutex);

        send_report(&local_state);
        usleep(50000); /* 50ms = 20Hz */
    }

    return NULL;
}

/* Socket server thread */
static void *socket_server_thread(void *arg) {
    (void)arg;
    int sock_fd, client_fd;
    struct sockaddr_un addr;

    /* Remove old socket if exists */
    unlink(SOCKET_PATH);

    sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        perror("Failed to create socket");
        return NULL;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (bind(sock_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("Failed to bind socket");
        close(sock_fd);
        return NULL;
    }

    if (listen(sock_fd, 5) < 0) {
        perror("Failed to listen on socket");
        close(sock_fd);
        return NULL;
    }

    printf("Socket server listening on %s\n", SOCKET_PATH);
    printf("\nAPI Commands:\n");
    printf("  BTN <button_mask>  - Set button state (hex)\n");
    printf("  STICK <lx> <ly> <rx> <ry>  - Set stick positions (0-255)\n");
    printf("  HAT <pos>  - Set HAT switch (0-7, 8=neutral)\n");
    printf("  RESET  - Reset to neutral state\n\n");

    while (running) {
        client_fd = accept(sock_fd, NULL, NULL);
        if (client_fd < 0) continue;

        char buffer[256];
        ssize_t n = read(client_fd, buffer, sizeof(buffer) - 1);
        if (n > 0) {
            buffer[n] = '\0';

            /* Parse command */
            if (strncmp(buffer, "BTN ", 4) == 0) {
                unsigned int buttons;
                if (sscanf(buffer + 4, "%x", &buttons) == 1) {
                    pthread_mutex_lock(&state_mutex);
                    current_state.buttons = buttons;
                    pthread_mutex_unlock(&state_mutex);
                    write(client_fd, "OK\n", 3);
                }
            } else if (strncmp(buffer, "STICK ", 6) == 0) {
                int lx, ly, rx, ry;
                if (sscanf(buffer + 6, "%d %d %d %d", &lx, &ly, &rx, &ry) == 4) {
                    pthread_mutex_lock(&state_mutex);
                    current_state.lx = lx;
                    current_state.ly = ly;
                    current_state.rx = rx;
                    current_state.ry = ry;
                    pthread_mutex_unlock(&state_mutex);
                    write(client_fd, "OK\n", 3);
                }
            } else if (strncmp(buffer, "HAT ", 4) == 0) {
                int hat;
                if (sscanf(buffer + 4, "%d", &hat) == 1) {
                    pthread_mutex_lock(&state_mutex);
                    current_state.hat = hat;
                    pthread_mutex_unlock(&state_mutex);
                    write(client_fd, "OK\n", 3);
                }
            } else if (strncmp(buffer, "RESET", 5) == 0) {
                pthread_mutex_lock(&state_mutex);
                memset(&current_state, 0, sizeof(current_state));
                current_state.lx = 128;
                current_state.ly = 128;
                current_state.rx = 128;
                current_state.ry = 128;
                current_state.hat = HAT_NEUTRAL;
                pthread_mutex_unlock(&state_mutex);
                write(client_fd, "OK\n", 3);
            }
        }

        close(client_fd);
    }

    close(sock_fd);
    unlink(SOCKET_PATH);
    return NULL;
}

/* Signal handler */
static void signal_handler(int sig) {
    (void)sig;
    running = 0;
}

int main(int argc, char **argv) {
    pthread_t sender_thread, server_thread;

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    if (geteuid() != 0) {
        fprintf(stderr, "This program must be run as root\n");
        return 1;
    }

    if (setup_gadget() < 0) {
        fprintf(stderr, "Failed to setup USB gadget\n");
        cleanup_gadget();
        return 1;
    }

    sleep(1);

    hidg_fd = open_hidg_device();
    if (hidg_fd < 0) {
        cleanup_gadget();
        return 1;
    }

    /* Initialize controller state */
    memset(&current_state, 0, sizeof(current_state));
    current_state.lx = 128;
    current_state.ly = 128;
    current_state.rx = 128;
    current_state.ry = 128;
    current_state.hat = HAT_NEUTRAL;

    printf("\nSwitch controller is ready!\n");
    printf("Connect the Luckfox Pico Max to your Nintendo Switch via USB\n");

    /* Start threads */
    pthread_create(&sender_thread, NULL, report_sender_thread, NULL);
    pthread_create(&server_thread, NULL, socket_server_thread, NULL);

    /* Wait for threads */
    pthread_join(sender_thread, NULL);
    pthread_join(server_thread, NULL);

    /* Cleanup */
    if (hidg_fd >= 0) {
        close(hidg_fd);
    }
    cleanup_gadget();

    return 0;
}
