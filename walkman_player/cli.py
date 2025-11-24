"""
Walkman Music Player - CLI Version
For headless operation (DragonBoard, SSH, etc.)
"""

import sys
import os
import time
from pathlib import Path
from threading import Event

# Add src to path
sys.path.insert(0, str(Path(__file__).parent / 'src'))

from core.player import MusicPlayer
from gpio.controller import GPIOController, RemoteControlHandler


class CLIPlayer:
    """Command-line interface for the music player."""

    def __init__(self, use_gpio: bool = False):
        """
        Initialize CLI player.
        
        Args:
            use_gpio: Whether to use real GPIO (False for simulator)
        """
        self.player = MusicPlayer()
        self.gpio = GPIOController(use_simulator=not use_gpio)
        self.remote = RemoteControlHandler(self.player, self.gpio)
        self.running = True
        self.last_display = 0

    def setup(self) -> bool:
        """Setup player and GPIO."""
        print("\n" + "=" * 60)
        print("Walkman Music Player - CLI Edition")
        print("=" * 60)
        
        if self.remote.setup():
            print("[✓] Player initialized and ready")
            return True
        else:
            print("[✗] Failed to initialize player")
            return False

    def print_menu(self):
        """Print command menu."""
        print("\n" + "-" * 60)
        print("Commands:")
        print("  help      - Show this menu")
        print("  load      - Load folder or file")
        print("  play      - Play current or resume")
        print("  pause     - Pause playback")
        print("  stop      - Stop playback")
        print("  next      - Next track")
        print("  prev      - Previous track")
        print("  shuffle   - Toggle shuffle mode")
        print("  loop      - Cycle loop mode (off → all → one)")
        print("  list      - List all tracks")
        print("  status    - Show player status")
        print("  vol [0-100] - Set volume (default shows current)")
        print("  exit      - Exit application")
        print("-" * 60)

    def display_status(self):
        """Display current player status."""
        status = self.player.get_status()
        
        if status['is_playing']:
            play_str = "▶ PLAYING"
            if status['is_paused']:
                play_str = "⏸ PAUSED"
        else:
            play_str = "⏹ STOPPED"

        current_track = status['current_track'] or "None"
        volume_pct = int(status['volume'] * 100)
        
        shuffle_status = "🔀 ON" if self.player.shuffle_enabled else "OFF"
        loop_status = self.player.loop_mode.upper()

        print(f"\n{play_str:15} | Track: {status['current_index'] + 1}/{status['playlist_size']}")
        print(f"Volume: {volume_pct:3}%       | {current_track}")
        print(f"Shuffle: {shuffle_status:10} | Loop: {loop_status}")
        print("-" * 60)

    def load_music(self):
        """Load music from user input."""
        path = input("Enter path to folder or file: ").strip()
        if not path:
            print("Cancelled.")
            return

        path = os.path.expanduser(path)
        
        if os.path.isdir(path):
            count = self.player.load_music_directory(path)
            print(f"Loaded {count} tracks")
        elif os.path.isfile(path):
            if self.player.add_file(path):
                print("File added to playlist")
            else:
                print("Failed to add file")
        else:
            print("Path not found")

    def list_tracks(self):
        """List all tracks in playlist."""
        playlist = self.player.get_playlist()
        
        if not playlist:
            print("No tracks in playlist")
            return

        print("\nPlaylist:")
        print("-" * 60)
        current = self.player.current_index
        
        for i, track in enumerate(playlist):
            marker = "▶ " if i == current else "  "
            filename = os.path.basename(track)
            print(f"{marker}{i + 1:3}. {filename}")
        
        print("-" * 60)

    def set_volume(self, level_str: str = None):
        """Set volume level."""
        if level_str is None:
            vol_pct = int(self.player.volume * 100)
            print(f"Current volume: {vol_pct}%")
            return

        try:
            level = int(level_str)
            if 0 <= level <= 100:
                self.player.set_volume(level / 100.0)
                print(f"Volume set to {level}%")
            else:
                print("Volume must be 0-100")
        except ValueError:
            print("Invalid volume value")

    def run_interactive(self):
        """Run interactive CLI mode."""
        self.print_menu()
        
        while self.running:
            try:
                self.display_status()
                cmd = input("Enter command > ").strip().lower().split()
                
                if not cmd:
                    continue

                command = cmd[0]
                args = cmd[1:] if len(cmd) > 1 else []

                if command == 'help':
                    self.print_menu()
                elif command == 'load':
                    self.load_music()
                elif command == 'play':
                    self.player.play()
                    print("Playing...")
                elif command == 'pause':
                    if self.player.pause():
                        print("Paused")
                    else:
                        print("Not playing")
                elif command == 'stop':
                    self.player.stop()
                    print("Stopped")
                elif command == 'next':
                    if self.player.next_track():
                        print(f"Next: {self.player.get_current_track_name()}")
                    else:
                        print("End of playlist")
                elif command == 'prev':
                    if self.player.prev_track():
                        print(f"Previous: {self.player.get_current_track_name()}")
                    else:
                        print("Beginning of playlist")
                elif command == 'shuffle':
                    enabled = self.player.toggle_shuffle()
                    status = "ON" if enabled else "OFF"
                    print(f"Shuffle: {status}")
                elif command == 'loop':
                    mode = self.player.toggle_loop()
                    print(f"Loop mode: {mode.upper()}")
                elif command == 'list':
                    self.list_tracks()
                elif command == 'status':
                    print("\nDetailed Status:")
                    status = self.player.get_status()
                    for key, value in status.items():
                        print(f"  {key}: {value}")
                elif command == 'vol':
                    self.set_volume(args[0] if args else None)
                elif command == 'exit':
                    self.running = False
                    print("Exiting...")
                else:
                    print(f"Unknown command: {command}. Type 'help' for commands.")

            except KeyboardInterrupt:
                print("\n\nExiting...")
                self.running = False
            except Exception as e:
                print(f"Error: {e}")

    def run_headless(self, music_folder: str = None):
        """
        Run in headless mode with GPIO control only.
        
        Args:
            music_folder: Optional initial music folder to load
        """
        print("[INFO] Running in headless mode")
        print("[INFO] Control via GPIO buttons or press Ctrl+C to exit")
        
        if music_folder:
            count = self.player.load_music_directory(music_folder)
            if count > 0:
                print(f"[INFO] Loaded {count} tracks")
                self.player.play()
        
        try:
            while True:
                time.sleep(1)
                if self.last_display + 5 < time.time():
                    self.display_status()
                    self.last_display = time.time()
        except KeyboardInterrupt:
            print("\n[INFO] Shutting down...")

    def cleanup(self):
        """Cleanup resources."""
        self.remote.cleanup()
        self.player.shutdown()
        print("[INFO] Cleanup complete")


def main():
    """Main entry point."""
    import argparse
    
    parser = argparse.ArgumentParser(description='Walkman Music Player - CLI')
    parser.add_argument('--headless', action='store_true',
                      help='Run in headless mode (GPIO control only)')
    parser.add_argument('--gpio', action='store_true',
                      help='Use real GPIO (requires root on hardware)')
    parser.add_argument('--folder', type=str,
                      help='Auto-load music folder on startup')
    
    args = parser.parse_args()
    
    cli = CLIPlayer(use_gpio=args.gpio)
    
    try:
        if not cli.setup():
            sys.exit(1)
        
        if args.headless:
            cli.run_headless(args.folder)
        else:
            cli.run_interactive()
    finally:
        cli.cleanup()


if __name__ == "__main__":
    main()
