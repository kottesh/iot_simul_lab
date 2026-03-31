import 'dart:async';
import 'dart:convert';
import 'package:mqtt_client/mqtt_client.dart';
import 'package:mqtt_client/mqtt_server_client.dart';

typedef MessageCallback = void Function(String topic, Map<String, dynamic> payload);
typedef ConnectionCallback = void Function(bool connected);

class MqttService {
  MqttServerClient? _client;
  bool _listenersSetup = false;

  MessageCallback? onMessage;
  ConnectionCallback? onConnectionChanged;

  bool get isConnected =>
      _client?.connectionStatus?.state == MqttConnectionState.connected;

  Future<bool> connect(String broker, int port, String clientId) async {
    // Disconnect old client cleanly
    try {
      _client?.disconnect();
    } catch (_) {}
    _client = null;
    _listenersSetup = false;

    _client = MqttServerClient.withPort(broker, clientId, port);
    _client!.logging(on: false);
    _client!.keepAlivePeriod = 20;
    _client!.connectTimeoutPeriod = 5000;
    _client!.autoReconnect = false;
    _client!.onDisconnected = _onDisconnected;
    _client!.onConnected = _onConnected;

    final connMessage = MqttConnectMessage()
        .withClientIdentifier(clientId)
        .startClean()
        .withWillQos(MqttQos.atLeastOnce);
    _client!.connectionMessage = connMessage;

    try {
      print('[MQTT] Connecting to $broker:$port as $clientId...');
      await _client!.connect();

      if (_client!.connectionStatus?.state == MqttConnectionState.connected) {
        print('[MQTT] Connected!');
        _setupSubscriptions();
        return true;
      } else {
        print('[MQTT] Failed: ${_client!.connectionStatus}');
        _client!.disconnect();
        return false;
      }
    } catch (e) {
      print('[MQTT] Exception: $e');
      try { _client?.disconnect(); } catch (_) {}
      return false;
    }
  }

  void _setupSubscriptions() {
    if (_listenersSetup) return;
    _listenersSetup = true;

    final topics = [
      'home/gas/level',
      'home/gas/status',
      'home/gas/alert',
      'home/gas/valve',
      'home/gas/fan',
      'home/gas/system/health',
    ];

    for (final topic in topics) {
      _client!.subscribe(topic, MqttQos.atLeastOnce);
      print('[MQTT] Subscribed to $topic');
    }

    _client!.updates!.listen((List<MqttReceivedMessage<MqttMessage>> messages) {
      for (final message in messages) {
        final recMessage = message.payload as MqttPublishMessage;
        final raw = MqttPublishPayload.bytesToStringAsString(
            recMessage.payload.message);
        print('[MQTT IN] ${message.topic} -> $raw');
        try {
          final decoded = jsonDecode(raw) as Map<String, dynamic>;
          onMessage?.call(message.topic, decoded);
        } catch (e) {
          print('[MQTT] JSON parse error on ${message.topic}: $e');
        }
      }
    });
  }

  void _onConnected() {
    print('[MQTT] onConnected');
    onConnectionChanged?.call(true);
  }

  void _onDisconnected() {
    print('[MQTT] onDisconnected');
    _listenersSetup = false;
    onConnectionChanged?.call(false);
  }

  void publishCommand(String command, {Map<String, dynamic>? params}) {
    if (!isConnected) {
      print('[MQTT] Not connected — cannot publish');
      return;
    }
    final payload = <String, dynamic>{'command': command};
    if (params != null) payload.addAll(params);

    final builder = MqttClientPayloadBuilder();
    builder.addString(jsonEncode(payload));
    _client!.publishMessage(
      'home/gas/control',
      MqttQos.atLeastOnce,
      builder.payload!,
    );
    print('[MQTT OUT] command=$command params=$params');
  }

  void disconnect() {
    try { _client?.disconnect(); } catch (_) {}
    _listenersSetup = false;
  }

  void dispose() {
    disconnect();
  }
}
