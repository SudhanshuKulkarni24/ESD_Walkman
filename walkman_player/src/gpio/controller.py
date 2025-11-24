"""
GPIO control module for DragonBoard 410c and other embedded systems.
Supports button input for play/pause, volume control, track navigation.
"""

import threading
import time
from typing import Callable, Dict, Optional


class GPIOButton:
    """Represents a single GPIO button with debouncing."""

    def __init__(self, pin: int, callback: Callable, mode: str = "BCM"):
        """
        Initialize a GPIO button.
        
        Args:
            pin: GPIO pin number
            callback: Function to call when button is pressed
            mode: GPIO mode ('BCM' or 'BOARD')
        """
        self.pin = pin
        self.callback = callback
        self.mode = mode
        self.gpio = None
        self._setup_complete = False

    def setup(self, gpio_lib) -> bool:
        """Setup the GPIO pin."""
        try:
            self.gpio = gpio_lib
            self.gpio.setmode(self.mode)
            self.gpio.setup(self.pin, self.gpio.IN, pull_up_down=self.gpio.PUD_UP)
            self._setup_complete = True
            return True
        except Exception as e:
            print(f"Error setting up GPIO pin {self.pin}: {e}")
            return False

    def is_pressed(self) -> bool:
        """Check if the button is pressed."""
        if not self._setup_complete or self.gpio is None:
            return False
        try:
            return self.gpio.input(self.pin) == self.gpio.LOW
        except:
            return False

    def cleanup(self):
        """Cleanup GPIO pin."""
        if self._setup_complete and self.gpio:
            try:
                self.gpio.cleanup(self.pin)
            except:
                pass
            self._setup_complete = False


class GPIOControllerSimulator:
    """
    GPIO controller that simulates hardware buttons.
    Used for desktop testing without physical GPIO hardware.
    """

    def __init__(self):
        """Initialize the GPIO simulator."""
        self.callbacks: Dict[str, Callable] = {}
        self._running = False

    def register_button(self, button_name: str, callback: Callable):
        """Register a button with a callback function."""
        self.callbacks[button_name] = callback

    def simulate_button_press(self, button_name: str):
        """Simulate a button press (for testing)."""
        if button_name in self.callbacks:
            self.callbacks[button_name]()
            print(f"[GPIO Simulator] Button pressed: {button_name}")

    def start(self):
        """Start the GPIO simulator."""
        self._running = True

    def stop(self):
        """Stop the GPIO simulator."""
        self._running = False


class GPIOController:
    """
    Hardware GPIO controller for DragonBoard 410c and similar boards.
    Handles button input with debouncing and threading.
    """

    # Default GPIO pin configuration for DragonBoard 410c
    DEFAULT_PIN_CONFIG = {
        'play_pause': 36,      # GPIO_L8 (GPIO52)
        'next': 26,            # GPIO_L3 (GPIO47)
        'prev': 24,            # GPIO_L1 (GPIO45)
        'vol_up': 32,          # GPIO_L6 (GPIO50)
        'vol_down': 30,        # GPIO_L4 (GPIO48)
        'shuffle': 28,         # GPIO_L2 (GPIO46)
        'loop': 25,            # GPIO_L0 (GPIO44)
    }

    def __init__(self, use_simulator: bool = False, pin_config: Optional[Dict] = None):
        """
        Initialize GPIO controller.
        
        Args:
            use_simulator: If True, use simulator instead of real GPIO
            pin_config: Custom pin configuration dictionary
        """
        self.use_simulator = use_simulator
        self.pin_config = pin_config or self.DEFAULT_PIN_CONFIG
        self.buttons: Dict[str, GPIOButton] = {}
        self.callbacks: Dict[str, Callable] = {}
        self._gpio_lib = None
        self._monitoring = False
        self._monitor_thread: Optional[threading.Thread] = None
        self._debounce_time = 0.2  # 200ms debounce
        self._last_press_time: Dict[str, float] = {}

        if use_simulator:
            self._gpio_controller = GPIOControllerSimulator()
        else:
            self._gpio_controller = None

    def setup(self) -> bool:
        """
        Initialize GPIO hardware.
        
        Returns:
            True if setup successful, False otherwise
        """
        if self.use_simulator:
            print("[GPIO] Using GPIO simulator mode")
            self._gpio_controller.start()
            return True

        try:
            import RPi.GPIO as GPIO
            self._gpio_lib = GPIO
            
            # Setup all buttons
            for button_name, pin in self.pin_config.items():
                button = GPIOButton(pin, self._on_button_press)
                if button.setup(self._gpio_lib):
                    self.buttons[button_name] = button
                    self._last_press_time[button_name] = 0
                    print(f"[GPIO] {button_name} button setup on pin {pin}")
                else:
                    print(f"[GPIO] Failed to setup {button_name} button")

            print("[GPIO] Hardware GPIO initialized")
            return True
        except ImportError:
            print("[GPIO] RPi.GPIO not available. Falling back to simulator mode.")
            self.use_simulator = True
            self._gpio_controller = GPIOControllerSimulator()
            self._gpio_controller.start()
            return True
        except Exception as e:
            print(f"[GPIO] Error initializing GPIO: {e}")
            return False

    def register_callback(self, button_name: str, callback: Callable) -> bool:
        """
        Register a callback function for a button.
        
        Args:
            button_name: Name of the button
            callback: Function to call when button is pressed
            
        Returns:
            True if callback registered successfully
        """
        if button_name not in self.pin_config:
            print(f"Unknown button: {button_name}")
            return False

        self.callbacks[button_name] = callback
        
        if self.use_simulator:
            self._gpio_controller.register_button(button_name, callback)

        return True

    def start_monitoring(self):
        """Start monitoring GPIO buttons."""
        if self._monitoring:
            return

        self._monitoring = True
        self._monitor_thread = threading.Thread(target=self._monitor_buttons, daemon=True)
        self._monitor_thread.start()
        print("[GPIO] Button monitoring started")

    def stop_monitoring(self):
        """Stop monitoring GPIO buttons."""
        self._monitoring = False
        if self._monitor_thread:
            self._monitor_thread.join(timeout=1.0)
        print("[GPIO] Button monitoring stopped")

    def _monitor_buttons(self):
        """Monitor buttons for presses."""
        while self._monitoring:
            for button_name, button in self.buttons.items():
                if button.is_pressed():
                    current_time = time.time()
                    last_press = self._last_press_time.get(button_name, 0)
                    
                    # Debounce check
                    if current_time - last_press > self._debounce_time:
                        self._last_press_time[button_name] = current_time
                        self._on_button_press(button_name)

            time.sleep(0.05)  # Check every 50ms

    def _on_button_press(self, button_name: str):
        """Handle button press."""
        if button_name in self.callbacks:
            callback = self.callbacks[button_name]
            callback()
            print(f"[GPIO] {button_name} pressed")

    def simulate_button_press(self, button_name: str):
        """
        Simulate a button press (for testing).
        
        Args:
            button_name: Name of the button to simulate
        """
        if self.use_simulator and button_name in self.callbacks:
            self.callbacks[button_name]()
            print(f"[GPIO Simulator] {button_name} pressed")

    def cleanup(self):
        """Clean up GPIO resources."""
        self.stop_monitoring()
        
        for button in self.buttons.values():
            button.cleanup()

        if not self.use_simulator and self._gpio_lib:
            try:
                self._gpio_lib.cleanup()
            except:
                pass

        print("[GPIO] Cleanup complete")


class RemoteControlHandler:
    """High-level handler for player control via GPIO."""

    def __init__(self, player, gpio_controller: GPIOController):
        """
        Initialize remote control handler.
        
        Args:
            player: MusicPlayer instance
            gpio_controller: GPIOController instance
        """
        self.player = player
        self.gpio = gpio_controller

    def setup(self) -> bool:
        """Setup remote control handlers."""
        if not self.gpio.setup():
            return False

        # Register callbacks
        self.gpio.register_callback('play_pause', self._on_play_pause)
        self.gpio.register_callback('next', self._on_next)
        self.gpio.register_callback('prev', self._on_prev)
        self.gpio.register_callback('vol_up', self._on_vol_up)
        self.gpio.register_callback('vol_down', self._on_vol_down)
        self.gpio.register_callback('shuffle', self._on_shuffle)
        self.gpio.register_callback('loop', self._on_loop)

        self.gpio.start_monitoring()
        return True

    def _on_play_pause(self):
        """Handle play/pause button."""
        if self.player.is_playing and not self.player.is_paused:
            self.player.pause()
            print("Paused")
        else:
            self.player.play()
            print("Playing")

    def _on_next(self):
        """Handle next button."""
        if self.player.next_track():
            print(f"Next: {self.player.get_current_track_name()}")

    def _on_prev(self):
        """Handle previous button."""
        if self.player.prev_track():
            print(f"Previous: {self.player.get_current_track_name()}")

    def _on_vol_up(self):
        """Handle volume up button."""
        vol = self.player.volume_up(0.1)
        print(f"Volume: {vol:.0%}")

    def _on_vol_down(self):
        """Handle volume down button."""
        vol = self.player.volume_down(0.1)
        print(f"Volume: {vol:.0%}")

    def _on_shuffle(self):
        """Handle shuffle button."""
        enabled = self.player.toggle_shuffle()
        status = "ON" if enabled else "OFF"
        print(f"Shuffle: {status}")

    def _on_loop(self):
        """Handle loop button."""
        mode = self.player.toggle_loop()
        mode_names = {'off': 'OFF', 'all': 'ALL', 'one': 'ONE'}
        print(f"Loop: {mode_names[mode]}")

    def cleanup(self):
        """Cleanup remote control resources."""
        self.gpio.cleanup()
