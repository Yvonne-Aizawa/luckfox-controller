#!/bin/sh
#
# Install the Switch controller as a service that runs at boot
#

set -e

if [ "$(id -u)" -ne 0 ]; then
    echo "Please run as root (use sudo)"
    exit 1
fi

echo "Installing Switch Controller service..."

# Get the current directory
INSTALL_DIR="$(pwd)"

# Copy files to /opt/switch_controller
echo "Copying files to /opt/switch_controller..."
mkdir -p /opt/switch_controller
cp switch_controller /opt/switch_controller/
cp switch_controller_api /opt/switch_controller/
cp switch_descriptors.h /opt/switch_controller/
cp cleanup.sh /opt/switch_controller/

# Create init script for SysV init or similar
cat > /etc/init.d/switch_controller << 'INITEOF'
#!/bin/sh
### BEGIN INIT INFO
# Provides:          switch_controller
# Required-Start:    $local_fs $remote_fs
# Required-Stop:     $local_fs $remote_fs
# Default-Start:     2 3 4 5
# Default-Stop:      0 1 6
# Short-Description: Nintendo Switch Controller Emulator
# Description:       Emulates a Switch controller via USB Gadget
### END INIT INFO

PATH=/sbin:/usr/sbin:/bin:/usr/bin
DAEMON=/opt/switch_controller/switch_controller_api
NAME=switch_controller
PIDFILE=/var/run/$NAME.pid

# Mount configfs if not already mounted
if [ ! -d "/sys/kernel/config" ]; then
    modprobe configfs 2>/dev/null || true
    mount -t configfs none /sys/kernel/config 2>/dev/null || true
fi

# Load modules
modprobe libcomposite 2>/dev/null || true
modprobe usb_f_hid 2>/dev/null || true
modprobe dwc2 2>/dev/null || true

case "$1" in
  start)
    echo "Starting $NAME..."
    # Cleanup any existing gadgets
    if [ -d "/sys/kernel/config/usb_gadget" ]; then
        for gadget in /sys/kernel/config/usb_gadget/*; do
            if [ -f "$gadget/UDC" ]; then
                echo "" > "$gadget/UDC" 2>/dev/null || true
            fi
        done
    fi
    sleep 0.2

    # Start the daemon
    start-stop-daemon --start --quiet --background --make-pidfile \
        --pidfile $PIDFILE --exec $DAEMON || true
    echo "Started $NAME"
    ;;
  stop)
    echo "Stopping $NAME..."
    start-stop-daemon --stop --quiet --pidfile $PIDFILE || true
    rm -f $PIDFILE

    # Cleanup gadget
    if [ -f "/sys/kernel/config/usb_gadget/switch_controller/UDC" ]; then
        echo "" > /sys/kernel/config/usb_gadget/switch_controller/UDC 2>/dev/null || true
    fi
    sleep 0.2
    echo "Stopped $NAME"
    ;;
  restart)
    $0 stop
    sleep 1
    $0 start
    ;;
  status)
    if [ -f $PIDFILE ] && kill -0 $(cat $PIDFILE) 2>/dev/null; then
      echo "$NAME is running"
      exit 0
    else
      echo "$NAME is not running"
      exit 1
    fi
    ;;
  *)
    echo "Usage: $0 {start|stop|restart|status}"
    exit 1
    ;;
esac

exit 0
INITEOF

chmod +x /etc/init.d/switch_controller

# Enable the service
if command -v update-rc.d >/dev/null 2>&1; then
    update-rc.d switch_controller defaults
    echo "Service enabled via update-rc.d"
elif command -v rc-update >/dev/null 2>&1; then
    rc-update add switch_controller default
    echo "Service enabled via rc-update"
else
    # Try to create a simple symlink for manual start
    ln -sf /etc/init.d/switch_controller /etc/rc5.d/S99switch_controller 2>/dev/null || true
    echo "Service script installed (manual start may be needed)"
fi

echo ""
echo "Installation complete!"
echo ""
echo "The Switch controller will now start automatically at boot."
echo ""
echo "Manual control:"
echo "  /etc/init.d/switch_controller start   # Start the service"
echo "  /etc/init.d/switch_controller stop    # Stop the service"
echo "  /etc/init.d/switch_controller status  # Check status"
echo ""
echo "To uninstall:"
echo "  Run: sudo ./uninstall_service.sh"
echo ""
