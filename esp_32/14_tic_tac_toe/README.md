# Tic Tac Toe — MQTT + ESP32 (Wokwi)

A full multiplayer Tic Tac Toe system using MQTT as the transport layer,
with an ESP32 (Wokwi simulation) acting as the game server.

---

## Architecture

```
[Web Client]   ←── ws://100.64.0.1:8083  ──→ [Mosquitto]
[Flutter App]  ←── tcp://100.64.0.1:1883 ──→ [  Broker  ] ←→ [ESP32 Wokwi]
```

- **Mosquitto** is the MQTT broker (your self-hosted VPC + Tailscale)
- **ESP32** (Wokwi) is the game server: validates moves, runs Minimax AI, manages sessions
- **Web client** connects via WebSocket (browsers cannot use raw TCP)
- **Flutter Android** connects via raw TCP MQTT

---

## Prerequisites

### 1. Mosquitto broker

Copy `mosquitto/tictactoe.conf` to `/etc/mosquitto/conf.d/` on your VPC, then:

```bash
sudo cp mosquitto/tictactoe.conf /etc/mosquitto/conf.d/
sudo systemctl restart mosquitto

# Open ports on your VPC firewall / security group:
# TCP 1883  — MQTT (ESP32)
# TCP 9001  — MQTT WebSocket (Web browser + Flutter)

# If using UFW:
sudo ufw allow 1883
sudo ufw allow 9001
```

Verify it's running:
```bash
# Test raw MQTT
mosquitto_sub -h 100.64.0.1 -t "test" &
mosquitto_pub -h 100.64.0.1 -t "test" -m "hello"

# Test WebSocket (from a browser console or wscat)
# wscat -c ws://100.64.0.1:8083
```

### 2. Tailscale setup

- Your VPC must be running Tailscale (IP: `100.64.0.1` per your setup)
- **Flutter device**: install Tailscale app on Android, connect to your network
- **Wokwi**: use the **VS Code Extension** (not the browser editor) so the simulated
  ESP32 can reach your Tailscale IP through your host machine's network stack

### 3. Wokwi VS Code Extension

```
1. Install: https://marketplace.visualstudio.com/items?itemName=Wokwi.wokwi-vscode
2. Open the esp32/ folder in VS Code
3. Build the sketch using PlatformIO:
   pio run
4. Press F1 → "Wokwi: Start Simulator"
5. Watch Serial Monitor for MQTT connection logs
```

---

## Running

### ESP32 (Wokwi)

```bash
cd esp32/
pio run           # builds the firmware
# Then start Wokwi in VS Code
```

### Web Client

Simply open `web/index.html` in a browser (no server needed — it's a single file).
Or serve it:
```bash
cd web/
python3 -m http.server 3000
# Open http://localhost:3000
```

### Flutter Android

```bash
cd flutter/
flutter pub get
flutter run          # with Android device connected over USB
# or:
flutter build apk --release
```

Make sure your Android device is connected to Tailscale before launching.

---

## MQTT Topic Map

| Topic | Direction | Description |
|---|---|---|
| `ttt/lobby/announce` | Client → ESP32 | Register / heartbeat in lobby |
| `ttt/sys/players` | ESP32 → All | Current online player list (retained) |
| `ttt/sys/heartbeat` | ESP32 → All | Server heartbeat every 5s (retained) |
| `ttt/lobby/invite/{pid}` | Client → Client | Send invite to specific player |
| `ttt/lobby/response/{pid}` | Client → ESP32 | Accept/decline invite |
| `ttt/game/new` | Client → ESP32 | Request new game (AI or PvP) |
| `ttt/game/{gid}/state` | ESP32 → All | Live board state (retained) |
| `ttt/game/{gid}/result` | ESP32 → All | Game result (retained) |
| `ttt/player/{pid}/notify` | ESP32 → Client | Personal notifications (game_start etc.) |

---

## Message Payload Examples

```json
// ttt/lobby/announce
{ "pid": "abc12345", "name": "Alice", "client": "web" }

// ttt/game/new  (AI mode)
{ "mode": "ai", "pid1": "abc12345" }

// ttt/game/new  (PvP — triggered internally by ESP32 after invite accepted)
{ "mode": "pvp", "pid1": "abc12345", "pid2": "def67890" }

// ttt/game/{gid}/move
{ "pid": "abc12345", "cell": 4 }

// ttt/game/{gid}/state
{
  "gid": "xk9ab2",
  "board": ["X"," ","O"," ","X"," "," "," ","O"],
  "turn": "abc12345",
  "player1": "abc12345",
  "player2": "AI",
  "ai_mode": true,
  "status": "ongoing"
}

// ttt/game/{gid}/result
{
  "gid": "xk9ab2",
  "winner": "abc12345",
  "symbol": "X",
  "winning_cells": [0, 4, 8]
}

// ttt/player/{pid}/notify  — game start
{
  "type": "game_start",
  "gid": "xk9ab2",
  "player1": "abc12345",
  "player2": "AI",
  "ai_mode": true
}
```

---

## AI Algorithm

The ESP32 implements **Minimax with alpha-beta pruning**.

- AI always plays `O`, human always plays `X`
- Minimax is perfect — it will never lose (only win or draw)
- For a beatable AI, you can add a `difficulty` field to `ttt/game/new`
  and skip Minimax for easy/medium, using random moves instead

---

## Changing the broker IP

All connection settings are in one place per component:

| Component | File | Variable |
|---|---|---|
| ESP32 | `esp32/sketch.ino` | `MQTT_HOST` |
| Web | `web/index.html` | `BROKER_WS` |
| Flutter | `flutter/lib/mqtt_service.dart` | `kBrokerHost` |

---

## Debugging tips

```bash
# Watch all TTT topics live on your broker machine:
mosquitto_sub -h 100.64.0.1 -t "ttt/#" -v

# Watch ESP32 serial output in Wokwi VS Code terminal (all moves logged)

# Test a move manually:
mosquitto_pub -h 100.64.0.1 -t "ttt/lobby/announce" \
  -m '{"pid":"test01","name":"Test","client":"cli"}'
```
