"""
Desktop GUI for the Walkman music player using tkinter.
Modern Spotify/Apple Music-inspired design with sleek interface.
"""

import tkinter as tk
from tkinter import ttk, filedialog, messagebox
from pathlib import Path
import os
from typing import Optional
import pygame


class WalkmanGUI:
    """Desktop GUI for the Walkman music player."""

    def __init__(self, root: tk.Tk, player, gpio_controller=None):
        """
        Initialize the GUI.
        
        Args:
            root: tkinter root window
            player: MusicPlayer instance
            gpio_controller: Optional GPIOController for simulating button presses
        """
        self.root = root
        self.player = player
        self.gpio_controller = gpio_controller
        
        self.root.title("Walkman Music Player")
        self.root.geometry("800x950")
        self.root.configure(bg="#0f0f0f")
        self.root.resizable(False, False)
        
        # Configure style
        style = ttk.Style()
        style.theme_use('clam')
        style.configure('TScale', background="#0f0f0f", troughcolor="#1a1a1a")
        
        # Setup GUI elements
        self._setup_ui()
        
        # Register player callbacks
        self.player.on_track_changed = self._on_track_changed
        self.player.on_playback_started = self._on_playback_started
        self.player.on_playback_stopped = self._on_playback_stopped
        self.player.on_playlist_updated = self._on_playlist_updated
        self.player.on_shuffle_changed = self._on_shuffle_changed
        self.player.on_loop_changed = self._on_loop_changed
        
        # Update status periodically
        self._update_status()

    def _setup_ui(self):
        """Setup UI elements with modern Spotify/Apple Music design."""
        
        # Main container
        main_container = tk.Frame(self.root, bg="#0f0f0f")
        main_container.pack(fill="both", expand=True)
        
        # ============ HEADER SECTION ============
        header_frame = tk.Frame(main_container, bg="#1a1a1a", height=80)
        header_frame.pack(fill="x", pady=(0, 10))
        
        # App title with gradient effect using colors
        title_frame = tk.Frame(header_frame, bg="#1a1a1a")
        title_frame.pack(fill="x", padx=20, pady=15)
        
        title_label = tk.Label(
            title_frame,
            text="♫ WALKMAN",
            font=("Segoe UI", 28, "bold"),
            fg="#1DB954",
            bg="#1a1a1a"
        )
        title_label.pack(side="left")
        
        subtitle_label = tk.Label(
            title_frame,
            text="Music Player",
            font=("Segoe UI", 14),
            fg="#b3b3b3",
            bg="#1a1a1a"
        )
        subtitle_label.pack(side="left", padx=(10, 0))
        
        # ============ NOW PLAYING SECTION ============
        now_playing_frame = tk.Frame(main_container, bg="#1a1a1a", pady=20)
        now_playing_frame.pack(fill="x", padx=20, pady=10)
        
        # Album art placeholder (elegant gradient representation)
        album_frame = tk.Frame(now_playing_frame, bg="#282828", relief="flat", height=200, width=200)
        album_frame.pack(pady=10)
        
        album_label = tk.Label(
            album_frame,
            text="🎵",
            font=("Arial", 80),
            bg="#282828",
            fg="#404040"
        )
        album_label.pack(fill="both", expand=True)
        
        # Track information
        self.track_label = tk.Label(
            now_playing_frame,
            text="No track loaded",
            font=("Segoe UI", 16, "bold"),
            fg="white",
            bg="#1a1a1a",
            wraplength=700,
            justify="center"
        )
        self.track_label.pack(pady=10)
        
        # Status information
        info_frame = tk.Frame(now_playing_frame, bg="#1a1a1a")
        info_frame.pack(fill="x", padx=10)
        
        self.status_label = tk.Label(
            info_frame,
            text="Stopped",
            font=("Segoe UI", 11),
            fg="#b3b3b3",
            bg="#1a1a1a"
        )
        self.status_label.pack(side="left")
        
        self.playlist_label = tk.Label(
            info_frame,
            text="Playlist: 0 tracks",
            font=("Segoe UI", 11),
            fg="#b3b3b3",
            bg="#1a1a1a"
        )
        self.playlist_label.pack(side="right")
        
        # Progress bar and time display
        progress_frame = tk.Frame(now_playing_frame, bg="#1a1a1a")
        progress_frame.pack(fill="x", pady=(15, 8), padx=10)
        
        # Time label
        time_frame = tk.Frame(progress_frame, bg="#1a1a1a")
        time_frame.pack(fill="x", pady=(0, 8))
        
        self.time_elapsed = tk.Label(
            time_frame,
            text="0:00",
            font=("Segoe UI", 9),
            fg="#b3b3b3",
            bg="#1a1a1a"
        )
        self.time_elapsed.pack(side="left")
        
        # Progress bar using Canvas for better control
        self.progress_var = tk.DoubleVar()
        self.progress_scale = ttk.Scale(
            progress_frame,
            from_=0,
            to=100,
            orient="horizontal",
            variable=self.progress_var,
            state="disabled",
            command=self._on_progress_seek
        )
        self.progress_scale.pack(fill="x")
        
        # ============ PLAYBACK CONTROLS ============
        controls_frame = tk.Frame(main_container, bg="#0f0f0f", pady=15)
        controls_frame.pack(fill="x")
        
        # Main play controls
        main_controls = tk.Frame(controls_frame, bg="#0f0f0f")
        main_controls.pack(fill="x", padx=20)
        
        # Previous button
        prev_btn = tk.Button(
            main_controls,
            text="⏮ PREV",
            command=self._on_prev_click,
            font=("Segoe UI", 10, "bold"),
            bg="#333333",
            fg="white",
            activebackground="#404040",
            activeforeground="white",
            padx=15,
            pady=12,
            relief="flat",
            bd=0,
            cursor="hand2"
        )
        prev_btn.pack(side="left", padx=5, expand=True, fill="x")
        
        # Play/Pause button (prominent)
        self.play_pause_btn = tk.Button(
            main_controls,
            text="▶ PLAY",
            command=self._on_play_pause_click,
            font=("Segoe UI", 12, "bold"),
            bg="#1DB954",
            fg="black",
            activebackground="#1ed760",
            activeforeground="black",
            padx=20,
            pady=14,
            relief="flat",
            bd=0,
            cursor="hand2"
        )
        self.play_pause_btn.pack(side="left", padx=5, expand=True, fill="x")
        
        # Next button
        next_btn = tk.Button(
            main_controls,
            text="NEXT ⏭",
            command=self._on_next_click,
            font=("Segoe UI", 10, "bold"),
            bg="#333333",
            fg="white",
            activebackground="#404040",
            activeforeground="white",
            padx=15,
            pady=12,
            relief="flat",
            bd=0,
            cursor="hand2"
        )
        next_btn.pack(side="left", padx=5, expand=True, fill="x")
        
        # Secondary controls
        secondary_controls = tk.Frame(controls_frame, bg="#0f0f0f")
        secondary_controls.pack(fill="x", padx=20, pady=(10, 0))
        
        # Stop button
        stop_btn = tk.Button(
            secondary_controls,
            text="⏹ STOP",
            command=self._on_stop_click,
            font=("Segoe UI", 9, "bold"),
            bg="#404040",
            fg="#b3b3b3",
            activebackground="#505050",
            activeforeground="white",
            padx=10,
            pady=8,
            relief="flat",
            bd=0,
            cursor="hand2"
        )
        stop_btn.pack(side="left", padx=5, expand=True, fill="x")
        
        # ============ PLAYBACK MODES ============
        modes_frame = tk.Frame(main_container, bg="#0f0f0f", pady=10)
        modes_frame.pack(fill="x", padx=20)
        
        # Shuffle button
        self.shuffle_btn = tk.Button(
            modes_frame,
            text="🔀",
            command=self._on_shuffle_click,
            font=("Arial", 12),
            bg="#333333",
            fg="#b3b3b3",
            activebackground="#1DB954",
            activeforeground="black",
            padx=12,
            pady=10,
            relief="flat",
            bd=0,
            cursor="hand2",
            width=4
        )
        self.shuffle_btn.pack(side="left", padx=5)
        
        # Loop button
        self.loop_btn = tk.Button(
            modes_frame,
            text="🔁",
            command=self._on_loop_click,
            font=("Arial", 12),
            bg="#333333",
            fg="#b3b3b3",
            activebackground="#1DB954",
            activeforeground="black",
            padx=12,
            pady=10,
            relief="flat",
            bd=0,
            cursor="hand2",
            width=4
        )
        self.loop_btn.pack(side="left", padx=5)
        
        # Volume control
        volume_frame = tk.Frame(modes_frame, bg="#0f0f0f")
        volume_frame.pack(side="right", padx=5)
        
        vol_label = tk.Label(
            volume_frame,
            text="🔊",
            font=("Arial", 11),
            fg="#b3b3b3",
            bg="#0f0f0f"
        )
        vol_label.pack(side="left", padx=(0, 8))
        
        self.volume_scale = tk.Scale(
            volume_frame,
            from_=0,
            to=100,
            orient="horizontal",
            command=self._on_volume_change,
            bg="#282828",
            fg="#1DB954",
            troughcolor="#1a1a1a",
            highlightthickness=0,
            relief="flat",
            bd=0,
            length=120,
            activebackground="#1DB954"
        )
        self.volume_scale.set(70)
        self.volume_scale.pack(side="left", padx=5)
        
        self.volume_label = tk.Label(
            volume_frame,
            text="70%",
            font=("Segoe UI", 9),
            fg="#b3b3b3",
            bg="#0f0f0f",
            width=4
        )
        self.volume_label.pack(side="left", padx=(8, 0))
        
        # ============ FILE MANAGEMENT ============
        file_frame = tk.Frame(main_container, bg="#0f0f0f", pady=10)
        file_frame.pack(fill="x", padx=20)
        
        load_folder_btn = tk.Button(
            file_frame,
            text="📁 LOAD FOLDER",
            command=self._on_load_folder,
            font=("Segoe UI", 9, "bold"),
            bg="#404040",
            fg="white",
            activebackground="#505050",
            activeforeground="white",
            padx=12,
            pady=10,
            relief="flat",
            bd=0,
            cursor="hand2"
        )
        load_folder_btn.pack(side="left", padx=5, expand=True, fill="x")
        
        load_file_btn = tk.Button(
            file_frame,
            text="🎵 LOAD FILE",
            command=self._on_load_file,
            font=("Segoe UI", 9, "bold"),
            bg="#404040",
            fg="white",
            activebackground="#505050",
            activeforeground="white",
            padx=12,
            pady=10,
            relief="flat",
            bd=0,
            cursor="hand2"
        )
        load_file_btn.pack(side="left", padx=5, expand=True, fill="x")
        
        # ============ GPIO SIMULATION (if available) ============
        if self.gpio_controller and self.gpio_controller.use_simulator:
            gpio_frame = tk.LabelFrame(
                main_container,
                text="GPIO SIMULATION (Testing)",
                font=("Segoe UI", 9, "bold"),
                fg="#1DB954",
                bg="#1a1a1a",
                padx=15,
                pady=10
            )
            gpio_frame.pack(pady=10, padx=20, fill="x")
            
            # GPIO buttons grid
            gpio_buttons = tk.Frame(gpio_frame, bg="#1a1a1a")
            gpio_buttons.pack(fill="x")
            
            buttons_config = [
                ("Play/Pause", "play_pause", 0, 0),
                ("Next", "next", 0, 1),
                ("Prev", "prev", 0, 2),
                ("Vol+", "vol_up", 1, 0),
                ("Vol-", "vol_down", 1, 1),
                ("Shuffle", "shuffle", 1, 2),
                ("Loop", "loop", 2, 0),
            ]
            
            for text, action, row, col in buttons_config:
                btn = tk.Button(
                    gpio_buttons,
                    text=text,
                    command=lambda a=action: self.gpio_controller.simulate_button_press(a),
                    font=("Segoe UI", 8),
                    bg="#282828",
                    fg="#b3b3b3",
                    activebackground="#404040",
                    activeforeground="white",
                    padx=8,
                    pady=6,
                    relief="flat",
                    bd=0,
                    cursor="hand2"
                )
                btn.grid(row=row, column=col, padx=3, pady=3, sticky="ew")
            
            # Configure grid weights
            for i in range(3):
                gpio_buttons.columnconfigure(i, weight=1)

    def _on_play_pause_click(self):
        """Handle play/pause button click."""
        if self.player.is_playing and not self.player.is_paused:
            self.player.pause()
        else:
            self.player.play()

    def _on_prev_click(self):
        """Handle previous button click."""
        self.player.prev_track()

    def _on_next_click(self):
        """Handle next button click."""
        self.player.next_track()

    def _on_stop_click(self):
        """Handle stop button click."""
        self.player.stop()

    def _on_volume_change(self, value):
        """Handle volume scale change."""
        volume = float(value) / 100.0
        self.player.set_volume(volume)
        if hasattr(self, 'volume_label'):
            self.volume_label.config(text=f"{value}%")

    def _on_shuffle_click(self):
        """Handle shuffle button click."""
        is_enabled = self.player.toggle_shuffle()
        self._update_shuffle_button()

    def _on_loop_click(self):
        """Handle loop button click."""
        mode = self.player.toggle_loop()
        self._update_loop_button()

    def _on_progress_seek(self, value):
        """Handle progress bar seek."""
        if self.player.is_playing and self.player.current_sound:
            position = float(value)
            try:
                self.player.current_sound.set_pos(position)
            except:
                pass  # Audio backend might not support seeking

    def _update_shuffle_button(self):
        """Update shuffle button appearance based on state."""
        if self.player.shuffle_enabled:
            self.shuffle_btn.config(bg="#1DB954", fg="black")
        else:
            self.shuffle_btn.config(bg="#333333", fg="#b3b3b3")

    def _update_loop_button(self):
        """Update loop button appearance based on state."""
        mode = self.player.loop_mode
        if mode == 'off':
            self.loop_btn.config(bg="#333333", fg="#b3b3b3")
        elif mode == 'all':
            self.loop_btn.config(bg="#1DB954", fg="black")
        elif mode == 'one':
            self.loop_btn.config(bg="#1DB954", fg="black")

    def _on_load_folder(self):
        """Handle load folder button click."""
        folder = filedialog.askdirectory(title="Select Music Folder")
        if folder:
            count = self.player.load_music_directory(folder)
            if count > 0:
                failed = len(self.player.failed_files)
                if failed > 0:
                    messagebox.showinfo(
                        "Success", 
                        f"Loaded {count} playable files\n\n"
                        f"⚠ {failed} file(s) were skipped (corrupted or unsupported format)"
                    )
                else:
                    messagebox.showinfo("Success", f"Loaded {count} music files")
            else:
                messagebox.showwarning("No Files", "No playable music files found in this folder")

    def _on_load_file(self):
        """Handle load file button click."""
        file = filedialog.askopenfilename(
            title="Select Music File",
            filetypes=[
                ("Audio Files", "*.mp3 *.wav *.ogg *.flac"),
                ("MP3", "*.mp3"),
                ("WAV", "*.wav"),
                ("OGG", "*.ogg"),
                ("FLAC", "*.flac"),
                ("All Files", "*.*")
            ]
        )
        if file:
            if self.player.add_file(file):
                messagebox.showinfo("Success", "File added to playlist")
            else:
                messagebox.showerror(
                    "Error", 
                    "Failed to add file.\n\n"
                    "Reasons:\n"
                    "- File is corrupted\n"
                    "- Unsupported format or codec\n"
                    "- File is not readable\n\n"
                    "Try a different MP3, WAV, OGG, or FLAC file"
                )

    def _on_track_changed(self, index, track_path):
        """Callback when track changes."""
        track_name = os.path.basename(track_path)
        name=track_name.split('.')[0]
        self.track_label.config(text=f"Now Playing: {name}")

    def _on_playback_started(self):
        """Callback when playback starts."""
        self.play_pause_btn.config(text="⏸ PAUSE", bg="#1DB954", fg="black")

    def _on_playback_stopped(self):
        """Callback when playback stops."""
        self.play_pause_btn.config(text="▶ PLAY", bg="#1DB954", fg="black")
        self.track_label.config(text="Stopped")

    def _on_playlist_updated(self, count):
        """Callback when playlist is updated."""
        self.playlist_label.config(text=f"Playlist: {count} tracks")

    def _on_shuffle_changed(self, enabled):
        """Callback when shuffle state changes."""
        self._update_shuffle_button()

    def _on_loop_changed(self, mode):
        """Callback when loop mode changes."""
        self._update_loop_button()

    def _update_status(self):
        """Update status display periodically."""
        status = self.player.get_status()
        
        if status['is_playing']:
            if status['is_paused']:
                play_status = "⏸ PAUSED"
            else:
                play_status = "▶ PLAYING"
        else:
            play_status = "⏹ STOPPED"

        volume_percent = int(status['volume'] * 100)
        track_pos = f"{status['current_index'] + 1}/{status['playlist_size']}"
        
        self.status_label.config(
            text=f"{play_status} • Track {track_pos} • Volume {volume_percent}%"
        )
        
        # Update progress bar if playing
        if status['is_playing'] and not status['is_paused']:
            try:
                # Get position in seconds
                position_sec = self.player.get_playback_position()
                
                if position_sec >= 0:
                    # Update elapsed time display
                    elapsed_mins = int(position_sec) // 60
                    elapsed_secs = int(position_sec) % 60
                    self.time_elapsed.config(text=f"{elapsed_mins}:{elapsed_secs:02d}")
                    
                    # Progress bar: scale over typical song length (300 seconds = 5 minutes)
                    # This gives smooth progression without hitting 100% too quickly
                    progress = min((position_sec / 300.0) * 100, 100)
                    self.progress_scale.config(state="normal")
                    self.progress_var.set(progress)
                    self.progress_scale.config(state="disabled")
                else:
                    # pygame returned -1 (invalid), keep current time
                    pass
            except:
                pass  # Ignore errors from pygame
        else:
            self.progress_var.set(0)
            if not status['is_playing']:
                self.time_elapsed.config(text="0:00")

        self.root.after(200, self._update_status)

    def run(self):
        """Run the GUI."""
        self.root.mainloop()

    def shutdown(self):
        """Shutdown the GUI and player."""
        self.player.shutdown()
