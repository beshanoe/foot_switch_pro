# foot_switch_pro GUI

A web-based configuration tool for the foot_switch_pro Arduino project.

## Features

- **WebSerial API Integration**: Connect directly to your foot_switch_pro device via USB
- **Visual Switch Configuration**: Configure all 6 footswitches with an intuitive interface
- **Real-time Updates**: See current device settings and update them instantly  
- **Action Types Supported**:
  - MIDI Notes (0-127)
  - MIDI Control Change (CC number + value)
  - Keyboard commands (with modifier keys)
- **Serial Communication Log**: Monitor all device communication
- **Memory Management**: Clear device memory when needed

## Requirements

- Modern browser with WebSerial API support (Chrome, Edge, or Chromium-based browsers)
- HTTPS connection (required for WebSerial API)
- foot_switch_pro device connected via USB

## Usage

1. **Connect**: Click "Connect Device" and select your foot_switch_pro from the browser dialog
2. **Configure**: Use the dropdown menus to set action types for each footswitch
3. **Customize**: Enter specific values (MIDI notes, CC parameters, or keyboard keys)
4. **Monitor**: View the serial log to see all communication with the device
5. **Reset**: Use "Clear Memory" to reset all footswitch configurations

## Development

```bash
# Install dependencies
npm install

# Run development server
npm run dev

# Build for production
npm run build

# Preview production build
npm run preview
```

## Deployment

This app is configured for GitHub Pages deployment. The build process:

1. Uses `@sveltejs/adapter-static` for static site generation
2. Configures base path for GitHub Pages
3. Includes GitHub Actions workflow for automatic deployment

## Device Communication Protocol

The app communicates with the foot_switch_pro device using these serial commands:

- `GET ALL` - Read all switch configurations
- `GET <switch_num>` - Read specific switch configuration  
- `SET <switch_num> NOTE <midi_note>` - Set MIDI note action
- `SET <switch_num> CC <cc_num> <cc_val>` - Set MIDI CC action
- `SET <switch_num> KEY <modifiers>+<key>` - Set keyboard action
- `CLEAR` - Clear all switch configurations

## Browser Compatibility

WebSerial API is supported in:
- Chrome 89+
- Edge 89+
- Opera 75+
- Other Chromium-based browsers

**Note**: Firefox and Safari do not currently support WebSerial API.