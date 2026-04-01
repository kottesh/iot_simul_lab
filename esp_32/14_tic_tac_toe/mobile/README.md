# TicTacToe Flutter App

An elegant Flutter mobile app for playing Tic-Tac-Toe against AI or other players via MQTT.

## Features

- 🤖 **AI Mode**: Play against the unbeatable Minimax AI running on ESP32
- 🧑‍🤝‍🧑 **Multiplayer**: Challenge other online players in real-time
- 🎨 **Beautiful UI**: Clean, modern interface with smooth animations
- 📡 **WebSocket MQTT**: Real-time communication via WebSocket protocol
- 📊 **Live Scoreboard**: Track wins during your session

## Prerequisites

1. **Tailscale VPN**
   - Install Tailscale on your Android device
   - Connect to your Tailscale network
   - Ensure you can reach `100.64.0.1` (your VPC broker)

2. **MQTT Broker**
   - Mosquitto running on `100.64.0.1:9001` (WebSocket port)
   - See main README.md for broker setup

3. **ESP32 Server**
   - Running the game server (Wokwi simulation or physical ESP32)

## Installation

```bash
cd mobile

# Get dependencies
flutter pub get

# Run on connected Android device
flutter run

# Or build APK
flutter build apk --release
# APK will be in: build/app/outputs/flutter-apk/app-release.apk
```

## Configuration

If your broker IP is different from `100.64.0.1`, update the configuration:

**File:** `lib/services/mqtt_service.dart`

```dart
class MqttService {
  // Change this to your broker IP
  static const String kBrokerHost = '100.64.0.1';
  static const int kBrokerPort = 9001; // WebSocket port
  ...
}
```

## Usage

### Playing Against AI

1. Launch the app
2. Wait for "Connected to broker" status
3. Enter your display name
4. Select "🤖 Play vs AI" mode
5. Tap "Start Game"
6. The AI will respond immediately to your moves

### Playing Against Humans

1. Launch the app
2. Enter your display name
3. Select "🧑‍🤝‍🧑 Human vs Human" mode
4. Tap "Start Game"
5. You'll see a lobby with online players
6. Tap "Invite" on any player
7. Wait for them to accept
8. Game starts automatically when accepted

## UI Screens

### Setup Screen
- Connection status indicator
- Player name input
- Game mode selection (AI / Human)
- Displays your unique Player ID

### Lobby Screen (Multiplayer)
- Shows all online players
- Real-time player list updates
- Send/receive game invites
- Accept/decline invite dialogs

### Game Screen
- 3×3 interactive game board
- Turn indicator
- Live scoreboard (X vs O)
- Win/loss/draw result banner
- Rematch and quit options

## Architecture

```
┌─────────────────┐
│  Flutter App    │
│  (Android)      │
└────────┬────────┘
         │ WebSocket
         │ (ws://100.64.0.1:9001)
         ▼
┌─────────────────┐
│  Mosquitto      │
│  MQTT Broker    │
└────────┬────────┘
         │ MQTT
         ▼
┌─────────────────┐
│  ESP32 Server   │
│  (Game Logic)   │
└─────────────────┘
```

## MQTT Topics

The app subscribes to:
- `ttt/game/{playerId}/created` - Game session created
- `ttt/game/{gameId}/state` - Board state updates
- `ttt/game/{gameId}/result` - Game result
- `ttt/lobby/invite/{playerId}` - Incoming invites
- `ttt/sys/players/online` - Online player list

The app publishes to:
- `ttt/lobby/announce` - Announce presence
- `ttt/game/new` - Request new game
- `ttt/game/{gameId}/move` - Send move
- `ttt/lobby/invite/{targetId}` - Send invite
- `ttt/lobby/response/{inviterId}` - Respond to invite

## Troubleshooting

### Cannot connect to broker

**Check Tailscale:**
```bash
# On Android, verify Tailscale is connected
# In Tailscale app, check status
```

**Test broker reachability:**
```bash
# From your computer on same Tailscale network
ping 100.64.0.1

# Test MQTT WebSocket
wscat -c ws://100.64.0.1:9001
```

### App shows "Connection failed"

1. Verify Mosquitto is configured for WebSocket on port 9001
2. Check firewall allows port 9001: `sudo ufw allow 9001`
3. Ensure `useWebSocket = true` in `mqtt_service.dart`
4. Check Mosquitto logs: `sudo journalctl -u mosquitto -f`
5. Test WebSocket: `wscat -c ws://100.64.0.1:9001`

### MQTT keeps disconnecting

1. **Check Mosquitto config** - Use the config from `../mosquitto/tictactoe.conf`:
   ```bash
   sudo cp mosquitto/tictactoe.conf /etc/mosquitto/conf.d/
   sudo systemctl restart mosquitto
   ```

2. **Increase keepalive** - Already set to 60s in the app

3. **Check broker logs**:
   ```bash
   sudo journalctl -u mosquitto -f
   ```

4. **Monitor connection**:
   ```bash
   # Watch for disconnects in app logs
   flutter run -d linux | grep -E "MQTT|Disconnect"
   ```

5. **Network issues** - Ensure Tailscale is stable:
   ```bash
   ping -c 10 100.64.0.1
   ```

### Player list is empty

- Ensure ESP32 server is running and connected to broker
- Check ESP32 serial output for MQTT connection
- Other players must also announce to lobby (both web & flutter)

### Moves not registering

- Verify it's your turn (turn indicator shows "Your turn")
- Check ESP32 serial logs for move validation
- Ensure game ID matches between app and ESP32

## Building for Release

```bash
# Build release APK
cd mobile
flutter build apk --release

# Install on device
adb install build/app/outputs/flutter-apk/app-release.apk
```

## Design Philosophy

The UI follows these principles:
- **Minimal distraction**: Clean dark theme, no clutter
- **Clear state**: Always know whose turn it is
- **Instant feedback**: Smooth animations for moves
- **Elegant simplicity**: Every element has a purpose

## Color Palette

```
Background:    #0F1117
Surface:       #1A1D27
Card:          #22263A
Border:        #2E3350
Primary (X):   #6C63FF (purple)
Secondary (O): #FF6584 (pink)
Success:       #43D98C (green)
Text:          #E8EAF0
Muted:         #7B80A0
```

## License

Part of the 14_tic_tac_toe ESP32 project.
