# Building on Windows (MINGW/MSYS)

This project now supports cross-platform builds including Windows with MINGW32/MINGW64/MSYS2.

## Prerequisites

You need:
- **MINGW32/MINGW64** or **MSYS2** environment
- **g++** compiler with C++17 support (included with MINGW/MSYS2)
- **make** utility (included with MINGW/MSYS2)

## Quick Start

1. Open **MINGW32** or **MSYS2** terminal

2. Navigate to the project directory:
   ```bash
   cd /c/Users/Can/TEST/JSON-reader-FOR-Specific-JSONS
   ```

3. Build the project:
   ```bash
   make
   ```

4. Run the application (NEW v1.1: Works from any directory!):
   ```bash
   # From repository root
   make run
   
   # Or run directly
   ./build/audio_config_system.exe
   
   # Or from build/ directory (auto-detects paths!)
   cd build
   ./audio_config_system.exe
   ```

## Available Make Targets

```bash
make              # Build the project
make run          # Build and run
make clean        # Clean build artifacts
make help         # Show all available targets
```

## Platform Detection

The Makefile automatically detects:
- **Windows** (MINGW/MSYS) → builds `audio_config_system.exe`
- **Linux** → builds `audio_config_system`
- **macOS** → builds `audio_config_system`

## Troubleshooting

### "cannot execute binary file" Error
This means the executable was built for Linux but you're on Windows.
**Solution:** Make sure you build on Windows with the updated Makefile.

### Missing json.hpp
Run:
```bash
make setup
```

### Clean Build
If you have issues, try a clean rebuild:
```bash
make clean
make
```

## Running the Application

Once built, you can run from multiple locations (v1.1 auto-detection):

1. **Interactive mode (from root):**
   ```bash
   ./build/audio_config_system.exe
   ```

2. **From build/ directory (NEW!):**
   ```bash
   cd build
   ./audio_config_system.exe
   ```

3. **With custom resource paths:**
   ```bash
   ./build/audio_config_system.exe --weights config/weights.json --config data/clean_config.json
   ```

4. **Show help:**
   ```bash
   ./build/audio_config_system.exe --help
   ```

## Notes

- On Windows, executables must have `.exe` extension
- The Makefile handles this automatically
- All functionality works the same across platforms
