import 'package:shared_preferences/shared_preferences.dart';

class StorageService {
  static const String _keyBroker = 'mqtt_broker';
  static const String _keyPort = 'mqtt_port';
  static const String _keyClientId = 'mqtt_client_id';
  static const String _keyThresholdWarning = 'threshold_warning';
  static const String _keyThresholdDanger = 'threshold_danger';
  static const String _keyAutoRecovery = 'auto_recovery';

  Future<String> getMqttBroker() async {
    final prefs = await SharedPreferences.getInstance();
    return prefs.getString(_keyBroker) ?? '100.64.0.1';
  }

  Future<void> setMqttBroker(String broker) async {
    final prefs = await SharedPreferences.getInstance();
    await prefs.setString(_keyBroker, broker);
  }

  Future<int> getMqttPort() async {
    final prefs = await SharedPreferences.getInstance();
    return prefs.getInt(_keyPort) ?? 1883;
  }

  Future<void> setMqttPort(int port) async {
    final prefs = await SharedPreferences.getInstance();
    await prefs.setInt(_keyPort, port);
  }

  Future<String> getMqttClientId() async {
    final prefs = await SharedPreferences.getInstance();
    return prefs.getString(_keyClientId) ?? 'FlutterGasDetectorApp';
  }

  Future<void> setMqttClientId(String clientId) async {
    final prefs = await SharedPreferences.getInstance();
    await prefs.setString(_keyClientId, clientId);
  }

  Future<int> getThresholdWarning() async {
    final prefs = await SharedPreferences.getInstance();
    return prefs.getInt(_keyThresholdWarning) ?? 300;
  }

  Future<void> setThresholdWarning(int threshold) async {
    final prefs = await SharedPreferences.getInstance();
    await prefs.setInt(_keyThresholdWarning, threshold);
  }

  Future<int> getThresholdDanger() async {
    final prefs = await SharedPreferences.getInstance();
    return prefs.getInt(_keyThresholdDanger) ?? 600;
  }

  Future<void> setThresholdDanger(int threshold) async {
    final prefs = await SharedPreferences.getInstance();
    await prefs.setInt(_keyThresholdDanger, threshold);
  }

  Future<bool> getAutoRecovery() async {
    final prefs = await SharedPreferences.getInstance();
    return prefs.getBool(_keyAutoRecovery) ?? false;
  }

  Future<void> setAutoRecovery(bool enabled) async {
    final prefs = await SharedPreferences.getInstance();
    await prefs.setBool(_keyAutoRecovery, enabled);
  }
}
