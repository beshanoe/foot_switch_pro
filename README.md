# foot_switch_pro

A programmable MIDI/USB footswitch controller built with Teensy, featuring 6 configurable switches with multiple banks and a web-based configuration interface.

## Overview

foot_switch_pro is a versatile footswitch controller that can send MIDI notes, MIDI control changes, or keyboard commands. With 16 configurable banks and 6 switches per bank, you get 96 total programmable actions. The device stores all configurations in EEPROM for persistence across power cycles.

## Features

### Hardware
- **6 Footswitches**: Each switch can be configured independently
- **16 Banks**: Switch between different configurations on the fly
- **LCD Display**: Shows current bank, switch actions, and status
- **EEPROM Storage**: Persistent configuration storage
- **USB Connectivity**: Plug-and-play with any computer

### Actions
- **MIDI Notes**: Send note on/off messages
- **MIDI Control Change**: Send CC messages with configurable values
- **Keyboard Commands**: Send keyboard shortcuts with modifier keys
- **Bank Navigation**: Switch to specific banks or navigate next/previous

### Web Interface
- **Modern Web GUI**: Configure your device through any modern browser
- **WebSerial Integration**: Direct USB connection without drivers
- **Real-time Configuration**: See changes immediately
- **Bank Management**: Name your banks and organize configurations
- **Command Log**: Monitor all communication with the device

## Quick Start

### Hardware Setup
1. Connect your Arduino/Teensy to USB
2. Connect 6 momentary switches to digital pins 0-5
3. Connect I2C LCD display (address 0x27)
4. Connect I2C EEPROM (256KB, address 0x50)
5. Upload the `foot_switch_pro.ino` sketch

### Web Configuration
1. Open the web interface at: https://beshanoe.github.io/foot_switch_pro
2. Click "Connect Device" and select your foot_switch_pro
3. Configure switches using the intuitive interface
4. Create and name banks for different use cases
5. Save configurations directly to the device

## Hardware Requirements

- **Microcontroller**: Teensy 4.0 with USB MIDI capability
- **Display**: 16x2 I2C LCD (address 0x27)
- **Storage**: I2C EEPROM 256KB (address 0x50)
- **Switches**: 6 momentary footswitches
- **Connectivity**: USB cable

## Software Requirements

- **Arduino IDE** or **PlatformIO** for firmware development
- **Modern Web Browser** (Chrome, Edge, Firefox) for configuration
- **Required Libraries**:
  - Bounce.h (debouncing)
  - Wire.h (I2C communication)
  - I2C_eeprom.h (EEPROM storage)
  - LiquidCrystal_I2C.h (LCD display)

## Configuration Examples

### MIDI Controller Setup
```
Bank 1: "Recording"
- Switch 1: MIDI Note 60 (C4) - Record
- Switch 2: MIDI Note 61 (C#4) - Play
- Switch 3: MIDI Note 62 (D4) - Stop
- Switch 4: MIDI CC 64, 127 - Sustain On
- Switch 5: MIDI CC 64, 0 - Sustain Off
- Switch 6: BANK_NEXT - Switch to next bank
```

### Keyboard Shortcut Setup
```
Bank 2: "Editing"
- Switch 1: CTRL+Z - Undo
- Switch 2: CTRL+Y - Redo
- Switch 3: CTRL+C - Copy
- Switch 4: CTRL+V - Paste
- Switch 5: CTRL+S - Save
- Switch 6: BANK_PREV - Switch to previous bank
```

## Serial Commands

The device responds to these serial commands for configuration:

```bash
INFO                          # Get device info
GET ALL <bank>               # Get all switches for bank
SET <bank> <switch> NOTE <note>              # Set MIDI note
SET <bank> <switch> CC <cc_num> <cc_val>     # Set MIDI CC
SET <bank> <switch> KEY <modifiers>+<key>    # Set keyboard shortcut
SET <bank> <switch> BANK_SEL <bank_index>    # Set bank select
SET <bank> <switch> BANK_NEXT                # Set next bank
SET <bank> <switch> BANK_PREV                # Set previous bank
SET <bank> <switch> NONE                     # Clear switch
NAME_BANK <bank> <name>                      # Name a bank
CLEAR_BANK <bank>                           # Clear all switches in bank
SEL_BANK <bank>                             # Select active bank
```

## Development

### Building the Firmware
1. Install Arduino IDE
2. Install required libraries
3. Open `foot_switch_pro.ino`
4. Select your board (Teensy 4.0 recommended)
5. Upload to device

### Building the Web Interface
```bash
cd gui
npm install
npm run dev          # Development server
npm run build        # Production build
```

## Technical Specifications

- **Switches**: 6 total
- **Banks**: 16 total (96 total programmable actions)
- **Bank Names**: 16 characters maximum
- **MIDI**: Channel 1, Velocity 127
- **Debounce Time**: 15ms
- **Storage**: I2C EEPROM, 119 bytes per bank
- **Communication**: 9600 baud serial, WebSerial API

## Contributing

Contributions welcome! Please open issues and pull requests on GitHub.