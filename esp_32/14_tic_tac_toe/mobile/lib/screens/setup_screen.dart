import 'package:flutter/material.dart';
import 'package:mqtt_client/mqtt_client.dart' as mqtt;
import '../services/mqtt_service.dart';
import '../services/game_state.dart';
import 'lobby_screen.dart';
import 'game_screen.dart';

class SetupScreen extends StatefulWidget {
  final MqttService mqttService;
  final GameState gameState;

  const SetupScreen({
    super.key,
    required this.mqttService,
    required this.gameState,
  });

  @override
  State<SetupScreen> createState() => _SetupScreenState();
}

class _SetupScreenState extends State<SetupScreen> {
  final _nameController = TextEditingController();
  String _selectedMode = 'ai';
  mqtt.MqttConnectionState _connectionState = mqtt.MqttConnectionState.disconnected;
  bool _isConnecting = false;

  @override
  void initState() {
    super.initState();
    _initMqtt();
  }

  Future<void> _initMqtt() async {
    setState(() => _isConnecting = true);
    
    await widget.mqttService.initialize();
    
    widget.mqttService.connectionState.listen((state) {
      if (mounted) {
        setState(() {
          _connectionState = state;
          _isConnecting = false;
        });
      }
    });
    
    widget.mqttService.messages.listen(_handleMessage);
    
    await widget.mqttService.connect();
  }

  void _handleMessage(GameMessage message) {
    final data = message.json;
    if (data == null) return;

    final myPid = widget.mqttService.playerId;

    // Handle game creation
    if (message.topic == 'ttt/game/$myPid/created') {
      if (data['accepted'] == false) {
        _showSnackBar('${data['from_name']} declined your invite');
        return;
      }
      
      widget.gameState.startNewGame(
        gameId: data['gid'],
        mySymbol: data['symbol'],
        opponentPid: data['opponent_pid'],
        opponentName: data['opponent_name'],
        aiMode: data['ai_mode'] ?? false,
      );
      
      widget.mqttService.subscribe('ttt/game/${data['gid']}/state');
      widget.mqttService.subscribe('ttt/game/${data['gid']}/result');
      
      Navigator.of(context).pushReplacement(
        MaterialPageRoute(
          builder: (_) => GameScreen(
            mqttService: widget.mqttService,
            gameState: widget.gameState,
          ),
        ),
      );
    }
  }

  void _startGame() {
    final name = _nameController.text.trim();
    if (name.isEmpty) {
      _showSnackBar('Please enter your name');
      return;
    }
    
    if (_connectionState != mqtt.MqttConnectionState.connected) {
      _showSnackBar('Not connected to broker');
      return;
    }
    
    widget.mqttService.announcePresence(name);
    
    if (_selectedMode == 'ai') {
      widget.mqttService.requestNewGame(name, 'ai');
    } else {
      Navigator.of(context).push(
        MaterialPageRoute(
          builder: (_) => LobbyScreen(
            mqttService: widget.mqttService,
            gameState: widget.gameState,
            playerName: name,
          ),
        ),
      );
    }
  }

  void _showSnackBar(String message) {
    ScaffoldMessenger.of(context).showSnackBar(
      SnackBar(
        content: Text(message),
        behavior: SnackBarBehavior.floating,
        shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(10)),
      ),
    );
  }

  @override
  void dispose() {
    _nameController.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      body: SafeArea(
        child: Padding(
          padding: const EdgeInsets.all(24.0),
          child: Column(
            mainAxisAlignment: MainAxisAlignment.center,
            crossAxisAlignment: CrossAxisAlignment.stretch,
            children: [
              // Title
              Text(
                'TicTacToe',
                textAlign: TextAlign.center,
                style: TextStyle(
                  fontSize: 32,
                  fontWeight: FontWeight.w800,
                  letterSpacing: -1,
                  foreground: Paint()
                    ..shader = const LinearGradient(
                      colors: [Color(0xFF6C63FF), Color(0xFFFF6584)],
                    ).createShader(const Rect.fromLTWH(0, 0, 200, 70)),
                ),
              ),
              const SizedBox(height: 8),
              const Text(
                'Powered by MQTT · ESP32',
                textAlign: TextAlign.center,
                style: TextStyle(
                  fontSize: 13,
                  color: Color(0xFF7B80A0),
                ),
              ),
              const SizedBox(height: 40),
              
              // Connection Status
              _buildConnectionStatus(),
              const SizedBox(height: 32),
              
              // Name Input Card
              Card(
                child: Padding(
                  padding: const EdgeInsets.all(20.0),
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      const Text(
                        'Your display name',
                        style: TextStyle(
                          fontSize: 13,
                          color: Color(0xFF7B80A0),
                          fontWeight: FontWeight.w600,
                        ),
                      ),
                      const SizedBox(height: 12),
                      TextField(
                        controller: _nameController,
                        decoration: const InputDecoration(
                          hintText: 'e.g. Alice',
                          hintStyle: TextStyle(color: Color(0xFF4A4E66)),
                        ),
                        maxLength: 20,
                        style: const TextStyle(fontSize: 16),
                        textInputAction: TextInputAction.done,
                        onSubmitted: (_) => _startGame(),
                      ),
                      const SizedBox(height: 20),
                      
                      const Text(
                        'Game mode',
                        style: TextStyle(
                          fontSize: 13,
                          color: Color(0xFF7B80A0),
                          fontWeight: FontWeight.w600,
                        ),
                      ),
                      const SizedBox(height: 12),
                      
                      // Mode Toggle
                      Row(
                        children: [
                          Expanded(
                            child: _buildModeButton(
                              mode: 'ai',
                              icon: '🤖',
                              label: 'Play vs AI',
                            ),
                          ),
                          const SizedBox(width: 12),
                          Expanded(
                            child: _buildModeButton(
                              mode: 'human',
                              icon: '🧑‍🤝‍🧑',
                              label: 'Human vs Human',
                            ),
                          ),
                        ],
                      ),
                    ],
                  ),
                ),
              ),
              const SizedBox(height: 24),
              
              // Start Button
              ElevatedButton(
                onPressed: _connectionState == mqtt.MqttConnectionState.connected
                    ? _startGame
                    : null,
                child: const Text('Start Game'),
              ),
              const SizedBox(height: 12),
              
              // Player ID Display
              Center(
                child: Text(
                  'Player ID: ${widget.mqttService.playerId.length > 8 ? widget.mqttService.playerId.substring(0, 8) : widget.mqttService.playerId}',
                  style: const TextStyle(
                    fontSize: 11,
                    color: Color(0xFF4A4E66),
                    fontFamily: 'monospace',
                  ),
                ),
              ),
            ],
          ),
        ),
      ),
    );
  }

  Widget _buildConnectionStatus() {
    IconData icon;
    String text;
    Color color;
    
    if (_isConnecting) {
      icon = Icons.sync;
      text = 'Connecting to broker…';
      color = const Color(0xFF7B80A0);
    } else if (_connectionState == mqtt.MqttConnectionState.connected) {
      icon = Icons.check_circle;
      text = 'Connected to broker';
      color = const Color(0xFF43D98C);
    } else {
      icon = Icons.error_outline;
      text = 'Connection failed — check broker';
      color = const Color(0xFFFF6584);
    }
    
    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 12),
      decoration: BoxDecoration(
        color: const Color(0xFF1A1D27),
        borderRadius: BorderRadius.circular(10),
        border: Border.all(color: const Color(0xFF2E3350)),
      ),
      child: Row(
        mainAxisAlignment: MainAxisAlignment.center,
        children: [
          Icon(icon, size: 18, color: color),
          const SizedBox(width: 10),
          Text(
            text,
            style: TextStyle(
              fontSize: 13,
              color: color,
              fontWeight: FontWeight.w500,
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildModeButton({
    required String mode,
    required String icon,
    required String label,
  }) {
    final isSelected = _selectedMode == mode;
    
    return GestureDetector(
      onTap: () => setState(() => _selectedMode = mode),
      child: Container(
        padding: const EdgeInsets.symmetric(vertical: 16),
        decoration: BoxDecoration(
          color: isSelected 
              ? const Color(0xFF6C63FF).withAlpha(20)
              : const Color(0xFF1A1D27),
          borderRadius: BorderRadius.circular(10),
          border: Border.all(
            color: isSelected 
                ? const Color(0xFF6C63FF)
                : const Color(0xFF2E3350),
            width: 1.5,
          ),
        ),
        child: Column(
          children: [
            Text(
              icon,
              style: const TextStyle(fontSize: 28),
            ),
            const SizedBox(height: 6),
            Text(
              label,
              textAlign: TextAlign.center,
              style: TextStyle(
                fontSize: 12,
                fontWeight: FontWeight.w600,
                color: isSelected 
                    ? const Color(0xFF6C63FF)
                    : const Color(0xFF7B80A0),
              ),
            ),
          ],
        ),
      ),
    );
  }
}
