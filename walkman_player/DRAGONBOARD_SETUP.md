# DragonBoard 410c Setup Guide

## Overview

This guide covers setting up the Walkman Music Player on a DragonBoard 410c with GPIO button controls.

## Prerequisites

- DragonBoard 410c
- 5 × Momentary Push Buttons
- 5 × 10kΩ Resistors (optional)
- Breadboard and Jumper Wires
- USB Power Supply (≥2A recommended)
- USB Sound Card or HDMI Audio Connection
- Debian/Linux OS installed on DragonBoard
- SSH access or keyboard/display

## Step 1: System Preparation

### Update System

```bash
sudo apt-get update
sudo apt-get upgrade -y
```

### Install Python and Dependencies

```bash
sudo apt-get install python3 python3-pip python3-dev -y
```

### Install System Audio Libraries

```bash
sudo apt-get install python3-dev libsndfile1-dev libportaudio2 portaudio19-dev -y
sudo pip3 install --upgrade pip setuptools wheel
```

## Step 2: Install Python Dependencies

Navigate to the project directory and install:

```bash
cd /path/to/walkman_player
sudo pip3 install -r requirements.txt
```

Or individually:

```bash
sudo pip3 install pygame
```

## Step 3: GPIO Hardware Setup

### GPIO Pin Reference (DragonBoard 410c)

The DragonBoard 410c uses APQ8016 SoC with the following GPIO mapping:

| Physical Pin | GPIO Number | Signal Name | Default Function |
|-------------|------------|-------------|------------------|
| 36          | 52         | GPIO_L8     | GPIO |
| 26          | 47         | GPIO_L3     | GPIO |
| 24          | 45         | GPIO_L1     | GPIO |
| 32          | 50         | GPIO_L6     | GPIO |
| 30          | 48         | GPIO_L4     | GPIO |

### Wiring Diagram

```
DragonBoard 410c Low Speed Expansion Connector (40-pin)
Pin Layout (looking at the board with connector on bottom left):

[Pin 1]  3.3V      [Pin 2]  5V
[Pin 3]  I2C_SDA   [Pin 4]  5V  
[Pin 5]  I2C_SCL   [Pin 6]  GND
[Pin 7]  GPIO_A    [Pin 8]  UART_TX
[Pin 9]  GND       [Pin 10] UART_RX
[Pin 11] GPIO_B    [Pin 12] GPIO_C
[Pin 13] GPIO_D    [Pin 14] GND
[Pin 15] GPIO_E    [Pin 16] GPIO_F
[Pin 17] GND       [Pin 18] GPIO_G
[Pin 19] SPI_CLK   [Pin 20] GND
...
[Pin 30] GPIO_L4   [Pin 32] GPIO_L6
[Pin 36] GPIO_L8   [Pin 40] GND
```

### Button Connection (Pull-up Configuration)

For each button, connect:

```
┌─ 3.3V ─ [10kΩ Resistor] ─┬─ GPIO Pin
└─────────────────────────┘
                            │
                        [Button]
                            │
┌────────────────────────────┘
│
GND
```

### Example: Play/Pause Button

```
Pin 36 (GPIO_L8) ─ [Pull-up 10kΩ] ─ 3.3V (Pin 1)
Pin 36 (GPIO_L8) ─ [Button] ─ GND (Pin 39)
```

## Step 4: Configure GPIO Pins

Edit `config/config.py` and update GPIO_CONFIG:

```python
GPIO_CONFIG = {
    'mode': 'BCM',
    'pins': {
        'play_pause': 36,   # Physical pin 36 → GPIO 52
        'next': 26,         # Physical pin 26 → GPIO 47
        'prev': 24,         # Physical pin 24 → GPIO 45
        'vol_up': 32,       # Physical pin 32 → GPIO 50
        'vol_down': 30,     # Physical pin 30 → GPIO 48
    },
    'debounce_ms': 200,
}
```

## Step 5: Configure Application

Edit `config/config.py`:

```python
APP_CONFIG = {
    'use_gpio_simulator': False,  # Use real GPIO
    'debug': True,                # Enable debug output
    'auto_play_on_load': False,
}
```

## Step 6: Setup Audio Output

### USB Sound Card Option

```bash
# List audio devices
aplay -l

# Edit ALSA configuration if needed
sudo nano /etc/asound.conf
```

### HDMI Audio Option

```bash
# Enable HDMI audio (may require reboot)
amixer cset numid=3 2  # 0=auto, 1=headphones, 2=HDMI
```

## Step 7: Test GPIO Setup

Create a test script `test_gpio.py`:

```python
#!/usr/bin/env python3
import RPi.GPIO as GPIO
import time

GPIO.setmode(GPIO.BCM)
GPIO.setup(52, GPIO.IN, pull_up_down=GPIO.PUD_UP)  # GPIO_L8

try:
    while True:
        if GPIO.input(52) == GPIO.LOW:
            print("Button pressed!")
            time.sleep(0.5)  # Debounce
        time.sleep(0.1)
except KeyboardInterrupt:
    print("Test ended")
finally:
    GPIO.cleanup()
```

Run with:
```bash
sudo python3 test_gpio.py
```

## Step 8: Run the Application

```bash
# First time - with simulator (for testing)
python3 main.py

# With real GPIO (requires sudo for GPIO access)
sudo python3 main.py
```

## Step 9: Create SystemD Service (Optional)

For automatic startup, create `/etc/systemd/system/walkman-player.service`:

```ini
[Unit]
Description=Walkman Music Player
After=network.target

[Service]
Type=simple
User=root
WorkingDirectory=/path/to/walkman_player
ExecStart=/usr/bin/python3 /path/to/walkman_player/main.py
Restart=on-failure
RestartSec=10

[Install]
WantedBy=multi-user.target
```

Enable and start:
```bash
sudo systemctl enable walkman-player.service
sudo systemctl start walkman-player.service
sudo systemctl status walkman-player.service
```

View logs:
```bash
sudo journalctl -u walkman-player.service -f
```

## Step 10: Music Directory Setup

```bash
# Create music directory
mkdir -p ~/music

# Copy music files
cp /path/to/your/music/*.mp3 ~/music/

# Set permissions
chmod 755 ~/music
```

## Troubleshooting

### GPIO Access Denied

```bash
# Add user to gpio group
sudo usermod -a -G gpio $USER
sudo usermod -a -G audio $USER

# Reboot required
sudo reboot
```

### No Audio Output

```bash
# Test audio
speaker-test -t sine -f 1000 -l 5

# Check audio devices
arecord -l
aplay -l

# Check volume
alsamixer
```

### Button Not Responding

1. Test GPIO manually:
```bash
# Check GPIO state
cat /sys/class/gpio/gpio52/value  # 0 = pressed, 1 = not pressed
```

2. Verify wiring with multimeter
3. Check pull-up resistors are correct value
4. Verify GPIO configuration in config.py matches physical connections

### Application Crashes

Check logs:
```bash
sudo journalctl -u walkman-player.service -n 50
```

## Performance Optimization

### Disable Unnecessary Services

```bash
sudo systemctl disable avahi-daemon
sudo systemctl disable bluetooth
```

### Set CPU Governor

```bash
echo powersave | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor
```

## Advanced: GPIO Pinout Reference

Full DragonBoard 410c Low Speed Expansion (40-pin):

```
Pin Layout:
1   3.3V          2   5V
3   I2C_SDA       4   5V  
5   I2C_SCL       6   GND
7   GPIO_A(117)   8   UART_TX
9   GND          10   UART_RX
11  GPIO_B(115)  12   GPIO_C(116)
13  GPIO_D(113)  14   GND
15  GPIO_E(114)  16   GPIO_F(112)
17  GND          18   GPIO_G(111)
19  SPI_CLK      20   GND
21  SPI_MOSI     22   SPI_MISO
23  SPI_CS       24   GPIO_L1(45)
25  GND          26   GPIO_L3(47)
27  I2C_SDA      28   I2C_SCL
29  GPIO_J(10)   30   GPIO_L4(48)
31  GPIO_K(11)   32   GPIO_L6(50)
33  GND          34   GPIO_L7(51)
35  GPIO_L8(52)  36   GPIO_H(9)
37  GPIO_I(12)   38   GND
39  GND          40   RESERVED
```

## Support and Resources

- DragonBoard Documentation: https://www.96boards.org/documentation/
- GPIO Mapping: Check 96boards dragonboard GPIO pinout
- Python GPIO: https://sourceforge.net/p/raspberry-pi-gpio-python/

---

**Last Updated**: 2025
**For**: DragonBoard 410c with Debian Linux
