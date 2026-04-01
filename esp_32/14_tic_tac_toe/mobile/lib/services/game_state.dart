import 'package:flutter/foundation.dart';

class GameState extends ChangeNotifier {
  String? _gameId;
  String? _mySymbol;
  String? _opponentPid;
  String? _opponentName;
  bool _isAiMode = false;
  List<String> _board = List.filled(9, '');
  bool _isMyTurn = false;
  bool _isGameOver = false;
  List<int> _winningCells = [];
  
  int _myScore = 0;
  int _opponentScore = 0;
  
  String? _gameResult;
  String? _winnerSymbol;
  
  // Getters
  String? get gameId => _gameId;
  String? get mySymbol => _mySymbol;
  String? get opponentPid => _opponentPid;
  String? get opponentName => _opponentName;
  bool get isAiMode => _isAiMode;
  List<String> get board => _board;
  bool get isMyTurn => _isMyTurn;
  bool get isGameOver => _isGameOver;
  List<int> get winningCells => _winningCells;
  int get myScore => _myScore;
  int get opponentScore => _opponentScore;
  String? get gameResult => _gameResult;
  String? get winnerSymbol => _winnerSymbol;
  
  void startNewGame({
    required String gameId,
    required String mySymbol,
    required String opponentPid,
    required String opponentName,
    required bool aiMode,
  }) {
    _gameId = gameId;
    _mySymbol = mySymbol;
    _opponentPid = opponentPid;
    _opponentName = opponentName;
    _isAiMode = aiMode;
    _board = List.filled(9, '');
    _isMyTurn = (mySymbol == 'X');
    _isGameOver = false;
    _winningCells = [];
    _gameResult = null;
    _winnerSymbol = null;
    notifyListeners();
  }
  
  void updateBoard(List<String> newBoard, String currentTurnPid, String myPid) {
    _board = newBoard;
    _isMyTurn = (currentTurnPid == myPid);
    print('[GameState] Board updated: $_board');
    print('[GameState] Current turn: $currentTurnPid, My PID: $myPid, isMyTurn: $_isMyTurn');
    notifyListeners();
  }
  
  void setGameResult({
    required String result,
    required String? winnerSymbol,
    required List<int> winningCells,
    required String myPid,
  }) {
    _isGameOver = true;
    _gameResult = result;
    _winnerSymbol = winnerSymbol;
    _winningCells = winningCells;
    
    // Update scores
    if (result != 'draw') {
      if (result == myPid) {
        if (_mySymbol == 'X') {
          _myScore++;
        } else {
          _opponentScore++;
        }
      } else {
        if (_mySymbol == 'X') {
          _opponentScore++;
        } else {
          _myScore++;
        }
      }
    }
    
    notifyListeners();
  }
  
  void resetGame() {
    _board = List.filled(9, '');
    _isMyTurn = (_mySymbol == 'X');
    _isGameOver = false;
    _winningCells = [];
    _gameResult = null;
    _winnerSymbol = null;
    notifyListeners();
  }
  
  void resetScores() {
    _myScore = 0;
    _opponentScore = 0;
    notifyListeners();
  }
  
  void clear() {
    _gameId = null;
    _mySymbol = null;
    _opponentPid = null;
    _opponentName = null;
    _isAiMode = false;
    _board = List.filled(9, '');
    _isMyTurn = false;
    _isGameOver = false;
    _winningCells = [];
    _myScore = 0;
    _opponentScore = 0;
    _gameResult = null;
    _winnerSymbol = null;
    notifyListeners();
  }
}
