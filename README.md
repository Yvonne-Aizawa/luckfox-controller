# Nintendo Switch Controller Emulator for Luckfox Pico Max

This project implements a Nintendo Switch Pro Controller emulator using Linux USB Gadget (ConfigFS) on the Luckfox Pico Max. It allows the Luckfox device to appear as a HORI POKKEN Controller when connected to a Nintendo Switch via USB.

## Features

- **Full USB HID Controller Emulation** - Emulates a licensed HORI POKKEN Controller (VID: 0x0F0D, PID: 0x0092)
- **All Controller Inputs Supported**:
  - 16 buttons (A, B, X, Y, L, R, ZL, ZR, +, -, Home, Capture, etc.)
  - D-pad (HAT switch)
  - Left and right analog sticks
- **Two Operation Modes**:
  - **Demo Mode**: Simple standalone demo that presses buttons automatically
  - **API Mode**: Unix socket API for external control
- **Real-time Control**: Sends input reports at 20Hz for responsive gameplay

## Hardware Requirements

- **Luckfox Pico Max** (or similar device with USB OTG support)
- USB cable for connecting to Nintendo Switch
- Linux kernel with USB Gadget support (ConfigFS)

## Software Requirements

- Linux kernel with:
  - `CONFIG_USB_CONFIGFS`
  - `CONFIG_USB_CONFIGFS_F_HID`
  - `CONFIG_USB_GADGET`
- GCC compiler
- pthread library

## Building

### Option 1: Cross-Compile on Your PC (Recommended)

The Luckfox Pico Max uses an ARM processor, so cross-compilation on your PC is much faster than compiling on the device.

**Using Luckfox SDK toolchain:**

```bash
# Set the path to your Luckfox SDK toolchain
export CROSS_COMPILE=/path/to/luckfox-pico/tools/linux/toolchain/arm-rockchip830-linux-uclibcgnueabihf/bin/arm-rockchip830-linux-uclibcgnueabihf-

# Build all programs
make CROSS_COMPILE=$CROSS_COMPILE

# Or build static binaries (more portable, no library dependencies)
make CROSS_COMPILE=$CROSS_COMPILE STATIC=1
```

**Using standard ARM toolchain (Ubuntu/Debian):**

```bash
# Install cross-compiler
sudo apt-get install gcc-arm-linux-gnueabihf

# Build
make CROSS_COMPILE=arm-linux-gnueabihf-

# Transfer binaries to Luckfox
scp switch_controller switch_controller_api client_example root@luckfox-ip:/root/
```

### Option 2: Build Directly on Luckfox (Slower)

```bash
# On the Luckfox Pico Max
cd luckfox-controller

# Build all programs
make

# Or build individually
make switch_controller          # Demo mode
make switch_controller_api      # API mode
make client_example            # Example client
```

**Note:** Building on the Luckfox is slower and may require installing build tools (gcc, make) if not already present.

## Setup

```bash
# Run the setup script to prepare the system
sudo ./setup.sh

# This will:
# - Mount configfs if needed
# - Load required kernel modules (libcomposite, usb_f_hid, dwc2)
# - Verify USB Device Controller is available
# - Build the programs
```

## Usage

### Demo Mode

The simple demo mode runs a pre-programmed sequence:

```bash
sudo ./switch_controller
```

If you get "Device or resource busy" error, first run:
```bash
sudo ./cleanup.sh
```

This will:
1. Set up the USB gadget
2. Wait for you to connect the USB cable to the Switch
3. Automatically press buttons and move sticks in a demo pattern
4. Press Ctrl+C to stop

**Important:** The program will wait for the Switch to connect before starting the demo. Make sure to plug in the USB cable when prompted!

### API Mode (Recommended)

The API mode allows external programs to control the virtual controller:

```bash
# Start the controller API server
sudo ./switch_controller_api

# In another terminal, use the example client
./client_example

# Or send commands manually
echo "BTN 0004" | nc -U /tmp/switch_controller.sock  # Press A button
echo "RESET" | nc -U /tmp/switch_controller.sock     # Reset to neutral
```

## API Commands

The API server listens on Unix socket `/tmp/switch_controller.sock` and accepts these commands:

| Command | Description | Example |
|---------|-------------|---------|
| `BTN <hex>` | Set button state (bitmask) | `BTN 0004` (A button) |
| `STICK <lx> <ly> <rx> <ry>` | Set stick positions (0-255) | `STICK 255 128 128 128` |
| `HAT <pos>` | Set D-pad position (0-7, 8=neutral) | `HAT 0` (up) |
| `RESET` | Reset to neutral state | `RESET` |

### Button Bitmasks

| Button | Hex Value | Decimal |
|--------|-----------|---------|
| Y | 0x0001 | 1 |
| B | 0x0002 | 2 |
| A | 0x0004 | 4 |
| X | 0x0008 | 8 |
| L | 0x0010 | 16 |
| R | 0x0020 | 32 |
| ZL | 0x0040 | 64 |
| ZR | 0x0080 | 128 |
| Minus | 0x0100 | 256 |
| Plus | 0x0200 | 512 |
| L-Click | 0x0400 | 1024 |
| R-Click | 0x0800 | 2048 |
| Home | 0x1000 | 4096 |
| Capture | 0x2000 | 8192 |

### HAT Switch (D-pad) Positions

| Position | Value |
|----------|-------|
| Up | 0 |
| Up-Right | 1 |
| Right | 2 |
| Down-Right | 3 |
| Down | 4 |
| Down-Left | 5 |
| Left | 6 |
| Up-Left | 7 |
| Neutral | 8 |

### Analog Stick Values

- Range: 0-255
- Center: 128
- Format: `STICK <left_x> <left_y> <right_x> <right_y>`

## Client Examples

### C Client

```c
#include <sys/socket.h>
#include <sys/un.h>

int sock = socket(AF_UNIX, SOCK_STREAM, 0);
struct sockaddr_un addr = {.sun_family = AF_UNIX};
strcpy(addr.sun_path, "/tmp/switch_controller.sock");
connect(sock, (struct sockaddr *)&addr, sizeof(addr));

// Press A button
write(sock, "BTN 0004", 8);
close(sock);
```

### Python Client

```python
import socket
import time

def send_command(cmd):
    sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    sock.connect("/tmp/switch_controller.sock")
    sock.send(cmd.encode())
    response = sock.recv(256).decode()
    sock.close()
    return response

# Press A button
send_command("BTN 0004")
time.sleep(0.1)
send_command("BTN 0")  # Release

# Move stick right
send_command("STICK 255 128 128 128")
time.sleep(0.5)
send_command("STICK 128 128 128 128")  # Center
```

### Shell Script

```sh
#!/bin/sh

press_button() {
    echo "BTN $1" | nc -U /tmp/switch_controller.sock
    sleep 0.1
    echo "BTN 0" | nc -U /tmp/switch_controller.sock
    sleep 0.2
}

# Press A button 5 times
i=1
while [ $i -le 5 ]; do
    press_button 0004
    i=$((i + 1))
done
```

## Architecture

The emulator consists of three main components:

1. **USB Descriptor** (`switch_descriptors.h`)
   - Defines HID report descriptor matching POKKEN controller
   - Button and axis mappings
   - Report structure definitions

2. **Gadget Setup** (in main programs)
   - Creates USB gadget via ConfigFS
   - Configures device, configuration, and HID function
   - Binds to USB Device Controller (UDC)

3. **Report Sender**
   - Continuously sends HID input reports at 20Hz
   - Thread-safe state management
   - Socket API for external control (API mode only)

## How It Works

### USB Gadget ConfigFS

The program uses Linux's ConfigFS to dynamically create a USB HID device:

1. Creates gadget directory in `/sys/kernel/config/usb_gadget/`
2. Sets USB descriptors (VID, PID, manufacturer, product)
3. Creates HID function with Switch controller report descriptor
4. Links function to configuration
5. Binds to USB Device Controller

### HID Report Format

The controller sends 8-byte input reports:

```
Byte 0-1: Buttons (16 bits)
Byte 2:   HAT switch (4 bits) + padding (4 bits)
Byte 3:   Left stick X axis
Byte 4:   Left stick Y axis
Byte 5:   Right stick X axis
Byte 6:   Right stick Y axis
Byte 7:   Vendor specific
```

## Troubleshooting

### "Cannot send after transport endpoint shutdown"

This error means the USB host (Switch) hasn't connected yet. The programs now automatically wait for the connection, so you should see a "Waiting for USB host..." message. Simply plug in the USB cable to your Switch when prompted.

If you see this error while the program is running:
- The Switch may have been disconnected or put to sleep
- Try unplugging and re-plugging the USB cable
- Restart the program

### Device or Resource Busy

If you see "Failed to write to UDC: Device or resource busy":

This means the USB Device Controller is already in use. Fix it by running:

```bash
sudo ./cleanup.sh
```

Or manually unbind all gadgets:

```bash
echo "" > /sys/kernel/config/usb_gadget/*/UDC
```

The programs now automatically unbind before binding, but if you're switching between different gadgets or had a crash, you may need to run cleanup first.

### No UDC Found

If you see "No USB Device Controller (UDC) found":

1. Check if USB OTG is enabled in device tree
2. Load dwc2 module: `modprobe dwc2`
3. Ensure USB port is in device mode (not host mode)
4. Check `dmesg` for USB-related errors

### Switch Doesn't Recognize Controller

1. Verify USB cable is connected to Switch dock or handheld mode
2. Check that gadget is bound: `cat /sys/kernel/config/usb_gadget/switch_controller/UDC`
3. Try unplugging and re-plugging the USB cable
4. Check `dmesg` for connection messages

### Permission Denied

The programs must run as root to access ConfigFS and create USB gadgets:

```bash
sudo ./switch_controller_api
```

### HID Device Not Found

If `/dev/hidg*` doesn't exist:

1. Ensure `usb_f_hid` module is loaded: `modprobe usb_f_hid`
2. Check that gadget is properly set up: `ls /sys/kernel/config/usb_gadget/switch_controller/functions/`
3. Verify UDC is bound (see above)

## Technical Details

### USB IDs

- **Vendor ID**: 0x0F0D (HORI CO., LTD.)
- **Product ID**: 0x0092 (POKKEN CONTROLLER)
- **Alternative**: 0x057E:0x2009 (Official Nintendo Pro Controller - may require additional protocol implementation)

### Why POKKEN Controller?

The HORI POKKEN Controller is a licensed third-party controller that:
- Is fully supported by Nintendo Switch
- Uses standard HID reports (simpler than official Pro Controller)
- Doesn't require Nintendo's proprietary authentication
- Has well-documented USB descriptors

### Report Rate

The controller sends input reports at 20Hz (50ms intervals), which is sufficient for most games. This can be adjusted by modifying the `usleep(50000)` value in the report sender thread.

## References

This project was built using information from:

- [dekuNukem/Nintendo_Switch_Reverse_Engineering](https://github.com/dekuNukem/Nintendo_Switch_Reverse_Engineering) - Switch controller protocol documentation
- [Linux USB HID gadget driver documentation](https://docs.kernel.org/usb/gadget_hid.html) - Linux kernel HID gadget API
- [ToadKing's Pro Controller descriptors](https://gist.github.com/ToadKing/b883a8ccfa26adcc6ba9905e75aeb4f2) - USB descriptor dumps
- [kashalls/NintendoSwitchController](https://github.com/kashalls/NintendoSwitchController) - POKKEN controller implementation
- [Yvonne-Aizawa/pico-switch-controller](https://github.com/Yvonne-Aizawa/pico-switch-controller) - Reference implementation

## License

MIT License - Feel free to use and modify for your projects.

## Contributing

Contributions are welcome! Some ideas for improvements:

- [ ] Support for official Pro Controller protocol (0x80/0x01 commands)
- [ ] Gyroscope/accelerometer emulation
- [ ] Rumble/vibration support
- [ ] Web-based control interface
- [ ] Bluetooth mode support
- [ ] Multiple controller emulation
- [ ] Input recording and playback

## Safety Notes

- This emulator is for educational and accessibility purposes
- Do not use to cheat in online games - you may get banned
- The Nintendo Switch and Pro Controller are trademarks of Nintendo
- HORI and POKKEN Controller are trademarks of their respective owners
