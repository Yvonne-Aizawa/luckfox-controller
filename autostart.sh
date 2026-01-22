#!/bin/sh
#
# Auto-start script for Switch Controller
# This script should run at boot to automatically start the controller
# when the Luckfox is powered by the Switch
#

# Wait for system to be ready
sleep 2

# Setup and run the controller
cd /root/luckfox-controller || exit 1

# Load modules if not already loaded
modprobe libcomposite 2>/dev/null || true
modprobe usb_f_hid 2>/dev/null || true
modprobe dwc2 2>/dev/null || true

# Run the controller (or API mode)
exec ./switch_controller_api
