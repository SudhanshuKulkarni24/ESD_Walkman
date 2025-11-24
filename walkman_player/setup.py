#!/usr/bin/env python3
"""
Setup and Installation Helper for Walkman Music Player
Helps verify dependencies and setup environment
"""

import sys
import subprocess
import platform
from pathlib import Path


def print_header(text):
    """Print formatted header."""
    print(f"\n{'=' * 60}")
    print(f"  {text}")
    print(f"{'=' * 60}\n")


def print_section(text):
    """Print section header."""
    print(f"\n[→] {text}\n")


def check_python_version():
    """Check if Python version meets requirements."""
    print_section("Checking Python Version")
    
    version = sys.version_info
    version_str = f"{version.major}.{version.minor}.{version.micro}"
    
    if version.major < 3 or (version.major == 3 and version.minor < 7):
        print(f"❌ Python {version_str} - Need Python 3.7 or higher")
        return False
    
    print(f"✓ Python {version_str} - OK")
    return True


def check_dependencies():
    """Check for required Python packages."""
    print_section("Checking Python Packages")
    
    required = {
        'pygame': 'Audio playback engine',
    }
    
    optional = {
        'tkinter': 'GUI (usually included with Python)',
    }
    
    all_ok = True
    
    for package, description in required.items():
        try:
            __import__(package)
            print(f"✓ {package:15} - {description}")
        except ImportError:
            print(f"❌ {package:15} - {description} (REQUIRED)")
            all_ok = False
    
    for package, description in optional.items():
        try:
            __import__(package)
            print(f"✓ {package:15} - {description}")
        except ImportError:
            print(f"⚠ {package:15} - {description} (optional)")
    
    return all_ok


def install_dependencies():
    """Attempt to install missing dependencies."""
    print_section("Installing Dependencies")
    
    try:
        print("Installing pygame...")
        subprocess.check_call([
            sys.executable, '-m', 'pip', 'install', 
            'pygame>=2.1.0'
        ])
        print("✓ pygame installed successfully\n")
        return True
    except subprocess.CalledProcessError:
        print("❌ Failed to install pygame")
        return False


def check_os():
    """Check operating system."""
    print_section("System Information")
    
    os_name = platform.system()
    os_ver = platform.release()
    arch = platform.architecture()[0]
    
    print(f"OS:           {os_name} {os_ver}")
    print(f"Architecture: {arch}")
    print(f"Python:       {sys.version.split()[0]}")
    
    if os_name == "Windows":
        print("✓ Windows detected - GUI mode supported")
    elif os_name == "Darwin":
        print("✓ macOS detected - GUI mode supported")
    elif os_name == "Linux":
        print("✓ Linux detected - Both GUI and CLI modes supported")
        print("  (DragonBoard 410c compatible)")
    
    return True


def verify_project_structure():
    """Verify project directory structure."""
    print_section("Verifying Project Structure")
    
    required_dirs = [
        'src',
        'src/core',
        'src/gpio',
        'src/ui',
        'config',
    ]
    
    required_files = [
        'main.py',
        'cli.py',
        'requirements.txt',
        'config/config.py',
        'src/core/player.py',
        'src/gpio/controller.py',
        'src/ui/gui.py',
    ]
    
    project_root = Path(__file__).parent
    all_ok = True
    
    for dir_path in required_dirs:
        full_path = project_root / dir_path
        if full_path.is_dir():
            print(f"✓ Directory: {dir_path}")
        else:
            print(f"❌ Missing: {dir_path}")
            all_ok = False
    
    for file_path in required_files:
        full_path = project_root / file_path
        if full_path.is_file():
            print(f"✓ File:      {file_path}")
        else:
            print(f"❌ Missing: {file_path}")
            all_ok = False
    
    return all_ok


def show_next_steps():
    """Show next steps for user."""
    print_header("SETUP COMPLETE!")
    
    print("""
Next Steps:

1. Desktop Testing (Recommended first step):
   ────────────────────────────────────────
   
   Windows/Mac:
     python main.py
   
   Linux:
     python3 main.py
   
   This opens the GUI where you can:
   - Load music files or folders
   - Test playback controls
   - Test GPIO simulator buttons

2. Command-Line Interface:
   ────────────────────────
   
   Interactive mode:
     python cli.py
   
   Headless mode (for servers/SSH):
     python cli.py --headless --folder ~/Music

3. DragonBoard 410c Setup:
   ───────────────────────
   
   See DRAGONBOARD_SETUP.md for:
   - Hardware wiring diagrams
   - GPIO pin configuration
   - System setup instructions
   - Troubleshooting guide

Documentation:
──────────────
- QUICKSTART.md - 5-minute quick start
- README.md - Full documentation
- PROJECT_OVERVIEW.md - Project structure and guide
- DRAGONBOARD_SETUP.md - Hardware integration

Testing:
────────
Run the test suite:
  python test_player.py

Test specific components:
  python test_player.py --test player
  python test_player.py --test gpio
  python test_player.py --test integration

Support:
────────
Check docstrings in source files for API reference.
All main classes have detailed docstrings.

Happy listening! 🎵
    """)


def main():
    """Run setup verification."""
    print_header("Walkman Music Player - Setup Verification")
    
    checks = [
        ("Python Version", check_python_version),
        ("OS Detection", check_os),
        ("Project Structure", verify_project_structure),
        ("Dependencies", check_dependencies),
    ]
    
    results = []
    for check_name, check_func in checks:
        try:
            result = check_func()
            results.append((check_name, result))
        except Exception as e:
            print(f"❌ Error during {check_name}: {e}")
            results.append((check_name, False))
    
    # Summary
    print_section("Setup Summary")
    
    all_passed = True
    for check_name, result in results:
        status = "✓ PASS" if result else "❌ FAIL"
        print(f"{check_name:20} - {status}")
        if not result:
            all_passed = False
    
    if not all_passed:
        print_section("Failed Checks")
        print("""
Some checks failed. Try:

1. Install missing dependencies:
   python -m pip install -r requirements.txt

2. Install pygame specifically:
   python -m pip install pygame

3. On Linux, you may need:
   sudo apt-get install python3-tk python3-dev

4. For audio libraries:
   sudo apt-get install libsndfile1-dev libportaudio2

Then run this script again.
        """)
        return False
    
    show_next_steps()
    return True


if __name__ == "__main__":
    success = main()
    sys.exit(0 if success else 1)
