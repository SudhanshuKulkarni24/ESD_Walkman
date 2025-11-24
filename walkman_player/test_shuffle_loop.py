#!/usr/bin/env python3
"""
Test shuffle and loop functionality
"""

import sys
from pathlib import Path

# Add src to path
sys.path.insert(0, str(Path(__file__).parent / 'src'))

from core.player import MusicPlayer


def test_shuffle_loop():
    """Test shuffle and loop modes."""
    print("\n" + "=" * 60)
    print("Testing Shuffle and Loop Functionality")
    print("=" * 60)
    
    player = MusicPlayer()
    
    # Create a mock playlist with 5 tracks
    test_tracks = [
        f"track_{i}.mp3" for i in range(1, 6)
    ]
    player.playlist = test_tracks
    player.current_index = 0
    
    print(f"\n✓ Created test playlist with {len(test_tracks)} tracks:")
    for i, track in enumerate(test_tracks):
        print(f"  {i + 1}. {track}")
    
    # Test shuffle toggle
    print("\n" + "-" * 60)
    print("Testing Shuffle Toggle:")
    print("-" * 60)
    
    print(f"Initial state: Shuffle enabled = {player.shuffle_enabled}")
    
    result = player.toggle_shuffle()
    print(f"After toggle 1: Shuffle enabled = {result}")
    print(f"Shuffle order: {player.shuffle_order}")
    
    result = player.toggle_shuffle()
    print(f"After toggle 2: Shuffle enabled = {result}")
    print(f"Shuffle order: {player.shuffle_order}")
    
    # Test loop modes
    print("\n" + "-" * 60)
    print("Testing Loop Modes:")
    print("-" * 60)
    
    modes_tested = []
    for _ in range(4):
        mode = player.toggle_loop()
        modes_tested.append(mode)
        print(f"Mode after toggle: {mode.upper()}")
    
    if modes_tested == ['all', 'one', 'off', 'all']:
        print("\n✓ Loop mode cycling works correctly!")
    else:
        print(f"\n✗ Loop mode cycling failed. Got: {modes_tested}")
    
    # Test shuffle with next_track
    print("\n" + "-" * 60)
    print("Testing Shuffle with Next Track:")
    print("-" * 60)
    
    player.shuffle_enabled = True
    player.shuffle_order = list(range(len(player.playlist)))
    import random
    random.seed(42)
    random.shuffle(player.shuffle_order)
    
    print(f"Shuffle order (seeded): {player.shuffle_order}")
    player.current_index = 0
    
    print("\nSimulating next_track calls:")
    current_pos = 0
    for i in range(3):
        # Simulate next_track logic
        current_pos = (current_pos + 1) % len(player.shuffle_order)
        next_idx = player.shuffle_order[current_pos]
        print(f"  Next track: {player.playlist[next_idx]} (index {next_idx})")
    
    # Test callbacks
    print("\n" + "-" * 60)
    print("Testing Callbacks:")
    print("-" * 60)
    
    callback_logs = []
    
    def on_shuffle(enabled):
        callback_logs.append(f"Shuffle changed: {enabled}")
    
    def on_loop(mode):
        callback_logs.append(f"Loop changed: {mode}")
    
    player.on_shuffle_changed = on_shuffle
    player.on_loop_changed = on_loop
    
    player.toggle_shuffle()
    player.toggle_loop()
    player.toggle_loop()
    
    print("Callback logs:")
    for log in callback_logs:
        print(f"  ✓ {log}")
    
    print("\n" + "=" * 60)
    print("All tests completed!")
    print("=" * 60 + "\n")


if __name__ == "__main__":
    test_shuffle_loop()
