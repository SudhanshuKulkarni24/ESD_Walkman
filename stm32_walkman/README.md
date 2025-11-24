# STM32 Walkman Music Player

A full-featured embedded music player for STM32 microcontrollers with a small LCD display and GPIO button controls.

## Features

- **Audio Playback**: I2S-based audio output supporting WAV and MP3 formats
- **LCD Display**: 240x320 TFT display (ILI9341) showing current song and playback info
- **Button Controls**: Physical buttons for playback control (Previous, Play/Pause, Next, Volume, Shuffle, Loop)
- **Playlist Management**: Load and navigate through music files from SD card
- **Shuffle & Loop**: Advanced playback modes with visual indication
- **Volume Control**: Hardware volume adjustment with display feedback

## Hardware Requirements

### Essential Components
- **MCU**: STM32F4 series (STM32F407, STM32F429, or similar)
- **LCD Display**: ILI9341 240x320 TFT (SPI interface)
- **Audio Codec**: WM8994 or similar I2S-compatible codec
- **Storage**: microSD card slot for music files
- **Buttons**: 7x GPIO-connected push buttons
- **Power**: USB or external power supply

### Pinout Configuration

#### SPI (LCD Communication)
- **MOSI**: PB15 (SPI2_MOSI)
- **MISO**: PB14 (SPI2_MISO) - optional, not used for display
- **SCK**: PB13 (SPI2_SCK)
- **CS**: PA4 (Chip Select)
- **DC**: PA5 (Data/Command)
- **RST**: PA6 (Reset)

#### I2S (Audio Output)
- **WS (Word Select)**: PB12
- **SCK (Serial Clock)**: PB13
- **SD (Serial Data)**: PC3
- **MCLK (Master Clock)**: PC6

#### GPIO Buttons (GPIOB)
- **PB0**: Previous Track
- **PB1**: Play/Pause
- **PB2**: Next Track
- **PB3**: Volume Up
- **PB4**: Volume Down
- **PB5**: Shuffle Toggle
- **PB6**: Loop Mode Cycle

#### SD Card (SDIO)
- **D0**: PC8
- **D1**: PC9
- **D2**: PC10
- **D3**: PC11
- **CMD**: PD2
- **CLK**: PC12

## Software Architecture

```
stm32_walkman/
├── src/
│   ├── audio/
│   │   ├── player.h       - Audio playback interface
│   │   └── player.c       - I2S driver and playback control
│   ├── lcd/
│   │   ├── lcd_display.h  - LCD interface
│   │   └── lcd_display.c  - ILI9341 driver and drawing functions
│   ├── buttons/
│   │   ├── buttons.h      - Button interface
│   │   └── buttons.c      - GPIO debouncing and callbacks
│   └── main.c             - Main application logic
├── README.md              - This file
└── STM32_SETUP.md         - Detailed setup instructions
```

## Compilation and Deployment

### Prerequisites
- STM32CubeMX or STM32CubeIDE
- ARM GCC Compiler
- STM32 HAL libraries
- OpenOCD or JLink debugger

### Build Instructions

1. **Create STM32 Project**
   ```bash
   # Using STM32CubeIDE
   # File → New → STM32 Project
   # Select your STM32F4 board variant
   ```

2. **Copy Source Files**
   - Copy all files from `src/` to your project

3. **Configure HAL**
   - Use STM32CubeMX to configure:
     - SPI2 for LCD (4-wire mode, DMA disabled)
     - SPI3 for I2S audio output
     - SDIO for SD card
     - GPIO for buttons
     - System clock to 168 MHz

4. **Compile**
   ```bash
   make all
   ```

5. **Flash to MCU**
   ```bash
   # Using OpenOCD
   openocd -f interface/stlink-v2.cfg -f target/stm32f4x.cfg
   
   # In another terminal
   telnet localhost 4444
   > program build/stm32_walkman.elf verify reset
   ```

## Operation

### Button Functions
- **Previous (PB0)**: Go to previous track or restart current
- **Play/Pause (PB1)**: Start/pause playback
- **Next (PB2)**: Go to next track
- **Volume+ (PB3)**: Increase volume by 5%
- **Volume- (PB4)**: Decrease volume by 5%
- **Shuffle (PB5)**: Toggle shuffle mode
- **Loop (PB6)**: Cycle through loop modes (OFF → ALL → ONE)

### Display Layout

```
┌─────────────────────────┐
│   NOW PLAYING [████]    │  Header (Dark Green)
├─────────────────────────┤
│                         │
│  Song Title             │  Song Information
│  Artist Name            │
│                         │
│ [████████░░░░] 1:23/3:45│  Progress Bar & Time
│                         │
│  ◀        ▶        ▶▶   │  Control Buttons
│                         │
│ Volume: 70%             │  Status Line
└─────────────────────────┘
```

## Audio Format Support

### Supported Formats
- **WAV**: PCM, 16-bit, 44.1kHz - fully supported
- **MP3**: 128-320kbps, MPEG-1 Layer 3 - with decoder chip
- **FLAC**: optional with decoder library
- **OGG**: optional with decoder library

### Audio Quality
- **Sample Rate**: 44100 Hz (default)
- **Bit Depth**: 16-bit signed
- **Channels**: Stereo
- **Buffer Size**: 4096 bytes (DMA)

## Customization

### Changing Button Pins
Edit `src/buttons/buttons.c` - modify the `buttons[]` array:
```c
static button_config_t buttons[NUM_BUTTONS] = {
    {GPIOB, GPIO_PIN_0, BTN_PREVIOUS, ...},  // Change GPIO_PIN_0 as needed
    // ... more buttons
};
```

### Changing LCD Display Dimensions
If using a different LCD (e.g., 320x480):
1. Edit `src/lcd/lcd_display.h`:
   ```c
   #define LCD_WIDTH  320
   #define LCD_HEIGHT 480
   ```
2. Adjust button layout in `lcd_display.c`

### Custom Colors
Edit color definitions in `src/lcd/lcd_display.h`:
```c
#define COLOR_DARK_GREEN  0x0320   // RGB565 format
#define COLOR_GREEN       0x07E0
```

## Performance Considerations

### CPU Usage
- Button polling: ~1% per 100ms
- LCD updates: ~5% at 10Hz refresh
- Audio DMA: minimal, interrupt-driven

### Memory Usage
- Code: ~40KB
- Audio buffer: 4KB
- Playlist cache: configurable (100 files × 256 bytes = 25KB)
- **Total RAM**: ~70KB

### Power Consumption
- Idle (display on): ~50mA
- Playing audio: ~100mA
- Peak (USB charging + playback): ~500mA

## Troubleshooting

### No Sound Output
1. Check I2S pins and codec connections
2. Verify codec is initialized via I2C
3. Check audio file format compatibility
4. Confirm SD card is mounted

### Display Shows No Text
1. Check SPI pins and LCD reset sequence
2. Verify LCD supply voltage (3.3V)
3. Check CS, DC, RST pin connections
4. Try lcd_reset() in debugger

### Buttons Not Responding
1. Check GPIO pin configuration (input with pull-up)
2. Verify button press lowers GPIO to ground
3. Check debounce timing (20ms default)
4. Test with buttons_is_pressed() in debugger

## Future Enhancements

- [ ] SD card file browser with pagination
- [ ] Album art display
- [ ] EQ settings (bass, treble, loudness)
- [ ] Bluetooth audio streaming
- [ ] RTC for track duration display
- [ ] Low battery warning
- [ ] USB mass storage mode
- [ ] Recording capability

## Notes

- This implementation assumes a standard STM32F4 board; adjust HAL initialization for your specific board
- The I2S audio output requires an external codec chip (WM8994, CS4344, etc.)
- SD card support requires SDIO/MMC HAL driver implementation
- All button debouncing is done in software (20ms default)

## License

This project is provided as-is for educational and embedded systems use.
