# Gas Detector Flutter App

A comprehensive Flutter Android application for monitoring and controlling an ESP32-based gas leakage detection system via MQTT.

## Features

### 📊 Real-time Monitoring
- Live gas level display with animated gauge
- PPM (parts per million) readings
- Three-state status indicator (SAFE, WARNING, DANGER)
- Visual alerts and notifications
- Device uptime tracking

### 🎛️ Remote Controls
- **Valve Control**: Open/Close gas valve remotely
- **Fan Control**: OFF (0°), PARTIAL (90°), ON (180°)
- **Buzzer Control**: Silence, Enable, Test
- **Testing**: Alarm test, Warning test, LED test, Sensor calibration
- **System Management**: Reset, Status request, Health ping, Reboot

### 📈 History & Analytics
- Gas level trend chart
- Statistics (Max, Min, Average PPM)
- Status distribution graphs
- Recent events timeline

### ⚙️ Configuration
- MQTT broker settings
- Custom threshold configuration (Warning/Danger levels)
- Auto-recovery settings
- Persistent storage of preferences

### 🔔 Notifications
- Background notifications for:
  - Safe status restoration
  - Warning level detection
  - Danger alerts
  - Valve closure events

## MQTT Topics

### Subscribe (App receives):
- `home/gas/level` - Gas PPM readings
- `home/gas/status` - Current safety status
- `home/gas/alert` - Emergency alerts
- `home/gas/valve` - Valve state changes
- `home/gas/fan` - Fan/ventilation state
- `home/gas/system/health` - System heartbeat (every 30s)

### Publish (App sends):
- `home/gas/control` - Control commands

## Commands Supported

### Valve Commands
- `VALVE_OPEN` - Open the gas valve
- `VALVE_CLOSE` - Close the gas valve

### Fan Commands
- `FAN_OFF` - Turn fan off (0°)
- `FAN_PARTIAL` - Partial ventilation (90°)
- `FAN_ON` - Full ventilation (180°)

### Buzzer Commands
- `BUZZER_SILENCE` - Mute the buzzer
- `BUZZER_ENABLE` - Re-enable buzzer
- `BUZZER_TEST` - Test buzzer (2-second beep)

### Test Commands
- `LED_TEST` - Test LED indicators
- `TEST_ALARM` - Simulate danger scenario (10s)
- `TEST_WARNING` - Simulate warning scenario (10s)
- `SENSOR_CALIBRATE` - Calibrate gas sensor

### System Commands
- `SYSTEM_RESET` - Reset to safe state
- `SYSTEM_STATUS` - Request full status update
- `SYSTEM_REBOOT` - Reboot ESP32
- `HEALTH_PING` - Request health status
- `SET_THRESHOLD` - Update warning/danger thresholds
- `AUTO_RECOVERY_ON` - Enable auto-recovery
- `AUTO_RECOVERY_OFF` - Disable auto-recovery

## Installation

### Prerequisites
- Flutter SDK (3.11.3 or higher)
- Android SDK
- Nix development environment (if using nix)

### Setup

1. Navigate to the app directory:
```bash
cd gas_detector_app
```

2. Install dependencies:
```bash
flutter pub get
```

3. Run the app:
```bash
flutter run
```

Or with Nix:
```bash
nix develop -c flutter run
```

### Building APK

```bash
flutter build apk --release
```

Or with Nix:
```bash
nix develop -c flutter build apk --release
```

The APK will be available at: `build/app/outputs/flutter-apk/app-release.apk`

## Configuration

### First Run Setup

1. Open the app
2. Navigate to Settings (bottom navigation)
3. Configure MQTT connection:
   - **Broker Address**: Your MQTT broker IP (default: 100.64.0.1)
   - **Port**: MQTT port (default: 1883)
   - **Client ID**: Unique identifier (default: FlutterGasDetectorApp)
4. Set gas level thresholds:
   - **Warning Threshold**: Default 300 PPM
   - **Danger Threshold**: Default 600 PPM
5. Tap "Save & Reconnect"

### Auto-Recovery

Enable auto-recovery to automatically open the gas valve when levels return to safe after being closed during a danger event.

## UI Screens

### Dashboard
- Large gas level gauge with color-coded zones
- Status card (SAFE/WARNING/DANGER)
- Device status panel (valve, fan, buzzer states)
- Quick action buttons
- Threshold information

### Controls
- Organized control sections for all device functions
- Visual feedback for current states
- One-tap command execution

### History
- Line chart showing gas level trends
- Statistics: Max, Min, Average PPM
- Status distribution bars
- Recent events list

### Settings
- MQTT configuration
- Threshold settings
- Auto-recovery toggle
- Device information
- Save and reconnect functionality

## Architecture

### State Management
- Provider pattern for reactive state updates
- Centralized `GasDetectorProvider` managing all state

### Services
- **MqttService**: Handles MQTT connection and messaging
- **NotificationService**: Local push notifications
- **StorageService**: SharedPreferences for settings persistence

### Models
- **GasDetectorModel**: Complete system state
- **AlertData**: Alert information
- **HistoryEntry**: Historical data points

### Widgets
- **GasLevelGauge**: Custom painted circular gauge
- **StatusCard**: Animated status display
- **DeviceStatusPanel**: Comprehensive device status

## Theme

Material Design 3 with dark theme:
- **Safe**: Green (#4CAF50)
- **Warning**: Orange (#FF9800)
- **Danger**: Red (#F44336)
- **Primary**: Blue (#2196F3)

## Dependencies

- `mqtt_client`: MQTT protocol
- `provider`: State management
- `shared_preferences`: Local storage
- `flutter_local_notifications`: Background notifications
- `fl_chart`: Charts and graphs
- `google_fonts`: Typography
- `flutter_animate`: UI animations

## Troubleshooting

### Connection Issues
1. Verify MQTT broker is running and accessible
2. Check broker IP address and port in Settings
3. Ensure device and MQTT broker are on same network
4. Check firewall settings

### Notifications Not Working
1. Grant notification permissions in Android settings
2. Ensure app is not in battery optimization
3. Check notification settings in app

### No Data Displaying
1. Verify ESP32 is powered on and connected
2. Check MQTT broker logs
3. Use "System Status" command to request update
4. Check connection status indicator

## ESP32 Compatibility

This app is designed to work with the ESP32 Gas Detector system with:
- MQ2 Gas Sensor
- OLED Display (SSD1306)
- Buzzer
- LED indicators
- Relay for valve control
- Servo for fan control

## License

This project is part of an IoT simulation for educational purposes.

## Support

For issues or questions, refer to the ESP32 source code documentation.
