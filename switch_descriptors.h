/*
 * Nintendo Switch Pro Controller USB Descriptors
 * Based on HORI POKKEN Controller (VID: 0x0F0D, PID: 0x0092)
 *
 * This is a licensed third-party controller that the Switch accepts
 * Alternative: Official Pro Controller (VID: 0x057E, PID: 0x2009)
 */

#ifndef SWITCH_DESCRIPTORS_H
#define SWITCH_DESCRIPTORS_H

#include <stdint.h>

/* USB Device Descriptor */
#define USB_VID 0x0F0D  /* HORI CO.,LTD. */
#define USB_PID 0x0092  /* POKKEN CONTROLLER */

/* Alternative official Pro Controller IDs (may require more protocol implementation) */
#define USB_VID_NINTENDO 0x057E
#define USB_PID_PROCON   0x2009

/* HID Report Descriptor for Switch Controller
 * This descriptor defines:
 * - 16 buttons (2 bytes)
 * - HAT switch (4 bits) for D-pad
 * - 4 analog axes: Left X, Left Y, Right X, Right Y (4 bytes)
 * - Vendor specific data
 */
static const uint8_t switch_hid_report_desc[] = {
    0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
    0x09, 0x05,        // Usage (Game Pad)
    0xA1, 0x01,        // Collection (Application)

    /* Buttons (16 bits) */
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x01,        //   Logical Maximum (1)
    0x35, 0x00,        //   Physical Minimum (0)
    0x45, 0x01,        //   Physical Maximum (1)
    0x75, 0x01,        //   Report Size (1)
    0x95, 0x10,        //   Report Count (16)
    0x05, 0x09,        //   Usage Page (Button)
    0x19, 0x01,        //   Usage Minimum (0x01)
    0x29, 0x10,        //   Usage Maximum (0x10)
    0x81, 0x02,        //   Input (Data,Var,Abs)

    /* HAT Switch (4 bits) */
    0x05, 0x01,        //   Usage Page (Generic Desktop Ctrls)
    0x25, 0x07,        //   Logical Maximum (7)
    0x46, 0x3B, 0x01,  //   Physical Maximum (315)
    0x75, 0x04,        //   Report Size (4)
    0x95, 0x01,        //   Report Count (1)
    0x65, 0x14,        //   Unit (System: English Rotation, Length: Centimeter)
    0x09, 0x39,        //   Usage (Hat switch)
    0x81, 0x42,        //   Input (Data,Var,Abs,Null State)

    /* Vendor Defined (4 bits) - padding */
    0x65, 0x00,        //   Unit (None)
    0x95, 0x01,        //   Report Count (1)
    0x81, 0x01,        //   Input (Const,Array,Abs,No Wrap)

    /* Analog Axes - X, Y, Z, Rz (4 bytes) */
    0x26, 0xFF, 0x00,  //   Logical Maximum (255)
    0x46, 0xFF, 0x00,  //   Physical Maximum (255)
    0x09, 0x30,        //   Usage (X)
    0x09, 0x31,        //   Usage (Y)
    0x09, 0x32,        //   Usage (Z)
    0x09, 0x35,        //   Usage (Rz)
    0x75, 0x08,        //   Report Size (8)
    0x95, 0x04,        //   Report Count (4)
    0x81, 0x02,        //   Input (Data,Var,Abs)

    /* Vendor Specific (1 byte) */
    0x06, 0x00, 0xFF,  //   Usage Page (Vendor Defined 0xFF00)
    0x09, 0x20,        //   Usage (0x20)
    0x95, 0x01,        //   Report Count (1)
    0x81, 0x02,        //   Input (Data,Var,Abs)

    /* Output (8 bytes) - for rumble and other commands */
    0x0A, 0x21, 0x26,  //   Usage (0x2621)
    0x95, 0x08,        //   Report Count (8)
    0x91, 0x02,        //   Output (Data,Var,Abs)

    0xC0,              // End Collection
};

/* Button bit definitions */
#define BTN_Y       (1 << 0)
#define BTN_B       (1 << 1)
#define BTN_A       (1 << 2)
#define BTN_X       (1 << 3)
#define BTN_L       (1 << 4)
#define BTN_R       (1 << 5)
#define BTN_ZL      (1 << 6)
#define BTN_ZR      (1 << 7)
#define BTN_MINUS   (1 << 8)
#define BTN_PLUS    (1 << 9)
#define BTN_LCLICK  (1 << 10)  /* Left stick click */
#define BTN_RCLICK  (1 << 11)  /* Right stick click */
#define BTN_HOME    (1 << 12)
#define BTN_CAPTURE (1 << 13)

/* HAT switch positions (D-pad) */
#define HAT_UP          0
#define HAT_UP_RIGHT    1
#define HAT_RIGHT       2
#define HAT_DOWN_RIGHT  3
#define HAT_DOWN        4
#define HAT_DOWN_LEFT   5
#define HAT_LEFT        6
#define HAT_UP_LEFT     7
#define HAT_NEUTRAL     8

/* Controller input report structure (8 bytes) */
typedef struct __attribute__((packed)) {
    uint16_t buttons;      /* 16 buttons */
    uint8_t hat : 4;       /* HAT switch (D-pad) */
    uint8_t padding : 4;   /* Padding */
    uint8_t lx;            /* Left stick X axis */
    uint8_t ly;            /* Left stick Y axis */
    uint8_t rx;            /* Right stick X axis */
    uint8_t ry;            /* Right stick Y axis */
    uint8_t vendor;        /* Vendor specific */
} switch_input_report_t;

#define SWITCH_INPUT_REPORT_SIZE sizeof(switch_input_report_t)

#endif /* SWITCH_DESCRIPTORS_H */
