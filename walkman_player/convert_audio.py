#!/usr/bin/env python3
"""
Audio Converter Helper for Walkman Music Player
Converts MP3 files to WAV or OGG format for better compatibility.
Requires: ffmpeg to be installed on system
"""

import subprocess
import os
import sys
from pathlib import Path
import shutil


def check_ffmpeg():
    """Check if ffmpeg is installed."""
    try:
        subprocess.run(['ffmpeg', '-version'], capture_output=True, check=True)
        return True
    except (subprocess.CalledProcessError, FileNotFoundError):
        return False


def install_ffmpeg_instructions():
    """Show instructions for installing FFmpeg."""
    system = sys.platform
    
    print("\n" + "=" * 60)
    print("FFmpeg is required for audio conversion")
    print("=" * 60)
    
    if system == "win32":
        print("""
Windows Installation:
1. Visit https://ffmpeg.org/download.html
2. Download the Windows build
3. Extract to a folder (e.g., C:\\ffmpeg)
4. Add to PATH:
   - Right-click "This PC" → Properties
   - Advanced system settings → Environment Variables
   - Add C:\\ffmpeg\\bin to PATH
5. Restart command prompt
        """)
    elif system == "darwin":
        print("""
macOS Installation:
1. Install Homebrew: https://brew.sh
2. Run: brew install ffmpeg
        """)
    elif system == "linux":
        print("""
Linux Installation:
Ubuntu/Debian:
  sudo apt-get install ffmpeg

Fedora/RHEL:
  sudo dnf install ffmpeg
        """)
    
    print("After installing, run this script again.\n")


def convert_audio(input_file, output_format='wav', quality=5):
    """
    Convert audio file to target format.
    
    Args:
        input_file: Path to input audio file
        output_format: Target format ('wav' or 'ogg')
        quality: Quality level (1-9, lower=better quality)
    
    Returns:
        Path to output file if successful, None otherwise
    """
    input_path = Path(input_file)
    
    if not input_path.exists():
        print(f"❌ File not found: {input_file}")
        return None
    
    # Create output filename
    output_name = input_path.stem + '.' + output_format
    output_path = input_path.parent / output_name
    
    print(f"Converting: {input_path.name}")
    print(f"   → {output_path.name}", end=" ", flush=True)
    
    try:
        if output_format.lower() == 'wav':
            # Convert to WAV
            cmd = ['ffmpeg', '-i', str(input_path), '-y', str(output_path)]
        elif output_format.lower() == 'ogg':
            # Convert to OGG with quality setting
            cmd = ['ffmpeg', '-i', str(input_path), '-q:a', str(quality), '-y', str(output_path)]
        else:
            print(f"\n❌ Unsupported format: {output_format}")
            return None
        
        # Run conversion silently
        result = subprocess.run(cmd, capture_output=True, text=True)
        
        if result.returncode == 0 and output_path.exists():
            print("✓")
            return str(output_path)
        else:
            print("❌")
            if result.stderr:
                print(f"   Error: {result.stderr.split(chr(10))[0]}")
            return None
    
    except Exception as e:
        print(f"❌\n   Error: {e}")
        return None


def batch_convert_folder(folder_path, input_format='mp3', output_format='wav', quality=5):
    """
    Convert all files of one format to another format in a folder.
    
    Args:
        folder_path: Path to folder containing audio files
        input_format: Input file extension (e.g., 'mp3')
        output_format: Target format ('wav' or 'ogg')
        quality: Quality level for lossy formats
    
    Returns:
        Tuple of (converted_count, failed_count)
    """
    folder = Path(folder_path)
    
    if not folder.is_dir():
        print(f"❌ Folder not found: {folder_path}")
        return 0, 0
    
    # Find all files with input format
    pattern = f"*.{input_format.lower()}"
    files = list(folder.glob(pattern))
    
    if not files:
        print(f"No {input_format.upper()} files found in {folder_path}")
        return 0, 0
    
    print(f"\nFound {len(files)} {input_format.upper()} files to convert")
    print("-" * 60)
    
    converted = 0
    failed = 0
    
    for i, file_path in enumerate(files, 1):
        print(f"[{i}/{len(files)}] ", end="")
        result = convert_audio(str(file_path), output_format, quality)
        if result:
            converted += 1
        else:
            failed += 1
    
    print("-" * 60)
    return converted, failed


def main():
    """Main entry point."""
    print("\n" + "=" * 60)
    print("Walkman Player - Audio Format Converter")
    print("=" * 60)
    
    # Check FFmpeg
    if not check_ffmpeg():
        install_ffmpeg_instructions()
        print("Please install FFmpeg and try again.")
        return False
    
    print("\n✓ FFmpeg found\n")
    
    # Get options from user
    print("Choose conversion option:")
    print("1. Convert single file")
    print("2. Batch convert folder (all MP3s)")
    print("3. Exit")
    
    choice = input("\nEnter choice (1-3): ").strip()
    
    if choice == '1':
        file_path = input("Enter file path: ").strip()
        if file_path.startswith('"') and file_path.endswith('"'):
            file_path = file_path[1:-1]
        
        print("\nChoose output format:")
        print("1. WAV (best compatibility, larger files)")
        print("2. OGG (good compatibility, smaller files)")
        
        format_choice = input("Enter choice (1-2): ").strip()
        
        if format_choice == '1':
            output_format = 'wav'
            print(f"\nConverting to {output_format.upper()}...")
            result = convert_audio(file_path, output_format)
        elif format_choice == '2':
            output_format = 'ogg'
            print(f"\nConverting to {output_format.upper()}...")
            print("(Quality 5 = good balance, 1-3 = higher quality, 6-9 = lower quality)\n")
            result = convert_audio(file_path, output_format)
        else:
            print("Invalid choice")
            return False
        
        if result:
            print(f"\n✓ Converted successfully!")
            print(f"  Output: {result}")
            print(f"\nLoad this file in Walkman Player for playback.")
        else:
            print(f"\n❌ Conversion failed")
        
    elif choice == '2':
        folder_path = input("Enter folder path: ").strip()
        if folder_path.startswith('"') and folder_path.endswith('"'):
            folder_path = folder_path[1:-1]
        
        print("\nChoose output format:")
        print("1. WAV (best compatibility, larger files)")
        print("2. OGG (good compatibility, smaller files)")
        
        format_choice = input("Enter choice (1-2): ").strip()
        
        if format_choice == '1':
            output_format = 'wav'
        elif format_choice == '2':
            output_format = 'ogg'
        else:
            print("Invalid choice")
            return False
        
        print(f"\nConverting all MP3s to {output_format.upper()}...")
        print(f"This may take a few minutes...\n")
        
        converted, failed = batch_convert_folder(folder_path, 'mp3', output_format)
        
        print(f"\n✓ Conversion complete!")
        print(f"  Successfully converted: {converted}")
        print(f"  Failed: {failed}")
        print(f"\nLoad the converted files in Walkman Player.")
    
    elif choice == '3':
        print("Goodbye!")
        return True
    
    else:
        print("Invalid choice")
        return False
    
    print()
    return True


if __name__ == "__main__":
    try:
        success = main()
        sys.exit(0 if success else 1)
    except KeyboardInterrupt:
        print("\n\nCancelled.")
        sys.exit(0)
