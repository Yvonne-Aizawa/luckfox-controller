#!/bin/sh
#
# Setup script for Switch Controller Emulator on Luckfox Pico Max
#

set -e

echo "Switch Controller Emulator Setup"
echo "================================="
echo

# Check if running as root
if [ "$(id -u)" -ne 0 ]; then
    echo "Please run as root (use sudo)"
    exit 1
fi

# Check if configfs is mounted
if [ ! -d "/sys/kernel/config" ]; then
    echo "Mounting configfs..."
    modprobe configfs
    mount -t configfs none /sys/kernel/config
fi

# Load required modules
echo "Loading USB gadget modules..."
modprobe libcomposite 2>/dev/null || true
modprobe usb_f_hid 2>/dev/null || true
modprobe dwc2 2>/dev/null || true

# Check for UDC
if [ ! -d "/sys/class/udc" ] || [ -z "$(ls -A /sys/class/udc 2>/dev/null)" ]; then
    echo "Warning: No USB Device Controller (UDC) found!"
    echo "This device may not support USB gadget mode."
    echo
    echo "For Luckfox Pico Max, you may need to:"
    echo "1. Enable USB OTG in device tree"
    echo "2. Load dwc2 module with correct parameters"
    echo "3. Ensure USB port is in device mode (not host mode)"
    exit 1
fi

echo "UDC found: $(ls /sys/class/udc)"
echo

# Build the programs
if [ -f "Makefile" ]; then
    echo "Building programs..."
    make clean
    make
    echo
fi

echo "Setup complete!"
echo
echo "To run the controller emulator:"
echo "  sudo ./switch_controller          # Simple demo mode"
echo "  sudo ./switch_controller_api      # API mode with socket control"
echo
echo "To test the API:"
echo "  ./client_example"
echo
