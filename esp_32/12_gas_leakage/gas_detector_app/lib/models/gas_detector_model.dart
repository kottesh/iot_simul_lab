class GasDetectorModel {
  final int ppm;
  final GasState status;
  final bool valveOpen;
  final FanState fanState;
  final int fanAngle;
  final bool buzzerSilenced;
  final bool autoRecovery;
  final int thresholdWarning;
  final int thresholdDanger;
  final String deviceIp;
  final int uptimeSeconds;
  final bool isOnline;
  final DateTime lastUpdate;
  final AlertData? activeAlert;

  GasDetectorModel({
    this.ppm = 0,
    this.status = GasState.safe,
    this.valveOpen = true,
    this.fanState = FanState.off,
    this.fanAngle = 0,
    this.buzzerSilenced = false,
    this.autoRecovery = false,
    this.thresholdWarning = 300,
    this.thresholdDanger = 600,
    this.deviceIp = '',
    this.uptimeSeconds = 0,
    this.isOnline = false,
    DateTime? lastUpdate,
    this.activeAlert,
  }) : lastUpdate = lastUpdate ?? DateTime.now();

  GasDetectorModel copyWith({
    int? ppm,
    GasState? status,
    bool? valveOpen,
    FanState? fanState,
    int? fanAngle,
    bool? buzzerSilenced,
    bool? autoRecovery,
    int? thresholdWarning,
    int? thresholdDanger,
    String? deviceIp,
    int? uptimeSeconds,
    bool? isOnline,
    DateTime? lastUpdate,
    AlertData? activeAlert,
    bool clearAlert = false,
  }) {
    return GasDetectorModel(
      ppm: ppm ?? this.ppm,
      status: status ?? this.status,
      valveOpen: valveOpen ?? this.valveOpen,
      fanState: fanState ?? this.fanState,
      fanAngle: fanAngle ?? this.fanAngle,
      buzzerSilenced: buzzerSilenced ?? this.buzzerSilenced,
      autoRecovery: autoRecovery ?? this.autoRecovery,
      thresholdWarning: thresholdWarning ?? this.thresholdWarning,
      thresholdDanger: thresholdDanger ?? this.thresholdDanger,
      deviceIp: deviceIp ?? this.deviceIp,
      uptimeSeconds: uptimeSeconds ?? this.uptimeSeconds,
      isOnline: isOnline ?? this.isOnline,
      lastUpdate: lastUpdate ?? DateTime.now(),
      activeAlert: clearAlert ? null : (activeAlert ?? this.activeAlert),
    );
  }

  String get uptimeFormatted {
    final hours = uptimeSeconds ~/ 3600;
    final minutes = (uptimeSeconds % 3600) ~/ 60;
    final seconds = uptimeSeconds % 60;
    return '${hours}h ${minutes}m ${seconds}s';
  }

  String get statusText {
    switch (status) {
      case GasState.safe:
        return 'SAFE';
      case GasState.warning:
        return 'WARNING';
      case GasState.danger:
        return 'DANGER';
    }
  }

  String get fanStateText {
    switch (fanState) {
      case FanState.off:
        return 'OFF';
      case FanState.partial:
        return 'PARTIAL';
      case FanState.on:
        return 'ON';
    }
  }
}

enum GasState {
  safe,
  warning,
  danger,
}

enum FanState {
  off,
  partial,
  on,
}

class AlertData {
  final bool alert;
  final String level;
  final int ppm;
  final String message;
  final DateTime timestamp;

  AlertData({
    required this.alert,
    required this.level,
    required this.ppm,
    required this.message,
    DateTime? timestamp,
  }) : timestamp = timestamp ?? DateTime.now();

  factory AlertData.fromJson(Map<String, dynamic> json) {
    return AlertData(
      alert: json['alert'] ?? false,
      level: json['level'] ?? 'DANGER',
      ppm: json['ppm'] ?? 0,
      message: json['message'] ?? '',
    );
  }
}

class HistoryEntry {
  final DateTime timestamp;
  final int ppm;
  final GasState status;

  HistoryEntry({
    required this.timestamp,
    required this.ppm,
    required this.status,
  });
}
