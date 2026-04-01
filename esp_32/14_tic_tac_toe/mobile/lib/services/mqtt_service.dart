import 'dart:async';
import 'dart:convert';
import 'dart:math';
import 'package:flutter/foundation.dart' show kIsWeb;
import 'package:mqtt_client/mqtt_client.dart';
import 'package:mqtt_client/mqtt_server_client.dart';
import 'package:shared_preferences/shared_preferences.dart';

// Custom message class to avoid conflict
class GameMessage {
  final String topic;
  final String payload;
  
  GameMessage({required this.topic, required this.payload});
  
  Map<String, dynamic>? get json {
    try {
      return jsonDecode(payload);
    } catch (e) {
      return null;
    }
  }
}

class MqttService {
  // MQTT Broker Configuration - using WebSocket
  static const String kBrokerWsUrl = 'ws://100.64.0.1:9001';
  static const String kBrokerHost = '100.64.0.1';
  static const int kBrokerPort = 9001; // WebSocket port
  
  MqttServerClient? _client;
  String? _playerId;
  
  // Connection state stream
  final _connectionStateController = StreamController<MqttConnectionState>.broadcast();
  Stream<MqttConnectionState> get connectionState => _connectionStateController.stream;
  
  // Message stream for specific topics
  final _messageController = StreamController<GameMessage>.broadcast();
  Stream<GameMessage> get messages => _messageController.stream;
  
  bool get isConnected => _client?.connectionStatus?.state == MqttConnectionState.connected;
  
  Future<void> initialize() async {
    // Load or generate player ID
    final prefs = await SharedPreferences.getInstance();
    _playerId = prefs.getString('ttt_pid');
    if (_playerId == null) {
      _playerId = _generateId();
      await prefs.setString('ttt_pid', _playerId!);
    }
  }
  
  String get playerId => _playerId ?? '';
  
  String _generateId() {
    const chars = 'abcdefghijklmnopqrstuvwxyz0123456789';
    final random = Random();
    return List.generate(12, (_) => chars[random.nextInt(chars.length)]).join();
  }
  
  Future<void> connect() async {
    if (_playerId == null) {
      await initialize();
    }
    
    try {
      final clientId = 'flutter-$_playerId';
      
      // Create client - pass full ws:// URL as host for WebSocket
      _client = MqttServerClient(kBrokerWsUrl, clientId);
      
      _client!.port = kBrokerPort;
      _client!.useWebSocket = true;
      _client!.websocketProtocols = MqttClientConstants.protocolsSingleDefault;
      _client!.setProtocolV311();
      _client!.logging(on: false);
      _client!.keepAlivePeriod = 60;
      _client!.connectTimeoutPeriod = 10000;
      _client!.autoReconnect = true;
      _client!.resubscribeOnAutoReconnect = true;
      
      // For web, we need to disable secure connection context
      if (kIsWeb) {
        _client!.secure = false;
      }
      
      _client!.onConnected = _onConnected;
      _client!.onDisconnected = _onDisconnected;
      _client!.onSubscribed = _onSubscribed;
      _client!.onAutoReconnect = _onAutoReconnect;
      _client!.onAutoReconnected = _onAutoReconnected;
      _client!.pongCallback = _onPong;
      
      final connMessage = MqttConnectMessage()
          .withWillTopic('ttt/lobby/leave/$_playerId')
          .withWillMessage(jsonEncode({'pid': _playerId}))
          .withWillQos(MqttQos.atMostOnce)
          .startClean()
          .withClientIdentifier(clientId);
      
      _client!.connectionMessage = connMessage;
      
      await _client!.connect();
    } catch (e) {
      print('[MQTT] Connection error: $e');
      _connectionStateController.add(MqttConnectionState.disconnected);
      disconnect();
    }
  }
  
  void _onConnected() {
    print('[MQTT] Connected to broker');
    if (!_connectionStateController.isClosed) {
      _connectionStateController.add(MqttConnectionState.connected);
    }
    
    // Subscribe to personal topics
    subscribe('ttt/game/$_playerId/created');
    subscribe('ttt/lobby/invite/$_playerId');
    subscribe('ttt/sys/players/online');
    
    // Listen to all messages
    _client!.updates!.listen((List<MqttReceivedMessage<MqttMessage>>? messages) {
      if (messages == null) return;
      for (final msg in messages) {
        final payload = msg.payload as MqttPublishMessage;
        final message = String.fromCharCodes(payload.payload.message);
        
        print('[MQTT] Received: ${msg.topic} -> $message');
        
        if (!_messageController.isClosed) {
          _messageController.add(GameMessage(
            topic: msg.topic,
            payload: message,
          ));
        }
      }
    });
  }
  
  void _onDisconnected() {
    print('[MQTT] Disconnected - attempting to reconnect...');
    if (!_connectionStateController.isClosed) {
      _connectionStateController.add(MqttConnectionState.disconnected);
    }
    
    // Reconnect if autoReconnect fails
    Future.delayed(const Duration(seconds: 2), () {
      if (_client?.connectionStatus?.state != MqttConnectionState.connected) {
        print('[MQTT] Reconnecting...');
        connect();
      }
    });
  }
  
  void _onSubscribed(String topic) {
    print('[MQTT] Subscribed to $topic');
  }
  
  void _onAutoReconnect() {
    print('[MQTT] Auto-reconnecting...');
  }
  
  void _onAutoReconnected() {
    print('[MQTT] Auto-reconnected successfully');
    if (!_connectionStateController.isClosed) {
      _connectionStateController.add(MqttConnectionState.connected);
    }
  }
  
  void _onPong() {
    // Pong received - connection is alive
  }
  
  void subscribe(String topic) {
    if (_client != null && isConnected) {
      _client!.subscribe(topic, MqttQos.atMostOnce);
    }
  }
  
  void unsubscribe(String topic) {
    if (_client != null && isConnected) {
      _client!.unsubscribe(topic);
    }
  }
  
  void publish(String topic, Map<String, dynamic> payload) {
    if (_client != null && isConnected) {
      final builder = MqttClientPayloadBuilder();
      builder.addString(jsonEncode(payload));
      _client!.publishMessage(topic, MqttQos.atMostOnce, builder.payload!);
      print('[MQTT] Published to $topic: $payload');
    }
  }
  
  void announcePresence(String playerName) {
    print('[MQTT] Announcing presence: pid=$_playerId, name=$playerName');
    publish('ttt/lobby/announce', {
      'pid': _playerId,
      'name': playerName,
      'client': 'flutter',
    });
  }
  
  void requestNewGame(String playerName, String mode) {
    publish('ttt/game/new', {
      'pid': _playerId,
      'name': playerName,
      'mode': mode, // 'ai' or 'human'
    });
  }
  
  void sendMove(String gameId, int cellIndex) {
    publish('ttt/game/$gameId/move', {
      'player': _playerId,
      'cell': cellIndex,
    });
  }
  
  void sendInvite(String targetPlayerId, String fromName) {
    publish('ttt/lobby/invite/$targetPlayerId', {
      'from_pid': _playerId,
      'from_name': fromName,
    });
  }
  
  void respondToInvite(String inviterPlayerId, String myName, bool accepted) {
    publish('ttt/lobby/response/$inviterPlayerId', {
      'pid': _playerId,
      'name': myName,
      'accepted': accepted,
    });
  }
  
  void leaveLobby() {
    publish('ttt/lobby/leave/$_playerId', {
      'pid': _playerId,
    });
  }
  
  void disconnect() {
    _client?.disconnect();
  }
  
  void dispose() {
    // Only close streams when truly disposing
    disconnect();
    if (!_connectionStateController.isClosed) {
      _connectionStateController.close();
    }
    if (!_messageController.isClosed) {
      _messageController.close();
    }
  }
}


