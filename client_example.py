#!/usr/bin/env python3
"""
Python client example for Switch Controller API
Demonstrates how to control the virtual controller from Python
"""

import socket
import time

SOCKET_PATH = "/tmp/switch_controller.sock"

# Button masks
BTN_Y = 0x0001
BTN_B = 0x0002
BTN_A = 0x0004
BTN_X = 0x0008
BTN_L = 0x0010
BTN_R = 0x0020
BTN_ZL = 0x0040
BTN_ZR = 0x0080
BTN_MINUS = 0x0100
BTN_PLUS = 0x0200
BTN_LCLICK = 0x0400
BTN_RCLICK = 0x0800
BTN_HOME = 0x1000
BTN_CAPTURE = 0x2000

# HAT positions
HAT_UP = 0
HAT_UP_RIGHT = 1
HAT_RIGHT = 2
HAT_DOWN_RIGHT = 3
HAT_DOWN = 4
HAT_DOWN_LEFT = 5
HAT_LEFT = 6
HAT_UP_LEFT = 7
HAT_NEUTRAL = 8


def send_command(cmd):
    """Send a command to the controller API"""
    try:
        sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        sock.connect(SOCKET_PATH)
        sock.send(cmd.encode())
        response = sock.recv(256).decode()
        sock.close()
        return response.strip()
    except Exception as e:
        print(f"Error: {e}")
        return None


def press_button(name, mask, duration=0.1):
    """Press and release a button"""
    print(f"Pressing {name} button...")
    send_command(f"BTN {mask:04x}")
    time.sleep(duration)
    send_command("BTN 0")
    time.sleep(0.2)


def move_stick(stick, x, y, duration=0.5):
    """Move a stick to a position
    stick: 'left' or 'right'
    x, y: 0-255 (128 is center)
    """
    if stick == 'left':
        send_command(f"STICK {x} {y} 128 128")
    else:
        send_command(f"STICK 128 128 {x} {y}")
    time.sleep(duration)
    send_command("STICK 128 128 128 128")  # Center
    time.sleep(0.2)


def press_dpad(direction):
    """Press a D-pad direction"""
    directions = {
        'up': HAT_UP,
        'right': HAT_RIGHT,
        'down': HAT_DOWN,
        'left': HAT_LEFT
    }
    if direction in directions:
        print(f"Pressing D-pad {direction}...")
        send_command(f"HAT {directions[direction]}")
        time.sleep(0.1)
        send_command(f"HAT {HAT_NEUTRAL}")
        time.sleep(0.2)


def reset():
    """Reset controller to neutral state"""
    print("Resetting controller...")
    send_command("RESET")
    time.sleep(0.2)


def main():
    print("Switch Controller Python Client Example")
    print("=" * 40)
    print()

    # Reset to neutral state
    reset()

    # Press individual buttons
    press_button("A", BTN_A)
    press_button("B", BTN_B)
    press_button("X", BTN_X)
    press_button("Y", BTN_Y)

    # Press multiple buttons together
    print("Pressing L+R together...")
    send_command(f"BTN {BTN_L | BTN_R:04x}")
    time.sleep(0.1)
    send_command("BTN 0")
    time.sleep(0.5)

    # Move left stick in a circle
    print("Moving left stick in a circle...")
    move_stick('left', 128, 0)    # Up
    move_stick('left', 255, 0)    # Up-right
    move_stick('left', 255, 128)  # Right
    move_stick('left', 255, 255)  # Down-right
    move_stick('left', 128, 255)  # Down
    move_stick('left', 0, 255)    # Down-left
    move_stick('left', 0, 128)    # Left
    move_stick('left', 0, 0)      # Up-left

    # Use D-pad
    press_dpad('up')
    press_dpad('down')
    press_dpad('left')
    press_dpad('right')

    # Press special buttons
    press_button("Plus", BTN_PLUS)
    press_button("Minus", BTN_MINUS)

    # Combo: Take a screenshot (Capture button)
    press_button("Capture", BTN_CAPTURE, duration=0.1)

    # Combo: Return to home screen
    press_button("Home", BTN_HOME)

    print()
    print("Demo complete!")


if __name__ == "__main__":
    main()
