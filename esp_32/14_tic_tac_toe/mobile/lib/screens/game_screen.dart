import 'dart:async';
import 'package:flutter/material.dart';
import '../services/mqtt_service.dart';
import '../services/game_state.dart';
import 'setup_screen.dart';

class GameScreen extends StatefulWidget {
  final MqttService mqttService;
  final GameState gameState;

  const GameScreen({
    super.key,
    required this.mqttService,
    required this.gameState,
  });

  @override
  State<GameScreen> createState() => _GameScreenState();
}

class _GameScreenState extends State<GameScreen> {
  StreamSubscription? _messageSubscription;

  @override
  void initState() {
    super.initState();
    _messageSubscription = widget.mqttService.messages.listen(_handleMessage);
    widget.gameState.addListener(_onGameStateChanged);
  }

  void _onGameStateChanged() {
    if (mounted) {
      setState(() {});
    }
  }

  void _handleMessage(GameMessage message) {
    print('[GameScreen] Received message: ${message.topic}');
    final data = message.json;
    if (data == null) {
      print('[GameScreen] Failed to parse JSON');
      return;
    }

    final gameId = widget.gameState.gameId;
    print('[GameScreen] Current gameId: $gameId');
    if (gameId == null) {
      print('[GameScreen] No gameId set, ignoring message');
      return;
    }

    // Board state update
    if (message.topic == 'ttt/game/$gameId/state') {
      print('[GameScreen] Processing board state update');
      final boardList = (data['board'] as List).map((e) => e.toString()).toList();
      print('[GameScreen] Board data: $boardList');
      widget.gameState.updateBoard(
        boardList,
        data['turn'],
        widget.mqttService.playerId,
      );
    }
    
    // Game result
    else if (message.topic == 'ttt/game/$gameId/result') {
      List<int> winCells = [];
      if (data['winning_cells'] != null && data['winning_cells'] != '') {
        winCells = (data['winning_cells'] as String)
            .split(',')
            .map((e) => int.tryParse(e) ?? -1)
            .where((e) => e >= 0)
            .toList();
      }
      
      widget.gameState.setGameResult(
        result: data['winner'],
        winnerSymbol: data['winner_symbol'],
        winningCells: winCells,
        myPid: widget.mqttService.playerId,
      );
    }
  }

  void _handleCellTap(int index) {
    print('[GAME] Cell $index tapped');
    print('[GAME] isMyTurn: ${widget.gameState.isMyTurn}');
    print('[GAME] isGameOver: ${widget.gameState.isGameOver}');
    print('[GAME] board[$index]: "${widget.gameState.board[index]}"');
    print('[GAME] gameId: ${widget.gameState.gameId}');
    
    if (!widget.gameState.isMyTurn) {
      print('[GAME] Blocked: Not my turn');
      return;
    }
    
    if (widget.gameState.isGameOver) {
      print('[GAME] Blocked: Game is over');
      return;
    }
    
    if (widget.gameState.board[index].isNotEmpty) {
      print('[GAME] Blocked: Cell is not empty');
      return;
    }
    
    print('[GAME] Sending move to ${widget.gameState.gameId} cell $index');
    widget.mqttService.sendMove(widget.gameState.gameId!, index);
  }

  void _rematch() {
    widget.gameState.resetGame();
    
    // Unsubscribe from old game
    widget.mqttService.unsubscribe('ttt/game/${widget.gameState.gameId}/state');
    widget.mqttService.unsubscribe('ttt/game/${widget.gameState.gameId}/result');
    
    // Request new game
    if (widget.gameState.isAiMode) {
      // For AI mode, just request a new game
      widget.mqttService.requestNewGame('Player', 'ai');
    } else {
      // For PvP, go back to setup
      _quitGame();
    }
  }

  void _quitGame() {
    if (widget.gameState.gameId != null) {
      widget.mqttService.unsubscribe('ttt/game/${widget.gameState.gameId}/state');
      widget.mqttService.unsubscribe('ttt/game/${widget.gameState.gameId}/result');
    }
    
    widget.gameState.clear();
    
    Navigator.of(context).pushAndRemoveUntil(
      MaterialPageRoute(
        builder: (_) => SetupScreen(
          mqttService: widget.mqttService,
          gameState: widget.gameState,
        ),
      ),
      (route) => false,
    );
  }

  @override
  void dispose() {
    _messageSubscription?.cancel();
    widget.gameState.removeListener(_onGameStateChanged);
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final myName = widget.gameState.mySymbol == 'X' ? 'You' : (widget.gameState.opponentName ?? 'AI');
    final oppName = widget.gameState.mySymbol == 'X' ? (widget.gameState.opponentName ?? 'AI') : 'You';
    
    return Scaffold(
      appBar: AppBar(
        backgroundColor: Colors.transparent,
        elevation: 0,
        title: const Text('Game', style: TextStyle(fontWeight: FontWeight.w700)),
        automaticallyImplyLeading: false,
        actions: [
          TextButton(
            onPressed: _quitGame,
            child: const Text(
              'Quit',
              style: TextStyle(color: Color(0xFF7B80A0)),
            ),
          ),
          const SizedBox(width: 8),
        ],
      ),
      body: SafeArea(
        child: Padding(
          padding: const EdgeInsets.symmetric(horizontal: 20.0),
          child: Column(
            children: [
              const SizedBox(height: 20),
              
              // Result banner (shown when game over)
              if (widget.gameState.isGameOver)
                _buildResultBanner(),
              
              const SizedBox(height: 20),
              
              // Turn indicator
              _buildTurnIndicator(),
              const SizedBox(height: 24),
              
              // Score display
              _buildScoreRow(myName, oppName),
              const SizedBox(height: 32),
              
              // Game board
              Expanded(
                child: Center(
                  child: AspectRatio(
                    aspectRatio: 1,
                    child: _buildBoard(),
                  ),
                ),
              ),
              const SizedBox(height: 32),
            ],
          ),
        ),
      ),
    );
  }

  Widget _buildResultBanner() {
    String emoji = '🤝';
    String text = "It's a draw!";
    String subtext = 'Nobody wins this round.';
    
    if (widget.gameState.gameResult != 'draw') {
      if (widget.gameState.gameResult == widget.mqttService.playerId) {
        emoji = '🎉';
        text = 'You win!';
        subtext = 'You played ${widget.gameState.mySymbol}';
      } else {
        emoji = '😔';
        text = '${widget.gameState.opponentName} wins!';
        subtext = 'Better luck next time.';
      }
    }
    
    return Container(
      padding: const EdgeInsets.all(20),
      decoration: BoxDecoration(
        color: const Color(0xFF22263A),
        borderRadius: BorderRadius.circular(14),
        border: Border.all(color: const Color(0xFF6C63FF), width: 1.5),
      ),
      child: Column(
        children: [
          Text(emoji, style: const TextStyle(fontSize: 48)),
          const SizedBox(height: 12),
          Text(
            text,
            style: const TextStyle(
              fontSize: 22,
              fontWeight: FontWeight.w700,
            ),
          ),
          const SizedBox(height: 6),
          Text(
            subtext,
            style: const TextStyle(
              fontSize: 13,
              color: Color(0xFF7B80A0),
            ),
          ),
          const SizedBox(height: 16),
          Row(
            children: [
              Expanded(
                child: ElevatedButton(
                  onPressed: _rematch,
                  child: const Text('Rematch'),
                ),
              ),
              const SizedBox(width: 12),
              Expanded(
                child: OutlinedButton(
                  onPressed: _quitGame,
                  child: const Text('Menu'),
                ),
              ),
            ],
          ),
        ],
      ),
    );
  }

  Widget _buildTurnIndicator() {
    String label = 'Current turn';
    String value = '—';
    Color borderColor = const Color(0xFF2E3350);
    
    if (!widget.gameState.isGameOver) {
      if (widget.gameState.isMyTurn) {
        value = 'Your turn (${widget.gameState.mySymbol})';
        borderColor = const Color(0xFF43D98C);
      } else {
        value = "${widget.gameState.opponentName}'s turn";
      }
    } else {
      label = 'Game status';
      value = 'Game over';
      borderColor = const Color(0xFF6C63FF);
    }
    
    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 20, vertical: 14),
      decoration: BoxDecoration(
        color: const Color(0xFF1A1D27),
        borderRadius: BorderRadius.circular(10),
        border: Border.all(color: borderColor, width: 1),
      ),
      child: Column(
        children: [
          Text(
            label,
            style: const TextStyle(
              fontSize: 12,
              color: Color(0xFF7B80A0),
            ),
          ),
          const SizedBox(height: 4),
          Text(
            value,
            style: TextStyle(
              fontSize: 16,
              fontWeight: FontWeight.w700,
              color: widget.gameState.isMyTurn && !widget.gameState.isGameOver
                  ? (widget.gameState.mySymbol == 'X' 
                      ? const Color(0xFF6C63FF) 
                      : const Color(0xFFFF6584))
                  : const Color(0xFFE8EAF0),
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildScoreRow(String player1Name, String player2Name) {
    return Row(
      children: [
        Expanded(
          child: _buildScoreCard(
            symbol: 'X',
            name: player1Name,
            score: widget.gameState.mySymbol == 'X' 
                ? widget.gameState.myScore 
                : widget.gameState.opponentScore,
            color: const Color(0xFF6C63FF),
          ),
        ),
        const Padding(
          padding: EdgeInsets.symmetric(horizontal: 12),
          child: Text(
            'VS',
            style: TextStyle(
              fontSize: 14,
              fontWeight: FontWeight.w700,
              color: Color(0xFF7B80A0),
            ),
          ),
        ),
        Expanded(
          child: _buildScoreCard(
            symbol: 'O',
            name: player2Name,
            score: widget.gameState.mySymbol == 'O' 
                ? widget.gameState.myScore 
                : widget.gameState.opponentScore,
            color: const Color(0xFFFF6584),
          ),
        ),
      ],
    );
  }

  Widget _buildScoreCard({
    required String symbol,
    required String name,
    required int score,
    required Color color,
  }) {
    return Container(
      padding: const EdgeInsets.symmetric(vertical: 12),
      decoration: BoxDecoration(
        color: const Color(0xFF1A1D27),
        borderRadius: BorderRadius.circular(10),
        border: Border.all(color: const Color(0xFF2E3350)),
      ),
      child: Column(
        children: [
          Text(
            symbol,
            style: TextStyle(
              fontSize: 24,
              fontWeight: FontWeight.w800,
              color: color,
            ),
          ),
          const SizedBox(height: 4),
          Text(
            name,
            style: const TextStyle(
              fontSize: 11,
              color: Color(0xFF7B80A0),
            ),
            maxLines: 1,
            overflow: TextOverflow.ellipsis,
          ),
          const SizedBox(height: 6),
          Text(
            score.toString(),
            style: const TextStyle(
              fontSize: 26,
              fontWeight: FontWeight.w800,
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildBoard() {
    return LayoutBuilder(
      builder: (context, constraints) {
        final size = constraints.maxWidth;
        return SizedBox(
          width: size,
          height: size,
          child: GridView.builder(
            physics: const NeverScrollableScrollPhysics(),
            gridDelegate: const SliverGridDelegateWithFixedCrossAxisCount(
              crossAxisCount: 3,
              mainAxisSpacing: 10,
              crossAxisSpacing: 10,
            ),
            itemCount: 9,
            itemBuilder: (context, index) => _buildCell(index),
          ),
        );
      },
    );
  }

  Widget _buildCell(int index) {
    final cellValue = widget.gameState.board[index];
    final isWinCell = widget.gameState.winningCells.contains(index);
    final isEmpty = cellValue.isEmpty;
    
    Color borderColor = const Color(0xFF2E3350);
    Color backgroundColor = const Color(0xFF1A1D27);
    
    if (isWinCell) {
      borderColor = cellValue == 'X' 
          ? const Color(0xFF6C63FF) 
          : const Color(0xFFFF6584);
      backgroundColor = cellValue == 'X' 
          ? const Color(0xFF6C63FF).withAlpha(38)
          : const Color(0xFFFF6584).withAlpha(38);
    }
    
    return GestureDetector(
      onTap: () => _handleCellTap(index),
      behavior: HitTestBehavior.opaque,
      child: AnimatedContainer(
        duration: const Duration(milliseconds: 200),
        decoration: BoxDecoration(
          color: backgroundColor,
          borderRadius: BorderRadius.circular(12),
          border: Border.all(
            color: borderColor,
            width: 1.5,
          ),
        ),
        child: Center(
          child: cellValue.isNotEmpty
              ? TweenAnimationBuilder<double>(
                  tween: Tween(begin: 0.0, end: 1.0),
                  duration: const Duration(milliseconds: 250),
                  builder: (context, value, child) {
                    return Transform.scale(
                      scale: value,
                      child: Text(
                        cellValue,
                        style: TextStyle(
                          fontSize: 48,
                          fontWeight: FontWeight.w800,
                          color: cellValue == 'X'
                              ? const Color(0xFF6C63FF)
                              : const Color(0xFFFF6584),
                        ),
                      ),
                    );
                  },
                )
              : null,
        ),
      ),
    );
  }
}
