"""
Core music player engine using pygame for audio playback.
Supports local music file loading and playback control.
"""

import pygame
import os
from pathlib import Path
from typing import List, Optional, Callable
import threading
import time
import random


class MusicPlayer:
    """Core music player with playlist management and playback control."""

    def __init__(self):
        """Initialize the music player."""
        pygame.mixer.init(frequency=44100, size=-16, channels=2, buffer=512)
        self.playlist: List[str] = []
        self.current_index: int = -1
        self.is_playing: bool = False
        self.is_paused: bool = False
        self.volume: float = 0.7  # Default volume (0.0 to 1.0)
        self.supported_formats = {'.mp3', '.wav', '.ogg', '.flac'}
        self.failed_files: List[str] = []  # Track files that failed to load
        self.playable_files: List[str] = []  # Track files that are playable
        
        # Playback modes
        self.shuffle_enabled: bool = False
        self.loop_mode: str = 'off'  # 'off', 'all', 'one'
        self.shuffle_order: List[int] = []  # Shuffled playlist indices
        
        # Playback time tracking
        self.playback_start_time: float = 0.0  # Time when track started playing
        self.playback_pause_time: float = 0.0  # Accumulated pause time
        
        # Callback functions
        self.on_track_changed: Optional[Callable] = None
        self.on_playback_started: Optional[Callable] = None
        self.on_playback_stopped: Optional[Callable] = None
        self.on_playlist_updated: Optional[Callable] = None
        self.on_shuffle_changed: Optional[Callable] = None
        self.on_loop_changed: Optional[Callable] = None
        
        # Monitoring thread
        self._monitor_thread = None
        self._stop_monitor = False

    def load_music_directory(self, directory: str) -> int:
        """
        Load all supported audio files from a directory.
        
        Args:
            directory: Path to the directory containing music files
            
        Returns:
            Number of files loaded
        """
        directory_path = Path(directory)
        if not directory_path.is_dir():
            print(f"Error: {directory} is not a valid directory")
            return 0

        self.playlist.clear()
        self.failed_files.clear()
        self.playable_files.clear()
        self.shuffle_order.clear()  # Reset shuffle order
        
        for file in sorted(directory_path.iterdir()):
            if file.is_file() and file.suffix.lower() in self.supported_formats:
                if self._validate_audio_file(str(file)):
                    self.playlist.append(str(file))
                    self.playable_files.append(str(file))
                else:
                    self.failed_files.append(str(file))

        if self.on_playlist_updated:
            self.on_playlist_updated(len(self.playlist))

        if len(self.failed_files) > 0:
            print(f"⚠ Warning: {len(self.failed_files)} files failed to load (corrupted or unsupported format)")
        
        print(f"✓ Loaded {len(self.playlist)} playable music files from {directory}")
        return len(self.playlist)

    def add_file(self, filepath: str) -> bool:
        """
        Add a single music file to the playlist.
        
        Args:
            filepath: Path to the music file
            
        Returns:
            True if successfully added, False otherwise
        """
        file_path = Path(filepath)
        if not file_path.is_file():
            print(f"Error: {filepath} is not a valid file")
            return False

        if file_path.suffix.lower() not in self.supported_formats:
            print(f"Error: {file_path.suffix} is not a supported format")
            return False

        if not self._validate_audio_file(str(filepath)):
            print(f"Error: File is corrupted or not a valid audio file")
            return False

        self.playlist.append(str(filepath))
        self.playable_files.append(str(filepath))
        if self.on_playlist_updated:
            self.on_playlist_updated(len(self.playlist))
        
        return True

    def _validate_audio_file(self, filepath: str) -> bool:
        """
        Validate if an audio file can be loaded.
        
        Args:
            filepath: Path to the audio file
            
        Returns:
            True if file is valid, False otherwise
        """
        try:
            # Try to test load the file
            test_sound = pygame.mixer.Sound(filepath)
            return True
        except pygame.error as e:
            # Log but don't print - validation is silent
            return False
        except Exception as e:
            return False

    def play(self, index: Optional[int] = None) -> bool:
        """
        Play music from the playlist.
        
        Args:
            index: Index of the track to play. If None, resume from paused state or play from start
            
        Returns:
            True if playback started, False otherwise
        """
        if index is not None:
            if 0 <= index < len(self.playlist):
                self.current_index = index
            else:
                print(f"Error: Invalid index {index}")
                return False

        if self.is_paused:
            pygame.mixer.music.unpause()
            self.is_paused = False
            self.is_playing = True
            self.playback_start_time = time.time()  # Reset timer when resuming
            return True

        if self.current_index < 0 or self.current_index >= len(self.playlist):
            if len(self.playlist) > 0:
                self.current_index = 0
            else:
                print("Error: No music files in playlist")
                return False

        # Try to play the current track, skip if corrupted
        max_retries = len(self.playlist)
        attempts = 0
        
        while attempts < max_retries:
            try:
                track_path = self.playlist[self.current_index]
                pygame.mixer.music.load(track_path)
                pygame.mixer.music.set_volume(self.volume)
                pygame.mixer.music.play()
                self.is_playing = True
                self.is_paused = False
                self.playback_start_time = time.time()  # Record when playback started
                
                if self.on_track_changed:
                    self.on_track_changed(self.current_index, track_path)
                if self.on_playback_started:
                    self.on_playback_started()
                
                # Start monitoring thread if not already running
                if self._monitor_thread is None or not self._monitor_thread.is_alive():
                    self._stop_monitor = False
                    self._monitor_thread = threading.Thread(target=self._monitor_playback, daemon=True)
                    self._monitor_thread.start()
                
                return True
            except pygame.error as e:
                # File is corrupted, try next
                track_name = os.path.basename(self.playlist[self.current_index])
                print(f"⚠ Skipping corrupted file: {track_name}")
                
                # Move to next track and retry
                if not self.next_track():
                    # End of playlist reached
                    self.is_playing = False
                    if self.on_playback_stopped:
                        self.on_playback_stopped()
                    return False
                
                attempts += 1
            except Exception as e:
                print(f"Error: {e}")
                return False
        
        # All files failed to load
        print("Error: Unable to play any tracks - all files are corrupted or incompatible")
        return False

    def pause(self) -> bool:
        """
        Pause the current playback.
        
        Returns:
            True if paused successfully, False otherwise
        """
        if self.is_playing and not self.is_paused:
            pygame.mixer.music.pause()
            self.is_paused = True
            return True
        return False

    def unpause(self) -> bool:
        """
        Resume playback from pause.
        
        Returns:
            True if resumed successfully, False otherwise
        """
        if self.is_playing and self.is_paused:
            pygame.mixer.music.unpause()
            self.is_paused = False
            return True
        return False

    def stop(self) -> bool:
        """
        Stop the current playback.
        
        Returns:
            True if stopped successfully, False otherwise
        """
        if self.is_playing:
            pygame.mixer.music.stop()
            self.is_playing = False
            self.is_paused = False
            self._stop_monitor = True
            
            if self.on_playback_stopped:
                self.on_playback_stopped()
            
            return True
        return False

    def next_track(self) -> bool:
        """
        Play the next track in the playlist.
        Respects shuffle and loop modes.
        
        Returns:
            True if next track playing, False if at end of playlist
        """
        next_index = self._get_next_track_index()
        
        if next_index is not None:
            return self.play(next_index)
        else:
            # End of playlist reached
            if self.loop_mode == 'one':
                # Loop single track
                return self.play(self.current_index)
            return False

    def prev_track(self) -> bool:
        """
        Play the previous track in the playlist.
        
        Returns:
            True if previous track playing, False if at beginning of playlist
        """
        if self.current_index > 0:
            return self.play(self.current_index - 1)
        return False

    def set_volume(self, volume: float) -> bool:
        """
        Set the volume level.
        
        Args:
            volume: Volume level from 0.0 to 1.0
            
        Returns:
            True if volume set successfully
        """
        volume = max(0.0, min(1.0, volume))
        self.volume = volume
        pygame.mixer.music.set_volume(self.volume)
        return True

    def volume_up(self, step: float = 0.1) -> float:
        """
        Increase volume by a step.
        
        Args:
            step: Volume increase step (default 0.1)
            
        Returns:
            Current volume level
        """
        self.set_volume(self.volume + step)
        return self.volume

    def volume_down(self, step: float = 0.1) -> float:
        """
        Decrease volume by a step.
        
        Args:
            step: Volume decrease step (default 0.1)
            
        Returns:
            Current volume level
        """
        self.set_volume(self.volume - step)
        return self.volume

    def get_current_track(self) -> Optional[str]:
        """Get the currently playing or selected track path."""
        if 0 <= self.current_index < len(self.playlist):
            return self.playlist[self.current_index]
        return None

    def get_current_track_name(self) -> Optional[str]:
        """Get the filename of the currently playing track."""
        track = self.get_current_track()
        if track:
            return os.path.basename(track)
        return None

    def get_playlist(self) -> List[str]:
        """Get the current playlist."""
        return self.playlist.copy()

    def get_status(self) -> dict:
        """Get current player status."""
        return {
            'is_playing': self.is_playing,
            'is_paused': self.is_paused,
            'current_index': self.current_index,
            'current_track': self.get_current_track_name(),
            'volume': self.volume,
            'playlist_size': len(self.playlist),
            'shuffle_enabled': self.shuffle_enabled,
            'loop_mode': self.loop_mode,
        }

    def get_playback_position(self) -> float:
        """
        Get current playback position in seconds.
        Uses pygame position when available, falls back to time-based calculation.
        
        Returns:
            Position in seconds, or 0 if not playing
        """
        if not self.is_playing or self.is_paused:
            return 0.0
        
        # Try to get position from pygame first
        try:
            pos_ms = pygame.mixer.music.get_pos()
            # pygame returns -1 when music is stopped or hasn't started
            # Use pygame position if it's positive
            if pos_ms > 0:
                return pos_ms / 1000.0  # Convert milliseconds to seconds
        except:
            pass
        
        # Fallback: use time-based calculation
        # Calculate elapsed time from when track started playing
        if self.playback_start_time > 0:
            elapsed = time.time() - self.playback_start_time
            if elapsed > 0:
                return elapsed
        
        return 0.0

    def toggle_shuffle(self) -> bool:
        """
        Toggle shuffle mode on/off.
        
        Returns:
            New shuffle state
        """
        self.shuffle_enabled = not self.shuffle_enabled
        
        if self.shuffle_enabled:
            # Create shuffled order
            self.shuffle_order = list(range(len(self.playlist)))
            random.shuffle(self.shuffle_order)
        else:
            self.shuffle_order = []
        
        if self.on_shuffle_changed:
            self.on_shuffle_changed(self.shuffle_enabled)
        
        return self.shuffle_enabled

    def toggle_loop(self) -> str:
        """
        Toggle loop mode: off → all → one → off
        
        Returns:
            New loop mode ('off', 'all', 'one')
        """
        if self.loop_mode == 'off':
            self.loop_mode = 'all'
        elif self.loop_mode == 'all':
            self.loop_mode = 'one'
        else:
            self.loop_mode = 'off'
        
        mode_names = {'off': 'OFF', 'all': 'ALL', 'one': 'ONE'}
        
        if self.on_loop_changed:
            self.on_loop_changed(self.loop_mode)
        
        return self.loop_mode

    def set_shuffle(self, enabled: bool) -> bool:
        """
        Set shuffle mode explicitly.
        
        Args:
            enabled: True to enable shuffle, False to disable
            
        Returns:
            Current shuffle state
        """
        if self.shuffle_enabled != enabled:
            self.toggle_shuffle()
        return self.shuffle_enabled

    def set_loop(self, mode: str) -> str:
        """
        Set loop mode explicitly.
        
        Args:
            mode: 'off', 'all', or 'one'
            
        Returns:
            Current loop mode
        """
        if mode not in ('off', 'all', 'one'):
            print(f"Invalid loop mode: {mode}")
            return self.loop_mode
        
        if self.loop_mode != mode:
            # Toggle until we reach desired mode
            while self.loop_mode != mode:
                self.toggle_loop()
        
        return self.loop_mode

    def _get_next_track_index(self) -> Optional[int]:
        """
        Get the next track index based on current mode.
        
        Returns:
            Next track index or None if playlist is empty
        """
        if len(self.playlist) == 0:
            return None
        
        if self.shuffle_enabled:
            # Use shuffled order
            if not self.shuffle_order:
                # Recreate shuffle order
                self.shuffle_order = list(range(len(self.playlist)))
                random.shuffle(self.shuffle_order)
            
            # Find current position in shuffle order
            try:
                current_pos = self.shuffle_order.index(self.current_index)
                next_pos = current_pos + 1
                
                if next_pos < len(self.shuffle_order):
                    return self.shuffle_order[next_pos]
                else:
                    # End of shuffled list
                    if self.loop_mode == 'all':
                        # Reshuffle and start over
                        self.shuffle_order = list(range(len(self.playlist)))
                        random.shuffle(self.shuffle_order)
                        return self.shuffle_order[0]
                    return None
            except ValueError:
                # Current index not in shuffle order, start from beginning
                if self.shuffle_order:
                    return self.shuffle_order[0]
        else:
            # Normal sequential order
            next_index = self.current_index + 1
            
            if next_index < len(self.playlist):
                return next_index
            else:
                # End of playlist
                if self.loop_mode == 'all':
                    return 0
                return None

    def _monitor_playback(self):
        """Monitor playback and advance to next track when current ends."""
        while not self._stop_monitor and self.is_playing:
            if not pygame.mixer.music.get_busy() and not self.is_paused:
                # Track finished, handle based on loop mode
                if self.loop_mode == 'one':
                    # Replay current track
                    self.play(self.current_index)
                elif not self.next_track():
                    # End of playlist reached and not in loop mode
                    self.is_playing = False
                    if self.on_playback_stopped:
                        self.on_playback_stopped()
                    break
            time.sleep(0.5)

    def shutdown(self):
        """Clean up and shutdown the player."""
        self._stop_monitor = True
        self.stop()
        pygame.mixer.quit()
