import 'dart:async';
import 'package:flutter/material.dart';
import '../services/mqtt_service.dart';
import '../services/game_state.dart';
import 'game_screen.dart';

class LobbyScreen extends StatefulWidget {
  final MqttService mqttService;
  final GameState gameState;
  final String playerName;

  const LobbyScreen({
    super.key,
    required this.mqttService,
    required this.gameState,
    required this.playerName,
  });

  @override
  State<LobbyScreen> createState() => _LobbyScreenState();
}

class _LobbyScreenState extends State<LobbyScreen> {
  List<Player> _onlinePlayers = [];
  StreamSubscription? _messageSubscription;
  String? _pendingInviteFrom;

  @override
  void initState() {
    super.initState();
    _messageSubscription = widget.mqttService.messages.listen(_handleMessage);
    
    // Re-announce presence to trigger player list update
    print('[LobbyScreen] Announcing presence in lobby');
    Future.delayed(const Duration(milliseconds: 500), () {
      widget.mqttService.announcePresence(widget.playerName);
    });
  }

  void _handleMessage(GameMessage message) {
    print('[LobbyScreen] Received message: ${message.topic}');
    final data = message.json;
    if (data == null) {
      print('[LobbyScreen] Failed to parse JSON');
      return;
    }

    final myPid = widget.mqttService.playerId;

    if (message.topic == 'ttt/sys/players/online') {
      print('[LobbyScreen] Players data: ${data['players']}');
      final players = (data['players'] as List?)
          ?.map((p) => Player(
                pid: p['pid'],
                name: p['name'],
                client: p['client'],
              ))
          .where((p) => p.pid != myPid)
          .toList() ?? [];
      
      print('[LobbyScreen] Filtered players: ${players.length}');
      for (var p in players) {
        print('[LobbyScreen]   - ${p.name} (${p.pid})');
      }
      
      if (mounted) {
        setState(() => _onlinePlayers = players);
      }
    } else if (message.topic == 'ttt/lobby/invite/$myPid') {
      _showInviteDialog(data['from_pid'], data['from_name']);
    } else if (message.topic == 'ttt/game/$myPid/created') {
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

  void _showInviteDialog(String fromPid, String fromName) {
    setState(() {
      _pendingInviteFrom = fromPid;
    });
    
    showDialog(
      context: context,
      barrierDismissible: false,
      builder: (context) => AlertDialog(
        backgroundColor: const Color(0xFF22263A),
        shape: RoundedRectangleBorder(
          borderRadius: BorderRadius.circular(14),
          side: const BorderSide(color: Color(0xFF6C63FF), width: 1.5),
        ),
        title: const Text('🎮 Incoming Invite'),
        content: Text('$fromName wants to play with you!'),
        actions: [
          OutlinedButton(
            onPressed: () {
              _respondToInvite(false);
              Navigator.of(context).pop();
            },
            child: const Text('Decline'),
          ),
          ElevatedButton(
            onPressed: () {
              _respondToInvite(true);
              Navigator.of(context).pop();
            },
            child: const Text('Accept'),
          ),
        ],
      ),
    );
  }

  void _respondToInvite(bool accepted) {
    if (_pendingInviteFrom != null) {
      widget.mqttService.respondToInvite(
        _pendingInviteFrom!,
        widget.playerName,
        accepted,
      );
      setState(() {
        _pendingInviteFrom = null;
      });
    }
  }

  void _invitePlayer(Player player) {
    widget.mqttService.sendInvite(player.pid, widget.playerName);
    _showSnackBar('Invite sent to ${player.name}! Waiting for response…');
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
    _messageSubscription?.cancel();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        backgroundColor: Colors.transparent,
        elevation: 0,
        title: const Text('Lobby', style: TextStyle(fontWeight: FontWeight.w700)),
        leading: IconButton(
          icon: const Icon(Icons.arrow_back),
          onPressed: () {
            widget.mqttService.leaveLobby();
            Navigator.of(context).pop();
          },
        ),
      ),
      body: Padding(
        padding: const EdgeInsets.all(20.0),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.stretch,
          children: [
            // Player info card
            Card(
              child: Padding(
                padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 12),
                child: Row(
                  children: [
                    Container(
                      width: 8,
                      height: 8,
                      decoration: const BoxDecoration(
                        color: Color(0xFF43D98C),
                        shape: BoxShape.circle,
                      ),
                    ),
                    const SizedBox(width: 12),
                    Expanded(
                      child: RichText(
                        text: TextSpan(
                          style: const TextStyle(fontSize: 13),
                          children: [
                            const TextSpan(text: 'Playing as '),
                            TextSpan(
                              text: widget.playerName,
                              style: const TextStyle(fontWeight: FontWeight.w700),
                            ),
                            const TextSpan(text: ' · '),
                            TextSpan(
                              text: widget.mqttService.playerId.substring(0, 6),
                              style: const TextStyle(
                                color: Color(0xFF6C63FF),
                                fontFamily: 'monospace',
                              ),
                            ),
                          ],
                        ),
                      ),
                    ),
                  ],
                ),
              ),
            ),
            const SizedBox(height: 24),
            
            // Header
            Row(
              mainAxisAlignment: MainAxisAlignment.spaceBetween,
              children: [
                const Text(
                  'ONLINE PLAYERS',
                  style: TextStyle(
                    fontSize: 12,
                    fontWeight: FontWeight.w700,
                    color: Color(0xFF7B80A0),
                    letterSpacing: 0.5,
                  ),
                ),
                Text(
                  '${_onlinePlayers.length} online',
                  style: const TextStyle(
                    fontSize: 12,
                    color: Color(0xFF7B80A0),
                  ),
                ),
              ],
            ),
            const SizedBox(height: 16),
            
            // Player list
            Expanded(
              child: _onlinePlayers.isEmpty
                  ? Center(
                      child: Column(
                        mainAxisAlignment: MainAxisAlignment.center,
                        children: [
                          const Text(
                            '👥',
                            style: TextStyle(fontSize: 48),
                          ),
                          const SizedBox(height: 16),
                          const Text(
                            'Waiting for players…',
                            style: TextStyle(
                              fontSize: 16,
                              color: Color(0xFF7B80A0),
                            ),
                          ),
                          const SizedBox(height: 8),
                          Text(
                            'Share your game with friends!',
                            style: TextStyle(
                              fontSize: 13,
                              color: const Color(0xFF7B80A0).withAlpha(153),
                            ),
                          ),
                        ],
                      ),
                    )
                  : ListView.separated(
                      itemCount: _onlinePlayers.length,
                      separatorBuilder: (context, index) => const SizedBox(height: 10),
                      itemBuilder: (context, index) {
                        final player = _onlinePlayers[index];
                        return _buildPlayerCard(player);
                      },
                    ),
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildPlayerCard(Player player) {
    return Container(
      padding: const EdgeInsets.all(14),
      decoration: BoxDecoration(
        color: const Color(0xFF1A1D27),
        borderRadius: BorderRadius.circular(10),
        border: Border.all(color: const Color(0xFF2E3350)),
      ),
      child: Row(
        children: [
          // Avatar
          Container(
            width: 44,
            height: 44,
            decoration: BoxDecoration(
              color: const Color(0xFF6C63FF).withAlpha(51),
              shape: BoxShape.circle,
            ),
            child: Center(
              child: Text(
                player.name.isNotEmpty 
                    ? player.name[0].toUpperCase()
                    : '?',
                style: const TextStyle(
                  fontSize: 18,
                  fontWeight: FontWeight.w700,
                  color: Color(0xFF6C63FF),
                ),
              ),
            ),
          ),
          const SizedBox(width: 14),
          
          // Player info
          Expanded(
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Text(
                  player.name,
                  style: const TextStyle(
                    fontSize: 15,
                    fontWeight: FontWeight.w600,
                  ),
                ),
                const SizedBox(height: 4),
                Text(
                  '${player.client} · ${player.pid.substring(0, 6)}',
                  style: const TextStyle(
                    fontSize: 11,
                    color: Color(0xFF7B80A0),
                    fontFamily: 'monospace',
                  ),
                ),
              ],
            ),
          ),
          
          // Invite button
          SizedBox(
            height: 36,
            child: ElevatedButton(
              onPressed: () => _invitePlayer(player),
              style: ElevatedButton.styleFrom(
                padding: const EdgeInsets.symmetric(horizontal: 20),
                backgroundColor: const Color(0xFF6C63FF).withAlpha(31),
                foregroundColor: const Color(0xFF6C63FF),
                elevation: 0,
                shape: RoundedRectangleBorder(
                  borderRadius: BorderRadius.circular(8),
                  side: const BorderSide(
                    color: Color(0xFF6C63FF),
                    width: 0.5,
                  ),
                ),
                textStyle: const TextStyle(
                  fontSize: 13,
                  fontWeight: FontWeight.w700,
                ),
              ),
              child: const Text('Invite'),
            ),
          ),
        ],
      ),
    );
  }
}

class Player {
  final String pid;
  final String name;
  final String client;

  Player({
    required this.pid,
    required this.name,
    required this.client,
  });
}
