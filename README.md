# NEDM - A Modern Wayland Compositor

NEDM is a modern, feature-rich Wayland compositor built on top of wlroots, evolved from the Cagebreak window manager. It provides a tiling window management experience with integrated desktop components including a status bar, wallpaper support, and comprehensive configuration options.

## Features

### Core Window Management
- **Tiling Window Manager**: Efficient keyboard-driven window management
- **Multiple Workspaces**: Support for up to 6 workspaces with easy switching
- **Multi-Monitor Support**: Native support for multiple displays
- **XWayland Compatibility**: Run legacy X11 applications seamlessly

### Desktop Integration
- **Integrated Status Bar**: Real-time system information display
  - Date and time
  - Battery status with charging indicators
  - Volume control information
  - WiFi connectivity status
  - Workspace indicators
- **Wallpaper Support**: PNG wallpaper with multiple scaling modes
  - Fill, fit, stretch, center, and tile modes
  - Configurable background colors
- **Layer Shell Protocol**: Support for notification daemons and overlays

### Modern Wayland Features
- **Full Protocol Support**: Comprehensive Wayland protocol implementation
- **Gaming Support**: Pointer constraints and relative pointer for gaming
- **Clipboard Management**: Primary selection and data control
- **Screen Capture**: Screenshots and screen recording support
- **Idle Management**: Screen locking and power management integration

## Installation

### Prerequisites
- **wlroots 0.19.0** or later
- **Wayland** development libraries
- **Cairo** and **Pango** for rendering
- **libinput** for input handling
- **Meson** and **Ninja** for building

### Building from Source

```bash
# Clone the repository
git clone <repository-url>
cd NEDM

# Configure the build
meson setup build

# Compile
ninja -C build

# Install (optional)
sudo ninja -C build install
```

## Quick Start

### Basic Usage

```bash
# Run NEDM
./build/nedm

# Run with custom config
./build/nedm -c ~/.config/nedm/config

# Show system information
./build/nedm -s

# Show version
./build/nedm -v
```

### Configuration Setup

1. Create the configuration directory:
   ```bash
   mkdir -p ~/.config/nedm/
   ```

2. Copy the example configuration:
   ```bash
   cp examples/config ~/.config/nedm/config
   ```

3. Edit your configuration:
   ```bash
   $EDITOR ~/.config/nedm/config
   ```

## Configuration

NEDM uses a text-based configuration file located at `~/.config/nedm/config`. The configuration supports:

### Basic Settings
```bash
# Set default terminal
exec foot

# Number of workspaces
workspaces 6

# Background color
background 0.25 0.21 0.2

# Key binding prefix
escape A-space
```

### Status Bar Configuration
```bash
# Enable and configure status bar
configure_status_bar position top_right
configure_status_bar height 24
configure_status_bar width_percent 20
configure_status_bar update_interval 1000
configure_status_bar font "monospace 10"
configure_status_bar bg_color 0.1 0.1 0.1 0.9
configure_status_bar text_color 1.0 1.0 1.0 1.0
```

### Wallpaper Configuration
```bash
# Set wallpaper
configure_wallpaper image_path "assets/nedm.png"
configure_wallpaper mode fill
configure_wallpaper bg_color 0.2 0.2 0.3 1.0
```

### Key Bindings
```bash
# Window management
bind s hsplit          # Split horizontally
bind S vsplit          # Split vertically
bind Q only            # Make window fullscreen
bind Tab focus         # Focus next window
bind C-k close         # Close window

# Workspace switching
bind 1 screen 1        # Switch to workspace 1
bind 2 screen 2        # Switch to workspace 2
# ... etc

# Application launching
bind t exec foot       # Launch terminal
bind w exec firefox    # Launch web browser
```

## Default Key Bindings

The default key binding prefix is `Alt+Space`. Common bindings include:

| Key Combination | Action |
|----------------|--------|
| `Alt+Space s` | Split window horizontally |
| `Alt+Space S` | Split window vertically |
| `Alt+Space Q` | Make window fullscreen |
| `Alt+Space Tab` | Focus next window |
| `Alt+Space Ctrl+k` | Close window |
| `Alt+Space t` | Launch terminal |
| `Alt+Space w` | Launch web browser |
| `Alt+Space 1-6` | Switch to workspace |
| `Alt+Space R` | Enter resize mode |

## Status Bar

The integrated status bar displays:
- **Current time and date**
- **Battery level and charging status** (color-coded)
- **Volume level**
- **WiFi connectivity** (color-coded)
- **Current workspace**

All components are configurable and can be individually enabled/disabled.

## Wallpaper Support

NEDM supports PNG wallpapers with multiple scaling modes:

- **Fill**: Scale to fill screen, cropping if necessary
- **Fit**: Scale to fit within screen, maintaining aspect ratio
- **Stretch**: Stretch to fill screen, ignoring aspect ratio
- **Center**: Center image without scaling
- **Tile**: Repeat image in a tiled pattern

## Advanced Features

### Multi-Monitor Setup
```bash
# Configure outputs
output eDP-1 enable
output eDP-1 pos 0 0 res 1920x1080 rate 60
output HDMI-A-1 pos 1920 0 res 1920x1080 rate 60
```

### Input Configuration
```bash
# Touchpad configuration
input type:touchpad tap enable
input type:touchpad natural_scroll enable
input type:touchpad dwt enable
```

### Layer Shell Applications
NEDM supports applications that use the layer shell protocol:
- **Notification daemons**: swaync, dunst, mako
- **Application launchers**: rofi, wofi
- **Status bars**: waybar (external)
- **Screen lockers**: swaylock

## Protocol Support

NEDM implements comprehensive Wayland protocol support:

- **Core Wayland protocols**
- **XDG Shell** for window management
- **Layer Shell** for overlays and panels
- **XWayland** for X11 application compatibility
- **Pointer constraints** for gaming applications
- **Relative pointer** for first-person games
- **Idle inhibit** for media applications
- **Gamma control** for night light applications
- **Screen capture** protocols

## Development

### Project Structure
```
NEDM/
├── nedm.c                 # Main executable
├── server.{c,h}           # Core server implementation
├── output.{c,h}           # Output/monitor management
├── status_bar.{c,h}       # Integrated status bar
├── wallpaper.{c,h}        # Wallpaper rendering
├── layer_shell.{c,h}      # Layer shell protocol
├── xdg_shell.{c,h}        # XDG shell implementation
├── input_manager.{c,h}    # Input handling
├── keybinding.{c,h}       # Key binding system
├── parse.{c,h}            # Configuration parsing
├── examples/config        # Example configuration
└── assets/                # Project assets
```

### Building for Development
```bash
# Debug build
meson setup build -Dbuildtype=debug

# Build and run
ninja -C build && ./build/nedm
```

## Troubleshooting

### Common Issues

**NEDM won't start**
- Ensure you're running under Wayland
- Check that wlroots dependencies are installed
- Verify your user is in the `input` group

**Configuration not loading**
- Check file exists at `~/.config/nedm/config`
- Verify file permissions are readable
- Check syntax with `nedm -c ~/.config/nedm/config`

**Wallpaper not displaying**
- Ensure PNG file exists at specified path
- Check file permissions
- Verify Cairo PNG support is installed

**Status bar not showing**
- Check if status bar is enabled in configuration
- Verify system information sources are available
- Check if required system files exist (`/sys/class/power_supply/`, etc.)

### Debug Information
```bash
# Show detailed system information
./build/nedm -s

# Run with specific configuration
./build/nedm -c /path/to/config
```

## Contributing

Contributions are welcome! Please:

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test thoroughly
5. Submit a pull request

## License

NEDM is licensed under the MIT License. See the LICENSE file for details.

## Acknowledgments

- Built on [wlroots](https://gitlab.freedesktop.org/wlroots/wlroots)
- Evolved from [Cagebreak](https://github.com/project-repo/cagebreak)
- Uses [Cairo](https://www.cairographics.org/) and [Pango](https://pango.gnome.org/) for rendering

## Version

Current version: **3.0.1**

For the latest updates and release notes, see [CHANGELOG.md](Changelog.md).