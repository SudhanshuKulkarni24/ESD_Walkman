"""
Test utilities for Walkman Music Player
Includes GPIO simulation, audio test, and player verification
"""

import sys
from pathlib import Path
import os
import time

# Add src to path
sys.path.insert(0, str(Path(__file__).parent / 'src'))

from core.player import MusicPlayer
from gpio.controller import GPIOController


def test_player_basic():
    """Test basic player functionality."""
    print("\n" + "=" * 60)
    print("TEST: Basic Player Functionality")
    print("=" * 60)
    
    player = MusicPlayer()
    
    # Test initialization
    print("[1/5] Player initialization... ", end="", flush=True)
    assert not player.is_playing
    assert player.volume == 0.7
    print("✓")
    
    # Test volume control
    print("[2/5] Volume control... ", end="", flush=True)
    player.set_volume(0.5)
    assert player.volume == 0.5
    player.volume_up(0.2)
    assert player.volume == 0.7
    player.volume_down(0.1)
    assert player.volume == 0.6
    print("✓")
    
    # Test playlist loading
    print("[3/5] Playlist operations... ", end="", flush=True)
    assert len(player.get_playlist()) == 0
    player.add_file(__file__)  # Add this file as dummy
    assert len(player.get_playlist()) == 1
    print("✓")
    
    # Test status
    print("[4/5] Status reporting... ", end="", flush=True)
    status = player.get_status()
    assert 'is_playing' in status
    assert 'volume' in status
    assert 'playlist_size' in status
    print("✓")
    
    # Cleanup
    print("[5/5] Cleanup... ", end="", flush=True)
    player.shutdown()
    print("✓")
    
    print("\n✓ All basic tests passed!\n")


def test_gpio_simulator():
    """Test GPIO controller simulator."""
    print("=" * 60)
    print("TEST: GPIO Controller Simulator")
    print("=" * 60)
    
    gpio = GPIOController(use_simulator=True)
    
    # Test initialization
    print("[1/4] GPIO simulator initialization... ", end="", flush=True)
    assert gpio.setup()
    print("✓")
    
    # Test callback registration
    print("[2/4] Button callback registration... ", end="", flush=True)
    test_pressed = []
    
    def test_callback():
        test_pressed.append(True)
    
    assert gpio.register_callback('play_pause', test_callback)
    print("✓")
    
    # Test button simulation
    print("[3/4] Button simulation... ", end="", flush=True)
    gpio.simulate_button_press('play_pause')
    time.sleep(0.1)
    assert len(test_pressed) > 0
    print("✓")
    
    # Cleanup
    print("[4/4] Cleanup... ", end="", flush=True)
    gpio.cleanup()
    print("✓")
    
    print("\n✓ All GPIO simulator tests passed!\n")


def test_gui_imports():
    """Test GUI module imports."""
    print("=" * 60)
    print("TEST: GUI Module Imports")
    print("=" * 60)
    
    try:
        print("[1/2] Importing tkinter... ", end="", flush=True)
        import tkinter as tk
        print("✓")
        
        print("[2/2] Importing GUI module... ", end="", flush=True)
        from ui.gui import WalkmanGUI
        print("✓")
        
        print("\n✓ GUI imports successful!\n")
        return True
    except ImportError as e:
        print(f"✗\n[ERROR] {e}\n")
        return False


def test_audio_formats():
    """Test supported audio format detection."""
    print("=" * 60)
    print("TEST: Audio Format Support")
    print("=" * 60)
    
    player = MusicPlayer()
    
    print(f"Supported formats: {player.supported_formats}")
    print("\n✓ Audio format test complete!\n")


def test_file_handling():
    """Test file and directory handling."""
    print("=" * 60)
    print("TEST: File Handling")
    print("=" * 60)
    
    player = MusicPlayer()
    
    print("[1/2] Invalid file handling... ", end="", flush=True)
    result = player.add_file("/nonexistent/file.mp3")
    assert result == False
    print("✓")
    
    print("[2/2] Invalid directory handling... ", end="", flush=True)
    result = player.load_music_directory("/nonexistent/directory")
    assert len(player.get_playlist()) == 0
    print("✓")
    
    print("\n✓ File handling tests passed!\n")


def test_integration():
    """Integration test: Player + GPIO."""
    print("=" * 60)
    print("TEST: Integration (Player + GPIO)")
    print("=" * 60)
    
    player = MusicPlayer()
    gpio = GPIOController(use_simulator=True)
    
    from gpio.controller import RemoteControlHandler
    
    print("[1/4] Initialize player... ", end="", flush=True)
    assert not player.is_playing
    print("✓")
    
    print("[2/4] Initialize GPIO... ", end="", flush=True)
    assert gpio.setup()
    print("✓")
    
    print("[3/4] Create remote handler... ", end="", flush=True)
    remote = RemoteControlHandler(player, gpio)
    assert remote.setup()
    print("✓")
    
    print("[4/4] Cleanup... ", end="", flush=True)
    remote.cleanup()
    player.shutdown()
    print("✓")
    
    print("\n✓ Integration test passed!\n")


def run_all_tests():
    """Run all tests."""
    print("\n")
    print("╔" + "=" * 58 + "╗")
    print("║" + " " * 10 + "Walkman Music Player - Test Suite" + " " * 15 + "║")
    print("╚" + "=" * 58 + "╝")
    
    tests = [
        ("Basic Player", test_player_basic),
        ("GPIO Simulator", test_gpio_simulator),
        ("File Handling", test_file_handling),
        ("Audio Formats", test_audio_formats),
        ("Integration", test_integration),
        ("GUI Imports", test_gui_imports),
    ]
    
    passed = 0
    failed = 0
    
    for test_name, test_func in tests:
        try:
            test_func()
            passed += 1
        except Exception as e:
            print(f"\n✗ {test_name} FAILED: {e}\n")
            failed += 1
    
    # Summary
    print("=" * 60)
    print(f"Test Results: {passed} passed, {failed} failed")
    print("=" * 60)
    
    return failed == 0


if __name__ == "__main__":
    import argparse
    
    parser = argparse.ArgumentParser(description='Walkman Player Test Suite')
    parser.add_argument('--test', type=str, choices=[
        'all', 'player', 'gpio', 'file', 'audio', 'integration', 'gui'
    ], default='all', help='Run specific test')
    
    args = parser.parse_args()
    
    if args.test == 'all':
        success = run_all_tests()
    elif args.test == 'player':
        test_player_basic()
        success = True
    elif args.test == 'gpio':
        test_gpio_simulator()
        success = True
    elif args.test == 'file':
        test_file_handling()
        success = True
    elif args.test == 'audio':
        test_audio_formats()
        success = True
    elif args.test == 'integration':
        test_integration()
        success = True
    elif args.test == 'gui':
        success = test_gui_imports()
    
    sys.exit(0 if success else 1)
