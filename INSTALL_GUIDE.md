# Installation Guide for Luckfox

## Step 1: Build on Your PC

On your development PC (not the Luckfox):

```bash
cd luckfox-controller

# Cross-compile for ARM
make CROSS_COMPILE=arm-linux-gnueabihf-

# Or if you have the Luckfox SDK toolchain:
# export CROSS_COMPILE=/path/to/luckfox-pico/tools/linux/toolchain/arm-rockchip830-linux-uclibcgnueabihf/bin/arm-rockchip830-linux-uclibcgnueabihf-
# make CROSS_COMPILE=$CROSS_COMPILE
```

## Step 2: Transfer Files to Luckfox

Transfer the built programs and scripts to the Luckfox:

```bash
# Replace <luckfox-ip> with your Luckfox's IP address
scp -r * root@<luckfox-ip>:/root/luckfox-controller/

# Or if using a specific directory:
ssh root@<luckfox-ip> "mkdir -p /root/luckfox-controller"
scp switch_controller switch_controller_api client_example *.sh *.h root@<luckfox-ip>:/root/luckfox-controller/
```

## Step 3: Install on Luckfox

SSH into the Luckfox and install:

```bash
ssh root@<luckfox-ip>

cd /root/luckfox-controller

# Make scripts executable
chmod +x *.sh

# Install the service
./install_service.sh
```

## Step 4: Test

After installation, the service should start automatically when you plug the Luckfox into the Switch.

To test immediately:

```bash
# Start the service
/etc/init.d/switch_controller start

# Check status
/etc/init.d/switch_controller status

# View logs
dmesg | tail -30
```

## Troubleshooting

### "couldn't find an available UDC or it's busy"

If you see this in dmesg, there's likely another USB gadget using the UDC. Fix:

```bash
# On the Luckfox, unbind all gadgets
echo "" > /sys/kernel/config/usb_gadget/*/UDC 2>/dev/null

# Or run cleanup
cd /root/luckfox-controller
./cleanup.sh

# Restart the service
/etc/init.d/switch_controller restart
```

### Device keeps resetting

If you see repeated "device reset" messages in dmesg, the Switch may not be recognizing the controller properly. This can happen if:

1. The gadget setup failed - check `/etc/init.d/switch_controller status`
2. Another gadget is bound - run cleanup script
3. The program crashed - check for error messages

Try running manually to see errors:

```bash
cd /root/luckfox-controller
./switch_controller_api
# Watch for any error messages
```

## Quick Reference

```bash
# Start service
/etc/init.d/switch_controller start

# Stop service
/etc/init.d/switch_controller stop

# Restart service
/etc/init.d/switch_controller restart

# Check status
/etc/init.d/switch_controller status

# View kernel messages
dmesg | tail -50

# Cleanup USB gadgets
./cleanup.sh

# Uninstall service
./uninstall_service.sh
```
