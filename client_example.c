/*
 * Example client for Switch Controller API
 * Demonstrates how to control the virtual controller
 *
 * Compile: gcc -o client_example client_example.c
 * Run: ./client_example
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCKET_PATH "/tmp/switch_controller.sock"

/* Send command to controller */
static int send_command(const char *cmd) {
    int sock;
    struct sockaddr_un addr;
    char response[256];

    sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        return -1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect");
        close(sock);
        return -1;
    }

    if (write(sock, cmd, strlen(cmd)) < 0) {
        perror("write");
        close(sock);
        return -1;
    }

    ssize_t n = read(sock, response, sizeof(response) - 1);
    if (n > 0) {
        response[n] = '\0';
        printf("Response: %s", response);
    }

    close(sock);
    return 0;
}

/* Button press helper */
static void press_button(const char *name, unsigned int mask) {
    char cmd[64];
    printf("Pressing %s button...\n", name);
    snprintf(cmd, sizeof(cmd), "BTN %x", mask);
    send_command(cmd);
    usleep(100000); /* Hold for 100ms */
    send_command("BTN 0"); /* Release */
    usleep(200000); /* Wait 200ms */
}

/* Move stick helper */
static void move_stick(const char *direction) {
    printf("Moving left stick %s...\n", direction);

    if (strcmp(direction, "up") == 0) {
        send_command("STICK 128 0 128 128");
    } else if (strcmp(direction, "down") == 0) {
        send_command("STICK 128 255 128 128");
    } else if (strcmp(direction, "left") == 0) {
        send_command("STICK 0 128 128 128");
    } else if (strcmp(direction, "right") == 0) {
        send_command("STICK 255 128 128 128");
    }

    usleep(500000); /* Hold for 500ms */
    send_command("STICK 128 128 128 128"); /* Center */
    usleep(200000);
}

/* D-pad helper */
static void press_dpad(const char *direction) {
    char cmd[64];
    int hat;

    printf("Pressing D-pad %s...\n", direction);

    if (strcmp(direction, "up") == 0) hat = 0;
    else if (strcmp(direction, "right") == 0) hat = 2;
    else if (strcmp(direction, "down") == 0) hat = 4;
    else if (strcmp(direction, "left") == 0) hat = 6;
    else return;

    snprintf(cmd, sizeof(cmd), "HAT %d", hat);
    send_command(cmd);
    usleep(100000);
    send_command("HAT 8"); /* Neutral */
    usleep(200000);
}

int main(int argc, char **argv) {
    printf("Switch Controller Client Example\n");
    printf("=================================\n\n");

    /* Reset to neutral state */
    printf("Resetting controller...\n");
    send_command("RESET");
    usleep(500000);

    /* Press A button (confirm) */
    press_button("A", 0x0004); /* BTN_A */

    /* Press B button (back) */
    press_button("B", 0x0002); /* BTN_B */

    /* Press X and Y together */
    printf("Pressing X+Y buttons...\n");
    send_command("BTN 0x0009"); /* BTN_Y | BTN_X */
    usleep(100000);
    send_command("BTN 0");
    usleep(200000);

    /* Move left stick in a circle */
    printf("Moving left stick in a circle...\n");
    move_stick("up");
    move_stick("right");
    move_stick("down");
    move_stick("left");

    /* Use D-pad */
    press_dpad("up");
    press_dpad("down");
    press_dpad("left");
    press_dpad("right");

    /* Press + button (menu) */
    press_button("Plus", 0x0200); /* BTN_PLUS */

    /* Press Home button */
    press_button("Home", 0x1000); /* BTN_HOME */

    printf("\nDemo complete!\n");
    return 0;
}
