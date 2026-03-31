import 'package:flutter/material.dart';
import 'package:flutter_animate/flutter_animate.dart';
import '../models/gas_detector_model.dart';
import '../utils/app_theme.dart';

class StatusCard extends StatelessWidget {
  final GasState status;
  final int ppm;

  const StatusCard({
    Key? key,
    required this.status,
    required this.ppm,
  }) : super(key: key);

  @override
  Widget build(BuildContext context) {
    final statusText = _getStatusText();
    final color = _getColor();
    final icon = _getIcon();

    return Card(
      color: color.withOpacity(0.2),
      child: Container(
        padding: const EdgeInsets.all(24),
        decoration: BoxDecoration(
          borderRadius: BorderRadius.circular(12),
          border: Border.all(color: color, width: 2),
        ),
        child: Column(
          children: [
            Icon(icon, size: 64, color: color)
                .animate(onPlay: (controller) => controller.repeat())
                .shimmer(
                  duration: status == GasState.danger ? 1000.ms : 2000.ms,
                  color: color.withOpacity(0.5),
                ),
            const SizedBox(height: 16),
            Text(
              statusText,
              style: TextStyle(
                fontSize: 32,
                fontWeight: FontWeight.bold,
                color: color,
              ),
            ),
            const SizedBox(height: 8),
            Text(
              _getMessage(),
              style: TextStyle(
                fontSize: 14,
                color: Colors.grey.shade400,
              ),
              textAlign: TextAlign.center,
            ),
          ],
        ),
      ),
    );
  }

  String _getStatusText() {
    switch (status) {
      case GasState.safe:
        return 'SAFE';
      case GasState.warning:
        return 'WARNING';
      case GasState.danger:
        return 'DANGER';
    }
  }

  Color _getColor() {
    switch (status) {
      case GasState.safe:
        return AppTheme.safeColor;
      case GasState.warning:
        return AppTheme.warningColor;
      case GasState.danger:
        return AppTheme.dangerColor;
    }
  }

  IconData _getIcon() {
    switch (status) {
      case GasState.safe:
        return Icons.check_circle;
      case GasState.warning:
        return Icons.warning;
      case GasState.danger:
        return Icons.dangerous;
    }
  }

  String _getMessage() {
    switch (status) {
      case GasState.safe:
        return 'All systems normal';
      case GasState.warning:
        return 'Elevated gas levels detected';
      case GasState.danger:
        return 'EVACUATE IMMEDIATELY!';
    }
  }
}
