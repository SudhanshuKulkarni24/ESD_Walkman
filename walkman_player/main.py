"""
Walkman Music Player - Main entry point
Desktop GUI application with GPIO support for embedded systems
"""

import sys
import tkinter as tk
from pathlib import Path

# Add src to path
sys.path.insert(0, str(Path(__file__).parent / 'src'))

from core.player import MusicPlayer
from gpio.controller import GPIOController, RemoteControlHandler
from ui.gui import WalkmanGUI


def main():
    """Main entry point for the application."""
    
    print("=" * 50)
    print("Walkman Music Player")
    print("=" * 50)
    
    # Initialize music player
    print("\n[INFO] Initializing music player...")
    player = MusicPlayer()
    
    # Initialize GPIO controller (use simulator for desktop testing)
    print("[INFO] Initializing GPIO controller (simulator mode for desktop)...")
    gpio_controller = GPIOController(use_simulator=True)
    
    # Initialize remote control handler
    print("[INFO] Setting up remote control...")
    remote_handler = RemoteControlHandler(player, gpio_controller)
    if remote_handler.setup():
        print("[INFO] Remote control ready")
    else:
        print("[WARNING] Remote control setup failed")
    
    # Create and run GUI
    print("[INFO] Starting GUI...")
    root = tk.Tk()
    gui = WalkmanGUI(root, player, gpio_controller)
    
    def on_closing():
        """Handle window closing."""
        print("\n[INFO] Shutting down...")
        gui.shutdown()
        remote_handler.cleanup()
        root.destroy()
    
    root.protocol("WM_DELETE_WINDOW", on_closing)
    
    print("[INFO] Ready! Load music and enjoy.")
    print("=" * 50 + "\n")
    
    gui.run()


if __name__ == "__main__":
    main()
