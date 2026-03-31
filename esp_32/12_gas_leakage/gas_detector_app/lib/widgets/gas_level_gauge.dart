import 'package:flutter/material.dart';
import 'dart:math' as math;

class GasLevelGauge extends StatelessWidget {
  final int ppm;
  final int warningThreshold;
  final int dangerThreshold;

  const GasLevelGauge({
    Key? key,
    required this.ppm,
    this.warningThreshold = 300,
    this.dangerThreshold = 600,
  }) : super(key: key);

  // Dynamically compute max so the gauge always has headroom above current ppm
  int get _maxPpm {
    const minimumMax = 1000;
    if (ppm <= minimumMax) return minimumMax;
    // Round up to next clean multiple of 1000
    return ((ppm / 1000).ceil() + 1) * 1000;
  }

  @override
  Widget build(BuildContext context) {
    return CustomPaint(
      size: const Size(240, 240),
      painter: GaugePainter(
        ppm: ppm,
        maxPpm: _maxPpm,
        warningThreshold: warningThreshold,
        dangerThreshold: dangerThreshold,
      ),
    );
  }
}

class GaugePainter extends CustomPainter {
  final int ppm;
  final int maxPpm;
  final int warningThreshold;
  final int dangerThreshold;

  GaugePainter({
    required this.ppm,
    required this.maxPpm,
    required this.warningThreshold,
    required this.dangerThreshold,
  });

  @override
  void paint(Canvas canvas, Size size) {
    final center = Offset(size.width / 2, size.height / 2);
    final radius = math.min(size.width, size.height) / 2 - 16;

    // ── Background track ──────────────────────────────────────────────────
    final bgPaint = Paint()
      ..color = Colors.grey.shade800
      ..style = PaintingStyle.stroke
      ..strokeWidth = 18
      ..strokeCap = StrokeCap.round;

    canvas.drawArc(
      Rect.fromCircle(center: center, radius: radius),
      -math.pi * 0.75,
      math.pi * 1.5,
      false,
      bgPaint,
    );

    // ── Colored zone sections (faint) ─────────────────────────────────────
    _drawZone(canvas, center, radius, 0,
        warningThreshold / maxPpm, const Color(0xFF4CAF50));
    _drawZone(canvas, center, radius, warningThreshold / maxPpm,
        dangerThreshold / maxPpm, const Color(0xFFFF9800));
    _drawZone(canvas, center, radius, dangerThreshold / maxPpm,
        1.0, const Color(0xFFF44336));

    // ── Active arc ────────────────────────────────────────────────────────
    final fraction = (ppm / maxPpm).clamp(0.0, 1.0);
    if (fraction > 0) {
      final activePaint = Paint()
        ..color = _arcColor()
        ..style = PaintingStyle.stroke
        ..strokeWidth = 22
        ..strokeCap = StrokeCap.round;

      canvas.drawArc(
        Rect.fromCircle(center: center, radius: radius),
        -math.pi * 0.75,
        fraction * math.pi * 1.5,
        false,
        activePaint,
      );
    }

    // ── Inner circle background ───────────────────────────────────────────
    final innerPaint = Paint()
      ..color = const Color(0xFF1A1A1A)
      ..style = PaintingStyle.fill;
    canvas.drawCircle(center, radius - 30, innerPaint);

    // ── PPM number ────────────────────────────────────────────────────────
    final ppmStr = ppm.toString();
    // Scale font size based on digit count so it always fits
    final fontSize = ppmStr.length <= 3 ? 52.0 : ppmStr.length == 4 ? 42.0 : 34.0;

    final ppmPainter = TextPainter(
      text: TextSpan(
        text: ppmStr,
        style: TextStyle(
          color: _arcColor(),
          fontSize: fontSize,
          fontWeight: FontWeight.bold,
        ),
      ),
      textDirection: TextDirection.ltr,
    )..layout();

    ppmPainter.paint(
      canvas,
      Offset(
        center.dx - ppmPainter.width / 2,
        center.dy - ppmPainter.height / 2 - 8,
      ),
    );

    // ── "PPM" label ───────────────────────────────────────────────────────
    final labelPainter = TextPainter(
      text: const TextSpan(
        text: 'PPM',
        style: TextStyle(color: Colors.grey, fontSize: 15),
      ),
      textDirection: TextDirection.ltr,
    )..layout();

    labelPainter.paint(
      canvas,
      Offset(center.dx - labelPainter.width / 2, center.dy + fontSize / 2 - 4),
    );

    // ── Range label at bottom right of arc ────────────────────────────────
    final maxPainter = TextPainter(
      text: TextSpan(
        text: 'max $maxPpm',
        style: const TextStyle(color: Colors.grey, fontSize: 11),
      ),
      textDirection: TextDirection.ltr,
    )..layout();

    maxPainter.paint(
      canvas,
      Offset(
        center.dx + radius * math.cos(math.pi * 0.75) - maxPainter.width / 2,
        center.dy + radius * math.sin(math.pi * 0.75) + 4,
      ),
    );
  }

  void _drawZone(Canvas canvas, Offset center, double radius,
      double start, double end, Color color) {
    if (end <= start) return;
    final paint = Paint()
      ..color = color.withOpacity(0.25)
      ..style = PaintingStyle.stroke
      ..strokeWidth = 18
      ..strokeCap = StrokeCap.butt;

    canvas.drawArc(
      Rect.fromCircle(center: center, radius: radius),
      -math.pi * 0.75 + start * math.pi * 1.5,
      (end - start) * math.pi * 1.5,
      false,
      paint,
    );
  }

  Color _arcColor() {
    if (ppm >= dangerThreshold) return const Color(0xFFF44336);
    if (ppm >= warningThreshold) return const Color(0xFFFF9800);
    return const Color(0xFF4CAF50);
  }

  @override
  bool shouldRepaint(GaugePainter old) =>
      old.ppm != ppm || old.maxPpm != maxPpm;
}
