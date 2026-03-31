import 'dart:async';
import 'package:flutter/foundation.dart';
import 'package:flutter/widgets.dart';
import 'package:mqtt_client/mqtt_client.dart';
import '../models/gas_detector_model.dart';
import '../services/mqtt_service.dart';
import '../services/notification_service.dart';
import '../services/storage_service.dart';

class GasDetectorProvider extends ChangeNotifier {
  final MqttService _mqttService = MqttService();
  final NotificationService _notificationService = NotificationService();
  final StorageService _storageService = StorageService();

  GasDetectorModel _state = GasDetectorModel();
  final List<HistoryEntry> _history = [];

  bool _isConnected = false;
  bool _isConnecting = false;
  String _connectionError = '';

  GasDetectorModel get state => _state;
  List<HistoryEntry> get history => List.unmodifiable(_history);
  bool get isConnected => _isConnected;
  bool get isConnecting => _isConnecting;
  String get connectionError => _connectionError;

  GasDetectorProvider() {
    // Set callbacks SYNCHRONOUSLY before anything async runs.
    // This guarantees onMessage is wired before connect() ever resolves.
    _mqttService.onMessage = _onMessage;
    _mqttService.onConnectionChanged = _onConnectionChanged;

    // Initialize notifications in background — no need to await
    _notificationService.initialize().catchError((e) {
      debugPrint('[Notifications] init error: $e');
    });
  }

  // ─── Safe notifyListeners ─────────────────────────────────────────────────
  // mqtt_client runs on the same Dart isolate (event loop) as Flutter,
  // so notifyListeners() is safe to call directly. We just need to avoid
  // calling it during a build phase.
  void _safeNotify() {
    // If we're in a build/layout phase, defer to next microtask.
    if (WidgetsBinding.instance.debugBuildingDirtyElements) {
      Future.microtask(notifyListeners);
    } else {
      notifyListeners();
    }
  }

  // ─── MQTT callbacks ───────────────────────────────────────────────────────

  void _onConnectionChanged(bool connected) {
    _isConnected = connected;
    _state = _state.copyWith(isOnline: connected);
    _safeNotify();
  }

  void _onMessage(String topic, Map<String, dynamic> payload) {
    switch (topic) {
      case 'home/gas/level':
        _handleLevel(payload);
        break;
      case 'home/gas/status':
        _handleStatus(payload);
        break;
      case 'home/gas/alert':
        _handleAlert(payload);
        break;
      case 'home/gas/valve':
        _handleValve(payload);
        break;
      case 'home/gas/fan':
        _handleFan(payload);
        break;
      case 'home/gas/system/health':
        _handleHealth(payload);
        break;
    }
    // Always notify after every message so the UI rebuilds immediately
    _safeNotify();
  }

  // ─── Connection ───────────────────────────────────────────────────────────

  Future<bool> connect() async {
    if (_isConnecting) return false;

    final broker = await _storageService.getMqttBroker();
    final port = await _storageService.getMqttPort();
    final clientId = await _storageService.getMqttClientId();

    _isConnecting = true;
    _connectionError = '';
    notifyListeners();

    final success = await _mqttService.connect(broker, port, clientId);

    _isConnecting = false;
    _isConnected = success;
    _state = _state.copyWith(isOnline: success);

    if (!success) {
      _connectionError = 'Could not connect to $broker:$port';
    }

    notifyListeners();
    return success;
  }

  void disconnect() {
    _mqttService.disconnect();
    _isConnected = false;
    _state = _state.copyWith(isOnline: false);
    notifyListeners();
  }

  // ─── Message handlers ─────────────────────────────────────────────────────

  /// {"ppm":3726,"unit":"ppm","uptime_s":36}
  void _handleLevel(Map<String, dynamic> payload) {
    final ppm = (payload['ppm'] as num?)?.toInt() ?? 0;
    final uptime = (payload['uptime_s'] as num?)?.toInt() ?? 0;
    final newStatus = _ppmToState(ppm);
    final prevStatus = _state.status;

    _state = _state.copyWith(
      ppm: ppm,
      uptimeSeconds: uptime,
      status: newStatus,
    );

    _history.add(HistoryEntry(
      timestamp: DateTime.now(),
      ppm: ppm,
      status: newStatus,
    ));
    if (_history.length > 100) _history.removeAt(0);

    if (prevStatus != newStatus) {
      _triggerNotification(prevStatus, newStatus, ppm);
    }
  }

  /// {"status":"DANGER","ppm":3726}  — only sent on state change
  void _handleStatus(Map<String, dynamic> payload) {
    final statusStr = (payload['status'] as String?) ?? 'SAFE';
    final ppm = (payload['ppm'] as num?)?.toInt() ?? _state.ppm;
    final newStatus = _stringToState(statusStr);
    final prevStatus = _state.status;

    _state = _state.copyWith(status: newStatus, ppm: ppm);

    if (prevStatus != newStatus) {
      _triggerNotification(prevStatus, newStatus, ppm);
    }
  }

  /// {"alert":true,"level":"DANGER","ppm":1000,"message":"..."}
  void _handleAlert(Map<String, dynamic> payload) {
    if (payload['alert'] != true) return;
    final alert = AlertData.fromJson(payload);
    _state = _state.copyWith(
      activeAlert: alert,
      status: GasState.danger,
      ppm: alert.ppm,
    );
    _notificationService.showDangerNotification(alert.ppm, alert.message);
  }

  /// {"state":"CLOSED","reason":"DANGER_AUTO"}
  void _handleValve(Map<String, dynamic> payload) {
    final stateStr = (payload['state'] as String?) ?? 'OPEN';
    final wasOpen = _state.valveOpen;
    final isOpen = stateStr == 'OPEN';
    _state = _state.copyWith(valveOpen: isOpen);
    if (wasOpen && !isOpen) {
      _notificationService.showValveClosedNotification();
    }
  }

  /// {"state":"ON","angle":180,"reason":"DANGER_AUTO","ppm":1000}
  void _handleFan(Map<String, dynamic> payload) {
    final stateStr = (payload['state'] as String?) ?? 'OFF';
    final angle = (payload['angle'] as num?)?.toInt() ?? 0;
    FanState fanState;
    switch (stateStr) {
      case 'ON':
        fanState = FanState.on;
        break;
      case 'PARTIAL':
        fanState = FanState.partial;
        break;
      default:
        fanState = FanState.off;
    }
    _state = _state.copyWith(fanState: fanState, fanAngle: angle);
  }

  /// Full device snapshot every 30 s
  void _handleHealth(Map<String, dynamic> payload) {
    final valveStr = (payload['valve'] as String?) ?? 'OPEN';
    final fanAngle = (payload['fan_angle'] as num?)?.toInt() ?? 0;
    final autoRecovery = (payload['auto_recovery'] as bool?) ?? false;
    final buzzerSilenced = (payload['buzzer_silenced'] as bool?) ?? false;
    final thresholdWarning =
        (payload['threshold_warning'] as num?)?.toInt() ?? 300;
    final thresholdDanger =
        (payload['threshold_danger'] as num?)?.toInt() ?? 600;
    final uptime = (payload['uptime_s'] as num?)?.toInt() ?? 0;
    final ip = (payload['ip'] as String?) ?? '';

    FanState fanState;
    if (fanAngle >= 180) {
      fanState = FanState.on;
    } else if (fanAngle >= 90) {
      fanState = FanState.partial;
    } else {
      fanState = FanState.off;
    }

    _state = _state.copyWith(
      valveOpen: valveStr == 'OPEN',
      fanAngle: fanAngle,
      fanState: fanState,
      autoRecovery: autoRecovery,
      buzzerSilenced: buzzerSilenced,
      thresholdWarning: thresholdWarning,
      thresholdDanger: thresholdDanger,
      uptimeSeconds: uptime,
      deviceIp: ip,
      isOnline: true,
    );
  }

  // ─── Helpers ──────────────────────────────────────────────────────────────

  GasState _ppmToState(int ppm) {
    if (ppm >= _state.thresholdDanger) return GasState.danger;
    if (ppm >= _state.thresholdWarning) return GasState.warning;
    return GasState.safe;
  }

  GasState _stringToState(String s) {
    switch (s) {
      case 'DANGER':
        return GasState.danger;
      case 'WARNING':
        return GasState.warning;
      default:
        return GasState.safe;
    }
  }

  void _triggerNotification(GasState prev, GasState curr, int ppm) {
    if (curr == GasState.danger) {
      _notificationService.showDangerNotification(
          ppm, 'Gas leakage detected! Evacuate immediately.');
    } else if (curr == GasState.warning) {
      _notificationService.showWarningNotification(ppm);
    } else if (curr == GasState.safe && prev != GasState.safe) {
      _notificationService.showSafeNotification();
      _notificationService.cancelNotification(3);
    }
  }

  // ─── Commands ─────────────────────────────────────────────────────────────

  void openValve() => _mqttService.publishCommand('VALVE_OPEN');
  void closeValve() => _mqttService.publishCommand('VALVE_CLOSE');
  void setFanOff() => _mqttService.publishCommand('FAN_OFF');
  void setFanPartial() => _mqttService.publishCommand('FAN_PARTIAL');
  void setFanOn() => _mqttService.publishCommand('FAN_ON');
  void silenceBuzzer() => _mqttService.publishCommand('BUZZER_SILENCE');
  void enableBuzzer() => _mqttService.publishCommand('BUZZER_ENABLE');
  void testBuzzer() => _mqttService.publishCommand('BUZZER_TEST');
  void testLed() => _mqttService.publishCommand('LED_TEST');
  void testAlarm() => _mqttService.publishCommand('TEST_ALARM');
  void testWarning() => _mqttService.publishCommand('TEST_WARNING');
  void calibrateSensor() => _mqttService.publishCommand('SENSOR_CALIBRATE');
  void systemReset() => _mqttService.publishCommand('SYSTEM_RESET');
  void systemStatus() => _mqttService.publishCommand('SYSTEM_STATUS');
  void systemReboot() => _mqttService.publishCommand('SYSTEM_REBOOT');
  void healthPing() => _mqttService.publishCommand('HEALTH_PING');

  void setThresholds(int warning, int danger) {
    _mqttService.publishCommand('SET_THRESHOLD',
        params: {'warning': warning, 'danger': danger});
    _storageService.setThresholdWarning(warning);
    _storageService.setThresholdDanger(danger);
    _state = _state.copyWith(
        thresholdWarning: warning, thresholdDanger: danger);
    notifyListeners();
  }

  void setAutoRecovery(bool enabled) {
    _mqttService
        .publishCommand(enabled ? 'AUTO_RECOVERY_ON' : 'AUTO_RECOVERY_OFF');
    _storageService.setAutoRecovery(enabled);
    _state = _state.copyWith(autoRecovery: enabled);
    notifyListeners();
  }

  void clearAlert() {
    _state = _state.copyWith(clearAlert: true);
    _notificationService.cancelNotification(3);
    notifyListeners();
  }

  @override
  void dispose() {
    _mqttService.dispose();
    super.dispose();
  }
}
