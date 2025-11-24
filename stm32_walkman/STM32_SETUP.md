# STM32F4 Walkman Player - Setup Instructions

Complete step-by-step guide to set up and deploy the STM32 Walkman music player.

## Prerequisites

### Development Tools
- **STM32CubeIDE** (latest version) or STM32CubeMX + your favorite IDE
- **ARM GCC Compiler** (included with CubeIDE)
- **Git** (for cloning)
- **OpenOCD** or **STLink utility** (for debugging/flashing)

### Hardware
- STM32F4 Discovery/Nucleo board or custom board with:
  - STM32F407VG/STM32F429 MCU
  - 168MHz capable with 192KB RAM
  - Built-in STLink debugger (Discovery/Nucleo) or external debugger

## Step 1: Project Setup in STM32CubeIDE

### 1.1 Create New Project
```
File → New → STM32 Project
Select Target → Choose STM32F4 variant
  (e.g., STM32F429ZI for maximum features)
Next → Finish
```

### 1.2 Configure Clock
In STM32CubeMX (Device Configuration Tab):
```
Clocks → System Clock Mux → PLLCLK
PLL Configuration:
  - Input: HSE (8MHz)
  - PLLM: 4
  - PLLN: 168
  - PLLP: 2
  - PLLQ: 7
Result: 168 MHz system clock
```

### 1.3 Configure Peripherals

#### Enable SPI2 (for LCD)
```
Connectivity → SPI2
Mode: Full-Duplex Master
Frame Format: Motorola (SPI)
Data Size: 8-bit
Clock Polarity: Low
Clock Phase: 1 Edge
```

**Pin Configuration for SPI2:**
```
PB13 → SPI2_SCK
PB14 → SPI2_MISO (optional)
PB15 → SPI2_MOSI
```

**Control Pins (GPIO Output):**
```
PA4  → GPIO_Output (CS)
PA5  → GPIO_Output (DC)
PA6  → GPIO_Output (RST)
```

#### Enable SPI3 (for I2S Audio)
```
Connectivity → SPI3
Mode: Full-Duplex Master, I2S mode
I2S Standard: Philips
Data Format: 16-bit
Audio Frequency: 44100 Hz
CPOL: Low
Clock Source: External
```

**Pin Configuration for I2S/SPI3:**
```
PB12 → I2S3_WS  (Word Select)
PB13 → I2S3_CK  (Clock)
PC3  → I2S3_SD  (Serial Data)
PC6  → I2S3_MCK (Master Clock - optional)
```

#### Configure SD Card (SDIO)
```
Connectivity → SDIO
Mode: SD 4-bit mode
Bus Width: 4-bit

Pin Configuration:
PC8  → SDIO_D0
PC9  → SDIO_D1
PC10 → SDIO_D2
PC11 → SDIO_D3
PD2  → SDIO_CMD
PC12 → SDIO_CK
```

#### Configure GPIO for Buttons
```
Port B → Configure as GPIO_Input with GPIO_PullUp:
PB0  → Button Previous
PB1  → Button Play/Pause
PB2  → Button Next
PB3  → Button Volume Up
PB4  → Button Volume Down
PB5  → Button Shuffle
PB6  → Button Loop
```

### 1.4 Project Settings

In Project Manager tab:
```
Project Name: stm32_walkman
Application Structure: Advanced
Toolchain: STM32CubeIDE
Generate Under Root: Unchecked (better organization)
```

Click "Generate Code"

## Step 2: Add Source Files

### 2.1 Create Directory Structure
In STM32CubeIDE Project Explorer:
```
src/
├── audio/
├── lcd/
├── buttons/
└── main.c
```

### 2.2 Copy Files
Copy the following files to your project:
```
src/audio/player.h
src/audio/player.c
src/lcd/lcd_display.h
src/lcd/lcd_display.c
src/buttons/buttons.h
src/buttons/buttons.c
src/main.c (replace generated one)
```

## Step 3: Configure Build Settings

### 3.1 Include Paths
Project → Properties → C/C++ Build → Settings

**GCC C Compiler → Includes:**
```
${workspace_loc:/${ProjName}/src}
${workspace_loc:/${ProjName}/src/audio}
${workspace_loc:/${ProjName}/src/lcd}
${workspace_loc:/${ProjName}/src/buttons}
```

### 3.2 Compiler Optimizations
GCC C Compiler → Optimization:
```
Optimization Level: -O2 (optimize for speed)
Other Optimization Flags: -ffunction-sections -fdata-sections
```

### 3.3 Linker Script
Ensure the linker script is correct:
```
Project → Properties → C/C++ Build → Settings
GCC C Linker → Script
Check: STM32F4xx_FLASH.ld (in LinkerScript folder)
```

## Step 4: Hardware Wiring

### Wiring Diagram (STM32F4 Discovery)

```
LCD Display (ILI9341) Connections:
┌─────────────────────┐
│  ILI9341 240x320    │
├─────────────────────┤
│ VCC    → 3.3V       │
│ GND    → GND        │
│ CS     → PA4        │
│ DC     → PA5        │
│ RST    → PA6        │
│ MOSI   → PB15       │
│ MISO   → PB14       │
│ SCK    → PB13       │
│ LED    → 3.3V (opt) │
└─────────────────────┘

Audio Codec Connections:
┌─────────────────────┐
│   WM8994 Codec      │
├─────────────────────┤
│ VDD    → 3.3V       │
│ GND    → GND        │
│ I2S_CK  → PB13      │
│ I2S_WS  → PB12      │
│ I2S_SD  → PC3       │
│ I2C_SCL → PB6       │
│ I2C_SDA → PB7       │
└─────────────────────┘

Button Connections (all pull to GND):
┌─────────────────────┐
│ Button Row         │
├─────────────────────┤
│ PB0 → Previous      │
│ PB1 → Play/Pause    │
│ PB2 → Next          │
│ PB3 → Vol+          │
│ PB4 → Vol-          │
│ PB5 → Shuffle       │
│ PB6 → Loop          │
└─────────────────────┘
```

### Decoupling Capacitors
```
Power Supply:
- 100µF on main VCC
- 10µF on each major IC (LCD, Codec)
- 100nF ceramic near each VCC pin
```

## Step 5: Build and Flash

### 5.1 Build Project
```
Project → Build Project
(or Ctrl+B)
```

### 5.2 Flash to Hardware
Option A - STLink (built-in on Discovery/Nucleo):
```
Run → Run As → STM32 C/C++ Application
(debugger will start and halt at main)
```

Option B - Manual OpenOCD:
```bash
# Terminal 1: Start OpenOCD server
openocd -f interface/stlink-v2.cfg -f target/stm32f4x.cfg

# Terminal 2: Flash using GDB
arm-none-eabi-gdb build/stm32_walkman.elf
> target remote localhost:3333
> load
> continue
```

## Step 6: Testing

### 6.1 Serial Debug Output
Connect to STM32 USB/UART at 115200 baud:
```
Monitor output for initialization messages:
- "Audio player initialized"
- "LCD display initialized"
- "Buttons initialized"
- "Loaded 3 tracks"
```

### 6.2 LCD Display Check
After startup, you should see:
```
┌──────────────────────────────┐
│   WALKMAN PLAYER             │
│   Loading...                 │
└──────────────────────────────┘
```

### 6.3 Button Testing
Press each button and verify in debug output:
```
Button: Previous
Button: Play/Pause
... etc
```

## Step 7: SD Card Setup

### 7.1 Prepare SD Card
```
1. Format microSD to FAT32
2. Create /MUSIC folder
3. Copy MP3/WAV files to /MUSIC
4. Insert into STM32 board
```

### 7.2 Enable SD Card Detection
Uncomment in main.c:
```c
// TODO: Enumerate SD card directory and load music files
// This requires FatFS library implementation
```

## Customization Guide

### Change LCD Size
If using 480x800 display instead of 240x320:

1. Edit `src/lcd/lcd_display.h`:
```c
#define LCD_WIDTH  480
#define LCD_HEIGHT 800
```

2. Adjust button positioning in `lcd_display.c`

### Change Button Pins
Edit `src/buttons/buttons.c`:
```c
static button_config_t buttons[NUM_BUTTONS] = {
    {GPIOB, GPIO_PIN_10, BTN_PREVIOUS, ...},  // Changed from PB0
    // ... rest of buttons
};
```

### Adjust Volume Steps
In `src/main.c`:
```c
#define VOLUME_STEP 2  // Was 5, now 2% per press
```

## Troubleshooting

### Build Errors

**Error: "stm32f4xx_hal.h not found"**
- Ensure STM32CubeMX generated all files
- Check Include Paths in project settings

**Error: "undefined reference to HAL_Init"**
- Check that src/stm32f4xx_it.c exists (generated by CubeMX)
- Verify HAL startup files in Drivers/STM32F4xx_HAL_Driver/

### Runtime Issues

**LCD shows only white/black screen**
- Check SPI clock speed (max 10MHz for LCD)
- Verify CS, DC, RST pins connected correctly
- Try adjusting initialization delay in lcd_reset()

**No sound from audio output**
- Verify I2S and codec are synchronized
- Check audio file is actually loaded to SD card
- Verify codec power supply and I2C setup

**Buttons not responding**
- Check GPIO pins configured as INPUT with PULLUP
- Verify button wiring to GND
- Test with buttons_is_pressed() in debugger

## Performance Optimization

### For Memory-Constrained Devices

Reduce playlist size in `src/main.c`:
```c
#define MAX_PLAYLIST_SIZE 50  // Was 100
```

### For Faster Response

Increase button poll frequency:
```c
#define UPDATE_INTERVAL_MS 50  // Was 100 (more responsive)
```

## Next Steps

1. **Integrate FatFS** for proper SD card file browsing
2. **Add MP3 Decoder** (e.g., libmad or helix-mp3)
3. **Implement I2C Codec Control** for volume adjustment
4. **Add RTC** for time display
5. **Optimize LCD Drawing** with partial updates

## Resources

- STM32F4 Reference Manual
- ILI9341 Display Datasheet
- WM8994 Codec Datasheet
- STM32CubeIDE Documentation
- HAL User Manual
