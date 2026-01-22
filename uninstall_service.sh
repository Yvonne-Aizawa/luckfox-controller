#!/bin/sh
#
# Uninstall the Switch controller service
#

set -e

if [ "$(id -u)" -ne 0 ]; then
    echo "Please run as root (use sudo)"
    exit 1
fi

echo "Uninstalling Switch Controller service..."

# Stop the service if running
/etc/init.d/switch_controller stop 2>/dev/null || true

# Disable the service
if command -v update-rc.d >/dev/null 2>&1; then
    update-rc.d -f switch_controller remove
elif command -v rc-update >/dev/null 2>&1; then
    rc-update del switch_controller default
else
    rm -f /etc/rc*.d/*switch_controller
fi

# Remove the init script
rm -f /etc/init.d/switch_controller

# Remove installed files
rm -rf /opt/switch_controller

echo "Uninstallation complete!"
