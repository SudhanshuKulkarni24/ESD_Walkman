# Walkman Music Player

A Python-based music player application inspired by classic Walkman devices. Designed for desktop use with GUI and built to support embedded systems like the DragonBoard 410c with GPIO button controls.

## Features

- 🎵 **Local Music File Loading** - Load individual files or entire directories
- ⏯️ **Playback Control** - Play, pause, stop, next, previous
- 🔊 **Volume Control** - Adjust volume with keyboard/mouse or GPIO buttons
- 🎚️ **Playlist Management** - Support for multiple audio formats
- 🎛️ **GPIO Integration** - Hardware button support for embedded systems
- 🖥️ **Desktop GUI** - User-friendly tkinter interface with simulator mode
- 📱 **Embedded-Ready** - Designed for DragonBoard 410c and similar boards

## Supported Audio Formats

- MP3 (.mp3) - Common format, variable compatibility
- WAV (.wav) - Best compatibility
- OGG Vorbis (.ogg) - Good compatibility, smaller files
- FLAC (.flac) - Lossless, good compression

**Note**: If you experience "corrupt MP3 file" errors, see `AUDIO_COMPATIBILITY.md` for solutions. Most MP3 compatibility issues can be resolved by converting to WAV or OGG format.

## Requirements

### Desktop (Development/Testing)

- Python 3.7+
- pygame (for audio playback)
- tkinter (usually included with Python)

### Embedded System (DragonBoard 410c)

- Python 3.7+
- pygame
- RPi.GPIO (or equivalent GPIO library)

## Installation

### Desktop Setup

```bash
# Clone or navigate to project directory
cd walkman_player

# Install dependencies
pip install -r requirements.txt

# Run the application
python main.py
```

### Audio Format Issues?

If you see "corrupt MP3 file" errors:

1. **Read the guide**: See `AUDIO_COMPATIBILITY.md`
2. **Convert files**: Use the included converter tool
   ```bash
   python convert_audio.py
   ```
3. **Use WAV or OGG**: Most reliable formats

The converter tool makes it easy to batch convert MP3s to WAV or OGG format.

### DragonBoard 410c Setup

1. Install Python and required packages:
```bash
sudo apt-get update
sudo apt-get install python3 python3-pip
pip3 install -r requirements.txt
```

2. Modify GPIO configuration:
   - Edit `config/config.py` with actual pin numbers for your buttons
   - Set `APP_CONFIG['use_gpio_simulator']` to `False`

3. Run the application:
```bash
python3 main.py
```

## Usage

### Desktop GUI

1. **Load Music**:
   - Click "Load Folder" to load all music files from a directory
   - Click "Load File" to add individual tracks

2. **Playback Controls**:
   - Click buttons or use GPIO simulators
   - Adjust volume with the slider or GPIO buttons

3. **GPIO Simulation**:
   - Use the "GPIO Simulation" buttons to test hardware button mappings
   - Press buttons: Play/Pause, Next, Previous, Volume Up/Down

### Keyboard/Mouse Controls

- **Load Folder**: Click "Load Folder" button
- **Load File**: Click "Load File" button
- **Play/Pause**: Click button or use GPIO
- **Next/Previous**: Click buttons or use GPIO
- **Volume**: Use slider or GPIO buttons

## Project Structure

```
walkman_player/
├── main.py                 # Entry point
├── requirements.txt        # Python dependencies
├── README.md              # This file
├── DRAGONBOARD_SETUP.md   # DragonBoard-specific setup guide
├── config/
│   └── config.py          # Configuration settings
├── src/
│   ├── core/
│   │   └── player.py      # Core music player engine
│   ├── gpio/
│   │   └── controller.py  # GPIO control module
│   └── ui/
│       └── gui.py         # Desktop GUI (tkinter)
```

## GPIO Pin Configuration

### DragonBoard 410c Default Mapping

| Function    | GPIO Pin | Signal |
|------------|----------|--------|
| Play/Pause | 36       | GPIO_L8 (GPIO52) |
| Next       | 26       | GPIO_L3 (GPIO47) |
| Previous   | 24       | GPIO_L1 (GPIO45) |
| Volume Up  | 32       | GPIO_L6 (GPIO50) |
| Volume Down| 30       | GPIO_L4 (GPIO48) |

**Note**: Pins can be customized in `config/config.py`

## Hardware Setup (DragonBoard 410c)

### Required Hardware

- DragonBoard 410c
- 5 Momentary Push Buttons (normally open)
- 5 × 10kΩ Pull-up Resistors (optional, can use internal)
- Breadboard and Jumper Wires
- USB Sound Card or HDMI Audio Output

### Wiring Example (with Pull-up Resistors)

```
Button 1 (Play/Pause)
  ├─ One end to GPIO pin 36 (GPIO_L8)
  └─ Other end to GND
  
Pull-up Resistor
  ├─ One end to GPIO pin 36
  └─ Other end to 3.3V

[Repeat for other buttons]
```

## API Reference

### MusicPlayer Class

```python
from src.core.player import MusicPlayer

player = MusicPlayer()

# Load music
player.load_music_directory('/path/to/music')
player.add_file('/path/to/song.mp3')

# Playback control
player.play()
player.pause()
player.stop()
player.next_track()
player.prev_track()

# Volume control
player.set_volume(0.7)  # 0.0 to 1.0
player.volume_up(0.1)
player.volume_down(0.1)

# Status
status = player.get_status()
print(f"Playing: {status['current_track']}")
```

### GPIOController Class

```python
from src.gpio.controller import GPIOController

# Desktop mode (simulator)
gpio = GPIOController(use_simulator=True)

# Hardware mode (DragonBoard)
gpio = GPIOController(use_simulator=False)

gpio.setup()
gpio.register_callback('play_pause', my_callback_function)
gpio.start_monitoring()

# Simulate button press (for testing)
gpio.simulate_button_press('play_pause')

gpio.cleanup()
```

## Testing

### Desktop Testing

1. Run the application in simulator mode (default)
2. Use the GUI buttons or GPIO simulation buttons
3. Test with sample music files

### Hardware Testing

1. Connect buttons according to GPIO pin configuration
2. Set `use_gpio_simulator` to `False` in config
3. Run with proper permissions:
```bash
sudo python3 main.py
```

## Troubleshooting

### Common Issues

**Issue**: "RPi.GPIO not available" on desktop
- **Solution**: This is normal. The app falls back to simulator mode.

**Issue**: Audio not playing
- **Solution**: Ensure pygame mixer initialized correctly. Check system audio settings.

**Issue**: Buttons not responding on DragonBoard
- **Solution**: Verify GPIO pin numbers in config. Check button wiring and pull-up resistors.

**Issue**: File not loading
- **Solution**: Ensure file format is supported and file permissions are readable.

## Development Notes

### Adding New Audio Formats

Edit `src/core/player.py`, update `supported_formats` set:
```python
self.supported_formats = {'.mp3', '.wav', '.ogg', '.flac', '.m4a'}
```

### Custom GPIO Callbacks

```python
def my_button_handler():
    print("Button pressed!")

gpio.register_callback('play_pause', my_button_handler)
```

### Extending the GUI

Edit `src/ui/gui.py` to add new buttons or controls to the tkinter interface.

## Performance Considerations

- Audio playback is non-blocking (runs in background)
- GPIO monitoring runs in separate thread
- Debounce delay: 200ms (adjustable in config)
- Update frequency: 500ms for GUI status

## Future Enhancements

- [ ] Shuffle and repeat modes
- [ ] EQ/Bass controls
- [ ] Display album art
- [ ] Seek/progress bar
- [ ] Playlist editing UI
- [ ] LCD/OLED display support for DragonBoard
- [ ] Battery level monitoring
- [ ] Sleep timer

## License

MIT License - Feel free to use and modify for your projects

## Support

For issues, questions, or contributions, please refer to the project repository.

---

**Created for**: Embedded Systems Development (ESD)
**Target Device**: DragonBoard 410c (with desktop compatibility)
