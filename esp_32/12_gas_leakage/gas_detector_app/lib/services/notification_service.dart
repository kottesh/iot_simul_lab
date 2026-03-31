import 'package:flutter/material.dart';
import 'package:flutter_local_notifications/flutter_local_notifications.dart';

class NotificationService {
  static final NotificationService _instance = NotificationService._internal();
  factory NotificationService() => _instance;
  NotificationService._internal();

  final FlutterLocalNotificationsPlugin _notifications = FlutterLocalNotificationsPlugin();
  bool _initialized = false;

  Future<void> initialize() async {
    if (_initialized) return;

    const androidSettings = AndroidInitializationSettings('@mipmap/ic_launcher');
    const iosSettings = DarwinInitializationSettings(
      requestAlertPermission: true,
      requestBadgePermission: true,
      requestSoundPermission: true,
    );

    const initSettings = InitializationSettings(
      android: androidSettings,
      iOS: iosSettings,
    );

    await _notifications.initialize(
      initSettings,
      onDidReceiveNotificationResponse: _onNotificationTap,
    );

    _initialized = true;
    print('[Notifications] Initialized');
  }

  void _onNotificationTap(NotificationResponse response) {
    print('[Notifications] Tapped: ${response.payload}');
  }

  Future<void> showSafeNotification() async {
    final androidDetails = AndroidNotificationDetails(
      'gas_status',
      'Gas Status',
      channelDescription: 'Gas detector status notifications',
      importance: Importance.defaultImportance,
      priority: Priority.defaultPriority,
      color: const Color(0xFF4CAF50),
      playSound: false,
    );

    const iosDetails = DarwinNotificationDetails();

    final details = NotificationDetails(
      android: androidDetails,
      iOS: iosDetails,
    );

    await _notifications.show(
      1,
      'Gas Levels Safe',
      'All gas levels have returned to normal',
      details,
      payload: 'SAFE',
    );
  }

  Future<void> showWarningNotification(int ppm) async {
    final androidDetails = AndroidNotificationDetails(
      'gas_warning',
      'Gas Warning',
      channelDescription: 'Gas detector warning notifications',
      importance: Importance.high,
      priority: Priority.high,
      color: const Color(0xFFFF9800),
      playSound: true,
      enableVibration: true,
    );

    const iosDetails = DarwinNotificationDetails(
      presentAlert: true,
      presentBadge: true,
      presentSound: true,
    );

    final details = NotificationDetails(
      android: androidDetails,
      iOS: iosDetails,
    );

    await _notifications.show(
      2,
      '⚠️ Gas Levels Elevated',
      'Warning: Gas detected at $ppm PPM',
      details,
      payload: 'WARNING',
    );
  }

  Future<void> showDangerNotification(int ppm, String message) async {
    final androidDetails = AndroidNotificationDetails(
      'gas_danger',
      'Gas Danger Alert',
      channelDescription: 'Critical gas detector alerts',
      importance: Importance.max,
      priority: Priority.max,
      color: const Color(0xFFF44336),
      playSound: true,
      enableVibration: true,
      ongoing: true,
      autoCancel: false,
      styleInformation: const BigTextStyleInformation(''),
    );

    const iosDetails = DarwinNotificationDetails(
      presentAlert: true,
      presentBadge: true,
      presentSound: true,
      interruptionLevel: InterruptionLevel.critical,
    );

    final details = NotificationDetails(
      android: androidDetails,
      iOS: iosDetails,
    );

    await _notifications.show(
      3,
      '🚨 DANGER - Gas Leakage!',
      message,
      details,
      payload: 'DANGER',
    );
  }

  Future<void> showValveClosedNotification() async {
    final androidDetails = AndroidNotificationDetails(
      'gas_system',
      'System Actions',
      channelDescription: 'Gas detector system action notifications',
      importance: Importance.high,
      priority: Priority.high,
      color: const Color(0xFF2196F3),
    );

    const iosDetails = DarwinNotificationDetails();

    final details = NotificationDetails(
      android: androidDetails,
      iOS: iosDetails,
    );

    await _notifications.show(
      4,
      'Gas Valve Closed',
      'The gas valve has been automatically closed due to high gas levels',
      details,
      payload: 'VALVE_CLOSED',
    );
  }

  Future<void> cancelAll() async {
    await _notifications.cancelAll();
  }

  Future<void> cancelNotification(int id) async {
    await _notifications.cancel(id);
  }
}
