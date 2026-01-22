/*
 * Nintendo Switch Controller Emulator for Luckfox Pico Max
 * Uses Linux USB Gadget (ConfigFS) to emulate a HORI POKKEN Controller
 *
 * Compile: gcc -o switch_controller switch_controller.c -lpthread
 * Run: sudo ./switch_controller
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
#include "switch_descriptors.h"

#define CONFIGFS_PATH "/sys/kernel/config/usb_gadget"
#define GADGET_NAME "switch_controller"
#define UDC_PATH "/sys/class/udc"

/* Global state */
static int hidg_fd = -1;
static volatile int running = 1;
static switch_input_report_t current_state = {0};

/* Convert HID report descriptor to hex string for configfs */
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

/* Write a string to a file */
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

/* Read the first UDC name */
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

    /* Remove newline */
    udc_name[strcspn(udc_name, "\n")] = 0;
    pclose(fp);
    return udc_name;
}

/* Setup USB gadget via ConfigFS */
static int setup_gadget(void) {
    char path[512];
    char value[256];

    printf("Setting up USB gadget...\n");

    /* Create gadget directory */
    snprintf(path, sizeof(path), "%s/%s", CONFIGFS_PATH, GADGET_NAME);
    if (mkdir(path, 0755) < 0 && errno != EEXIST) {
        perror("Failed to create gadget directory");
        return -1;
    }

    /* Set VID and PID */
    snprintf(path, sizeof(path), "%s/%s/idVendor", CONFIGFS_PATH, GADGET_NAME);
    snprintf(value, sizeof(value), "0x%04X", USB_VID);
    if (write_file(path, value) < 0) return -1;

    snprintf(path, sizeof(path), "%s/%s/idProduct", CONFIGFS_PATH, GADGET_NAME);
    snprintf(value, sizeof(value), "0x%04X", USB_PID);
    if (write_file(path, value) < 0) return -1;

    /* Set USB version */
    snprintf(path, sizeof(path), "%s/%s/bcdUSB", CONFIGFS_PATH, GADGET_NAME);
    if (write_file(path, "0x0200") < 0) return -1;

    snprintf(path, sizeof(path), "%s/%s/bcdDevice", CONFIGFS_PATH, GADGET_NAME);
    if (write_file(path, "0x0100") < 0) return -1;

    /* Create English strings */
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

    /* Create configuration */
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

    /* Create HID function */
    snprintf(path, sizeof(path), "%s/%s/functions/hid.usb0", CONFIGFS_PATH, GADGET_NAME);
    if (mkdir(path, 0755) < 0 && errno != EEXIST) {
        perror("Failed to create HID function directory");
        return -1;
    }

    /* Configure HID parameters */
    snprintf(path, sizeof(path), "%s/%s/functions/hid.usb0/protocol", CONFIGFS_PATH, GADGET_NAME);
    if (write_file(path, "0") < 0) return -1;

    snprintf(path, sizeof(path), "%s/%s/functions/hid.usb0/subclass", CONFIGFS_PATH, GADGET_NAME);
    if (write_file(path, "0") < 0) return -1;

    snprintf(path, sizeof(path), "%s/%s/functions/hid.usb0/report_length", CONFIGFS_PATH, GADGET_NAME);
    snprintf(value, sizeof(value), "%d", SWITCH_INPUT_REPORT_SIZE);
    if (write_file(path, value) < 0) return -1;

    /* Write HID report descriptor */
    snprintf(path, sizeof(path), "%s/%s/functions/hid.usb0/report_desc", CONFIGFS_PATH, GADGET_NAME);
    if (write_report_desc(path) < 0) return -1;

    /* Link function to configuration */
    char link_src[512], link_dst[512];
    snprintf(link_src, sizeof(link_src), "%s/%s/functions/hid.usb0", CONFIGFS_PATH, GADGET_NAME);
    snprintf(link_dst, sizeof(link_dst), "%s/%s/configs/c.1/hid.usb0", CONFIGFS_PATH, GADGET_NAME);
    if (symlink(link_src, link_dst) < 0 && errno != EEXIST) {
        perror("Failed to link function to config");
        return -1;
    }

    /* Bind to UDC */
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

/* Cleanup USB gadget */
static void cleanup_gadget(void) {
    char path[512];

    printf("Cleaning up USB gadget...\n");

    /* Unbind from UDC */
    snprintf(path, sizeof(path), "%s/%s/UDC", CONFIGFS_PATH, GADGET_NAME);
    write_file(path, "");

    /* Remove symlink */
    snprintf(path, sizeof(path), "%s/%s/configs/c.1/hid.usb0", CONFIGFS_PATH, GADGET_NAME);
    unlink(path);

    /* Remove directories (in reverse order) */
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

    /* Try /dev/hidg0, hidg1, etc. */
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
        fprintf(stderr, "Failed to write report: %s\n", strerror(errno));
        return -1;
    }

    return 0;
}

/* Signal handler */
static void signal_handler(int sig) {
    (void)sig;
    running = 0;
}

/* Demo: Send button presses */
static void demo_controller(void) {
    printf("\nStarting controller demo...\n");
    printf("Press Ctrl+C to stop\n\n");

    /* Initialize controller state - centered sticks */
    memset(&current_state, 0, sizeof(current_state));
    current_state.lx = 128;
    current_state.ly = 128;
    current_state.rx = 128;
    current_state.ry = 128;
    current_state.hat = HAT_NEUTRAL;

    int counter = 0;
    while (running) {
        /* Send neutral state */
        send_report(&current_state);

        /* Every 2 seconds, press a button */
        if (counter % 40 == 0) {
            printf("Pressing A button...\n");
            current_state.buttons = BTN_A;
            send_report(&current_state);
            usleep(100000); /* Hold for 100ms */
            current_state.buttons = 0;
        }

        /* Every 3 seconds, move left stick */
        if (counter % 60 == 0 && counter > 0) {
            printf("Moving left stick...\n");
            current_state.lx = 255;  /* Right */
            send_report(&current_state);
            usleep(500000); /* Hold for 500ms */
            current_state.lx = 128;  /* Center */
        }

        usleep(50000); /* 50ms = 20Hz */
        counter++;
    }
}

int main(int argc, char **argv) {
    /* Setup signal handler */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    /* Check if running as root */
    if (geteuid() != 0) {
        fprintf(stderr, "This program must be run as root\n");
        return 1;
    }

    /* Setup USB gadget */
    if (setup_gadget() < 0) {
        fprintf(stderr, "Failed to setup USB gadget\n");
        cleanup_gadget();
        return 1;
    }

    /* Wait a bit for device to be created */
    sleep(1);

    /* Open HID device */
    hidg_fd = open_hidg_device();
    if (hidg_fd < 0) {
        cleanup_gadget();
        return 1;
    }

    printf("\nSwitch controller is ready!\n");
    printf("Connect the Luckfox Pico Max to your Nintendo Switch via USB\n");

    /* Run demo */
    demo_controller();

    /* Cleanup */
    if (hidg_fd >= 0) {
        close(hidg_fd);
    }
    cleanup_gadget();

    return 0;
}
