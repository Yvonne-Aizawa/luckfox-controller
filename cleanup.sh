#!/bin/sh
#
# Cleanup script to remove any existing USB gadgets
#

echo "Cleaning up USB gadgets..."

# Unbind all UDCs
for udc in /sys/class/udc/*; do
    if [ -e "$udc" ]; then
        udc_name=$(basename "$udc")
        echo "Unbinding UDC: $udc_name"

        # Find all gadgets and unbind them
        for gadget in /sys/kernel/config/usb_gadget/*; do
            if [ -d "$gadget" ] && [ -f "$gadget/UDC" ]; then
                echo "" > "$gadget/UDC" 2>/dev/null || true
            fi
        done
    fi
done

# Remove switch_controller gadget if it exists
GADGET_PATH="/sys/kernel/config/usb_gadget/switch_controller"
if [ -d "$GADGET_PATH" ]; then
    echo "Removing switch_controller gadget..."

    # Unbind from UDC
    if [ -f "$GADGET_PATH/UDC" ]; then
        echo "" > "$GADGET_PATH/UDC" 2>/dev/null || true
    fi

    # Remove symlinks
    rm -f "$GADGET_PATH/configs/c.1/hid.usb0" 2>/dev/null || true

    # Remove directories in reverse order
    rmdir "$GADGET_PATH/configs/c.1/strings/0x409" 2>/dev/null || true
    rmdir "$GADGET_PATH/configs/c.1" 2>/dev/null || true
    rmdir "$GADGET_PATH/functions/hid.usb0" 2>/dev/null || true
    rmdir "$GADGET_PATH/strings/0x409" 2>/dev/null || true
    rmdir "$GADGET_PATH" 2>/dev/null || true
fi

echo "Cleanup complete!"
echo
echo "You can now run: sudo ./switch_controller"
