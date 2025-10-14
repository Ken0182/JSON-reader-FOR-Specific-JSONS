# Startup Bug Fix - v1.1

## Problem Fixed

Previously, the `audio_config_system` binary could only be launched from the repository root directory. Running it from the `build/` directory would fail with:

```
Error: Weights file not found: config/weights.json
Error: Configuration database not found: data/clean_config.json
```

This occurred because paths were hardcoded as relative to the repository root.

## Solution Implemented

**Version 1.1** implements intelligent path auto-detection with the following features:

### 1. Auto-Detection Behavior

The system now automatically detects resource paths using a multi-strategy approach:

**Strategy 1: Current Working Directory**
- Checks if `config/weights.json` and `data/clean_config.json` exist relative to current directory
- Used when running from repository root

**Strategy 2: Build Directory Detection**
- Detects if current directory is named `build/`
- Automatically resolves paths to parent directory
- Used when running `./audio_config_system` from inside `build/`

**Strategy 3: Executable Location**
- Checks if executable is located in `build/` directory
- Resolves paths relative to executable's parent directory
- Used when running `./build/audio_config_system` from elsewhere

**Strategy 4: Executable Directory**
- Falls back to checking paths relative to executable location
- Handles custom installation scenarios

### 2. CLI Override Support

Both resource paths can be overridden via command-line arguments:

```bash
# Custom weights file
./build/audio_config_system --weights /path/to/custom_weights.json

# Custom config database
./build/audio_config_system --config /path/to/custom_config.json

# Both custom paths
./build/audio_config_system \
  --weights /custom/weights.json \
  --config /custom/clean_config.json
```

### 3. Help System

New `--help` flag provides usage information:

```bash
./build/audio_config_system --help
```

Output:
```
Usage: ./build/audio_config_system [OPTIONS]

Options:
  --weights <path>    Path to weights.json (default: auto-detected)
  --config <path>     Path to clean_config.json (default: auto-detected)
  --help              Show this help message

Auto-detection:
  The program automatically detects resource paths based on the
  executable location. Works from both repository root and build/
  directory without manual path configuration.
```

## Usage Examples

### From Repository Root (Always Worked)

```bash
# From repository root
$ pwd
/path/to/multi-dimensional-audio-system

$ ./build/audio_config_system
Multi-Dimensional Audio Configuration System
=================================================================
Loaded 30 configurations with multi-dimensional metadata.
```

### From Build Directory (NOW WORKS!)

```bash
# From build/ directory
$ cd build/

$ ./audio_config_system
Multi-Dimensional Audio Configuration System
=================================================================
Loaded 30 configurations with multi-dimensional metadata.
```

### With Custom Paths

```bash
# Using custom resource locations
$ ./build/audio_config_system \
  --weights /mnt/shared/custom_weights.json \
  --config /mnt/shared/custom_database.json
```

### From Arbitrary Location

```bash
# Executable can be run from anywhere
$ cd /tmp
$ /path/to/repo/build/audio_config_system
# Works - auto-detects paths from executable location
```

## Error Handling

Clear error messages when resources aren't found:

```bash
$ ./build/audio_config_system --weights missing.json
Error: Weights file not found: missing.json
Use --weights <path> to specify custom location
```

## Implementation Details

### Code Changes (main.cpp)

- Added `<filesystem>` support for path manipulation
- New `detectResourcePaths()` function with 4-strategy detection
- Command-line argument parsing for `--weights`, `--config`, `--help`
- Path existence verification before initialization
- Clear error messages with helpful suggestions

### Backward Compatibility

✅ 100% backward compatible
- Existing usage from repository root unchanged
- No breaking changes to API or behavior
- Additional flexibility for advanced users

## Testing Verification

All scenarios tested and working:

✅ Run from repository root  
✅ Run from build/ directory  
✅ Run with custom `--weights` path  
✅ Run with custom `--config` path  
✅ Run with both custom paths  
✅ `--help` flag displays usage  
✅ Clear error messages for missing files  

## Benefits

1. **Improved Developer Experience**: No need to `cd` to root directory
2. **Flexible Deployment**: Works from any directory structure
3. **Custom Configurations**: Easy testing with different config files
4. **Clear Documentation**: Help system explains auto-detection
5. **Better Error Messages**: Helpful guidance when resources missing

## Version History

- **v1.0**: Original implementation (root directory only)
- **v1.1**: Added auto-detection + CLI overrides (this fix)

---

*This fix makes the Multi-Dimensional Audio Configuration System more flexible and user-friendly while maintaining complete backward compatibility.*
