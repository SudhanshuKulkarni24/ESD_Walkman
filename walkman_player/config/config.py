# Walkman Player - Configuration
# Copy to config/config.py and modify as needed

# GPIO Configuration for DragonBoard 410c
GPIO_CONFIG = {
    'mode': 'BCM',  # BCM or BOARD
    'pins': {
        'play_pause': 36,      # GPIO_L8 (GPIO52)
        'next': 26,            # GPIO_L3 (GPIO47)
        'prev': 24,            # GPIO_L1 (GPIO45)
        'vol_up': 32,          # GPIO_L6 (GPIO50)
        'vol_down': 30,        # GPIO_L4 (GPIO48)
    },
    'debounce_ms': 200,
}

# Audio Configuration
AUDIO_CONFIG = {
    'supported_formats': {'.mp3', '.wav', '.ogg', '.flac'},
    'default_volume': 0.7,  # 0.0 to 1.0
    'volume_step': 0.1,     # Volume increase/decrease step
}

# UI Configuration
UI_CONFIG = {
    'window_width': 600,
    'window_height': 700,
    'theme': 'dark',  # 'dark' or 'light'
}

# Application Configuration
APP_CONFIG = {
    'use_gpio_simulator': True,  # Set to False when deploying to DragonBoard
    'debug': False,
    'auto_play_on_load': True,
}
