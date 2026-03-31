import 'package:flutter/material.dart';

class AppTheme {
  static const Color safeColor = Color(0xFF4CAF50);
  static const Color warningColor = Color(0xFFFF9800);
  static const Color dangerColor = Color(0xFFF44336);
  static const Color primaryColor = Color(0xFF2196F3);
  static const Color backgroundColor = Color(0xFF121212);
  static const Color surfaceColor = Color(0xFF1E1E1E);
  static const Color cardColor = Color(0xFF2C2C2C);

  static ThemeData darkTheme = ThemeData(
    useMaterial3: true,
    brightness: Brightness.dark,
    colorScheme: ColorScheme.dark(
      primary: primaryColor,
      secondary: primaryColor,
      surface: surfaceColor,
      background: backgroundColor,
      error: dangerColor,
    ),
    scaffoldBackgroundColor: backgroundColor,
    cardTheme: CardThemeData(
      color: cardColor,
      elevation: 4,
      shape: RoundedRectangleBorder(
        borderRadius: BorderRadius.circular(12),
      ),
    ),
    elevatedButtonTheme: ElevatedButtonThemeData(
      style: ElevatedButton.styleFrom(
        padding: const EdgeInsets.symmetric(horizontal: 24, vertical: 12),
        shape: RoundedRectangleBorder(
          borderRadius: BorderRadius.circular(8),
        ),
      ),
    ),
    appBarTheme: const AppBarTheme(
      backgroundColor: surfaceColor,
      elevation: 0,
    ),
  );

  static Color getStatusColor(String status) {
    switch (status.toUpperCase()) {
      case 'SAFE':
        return safeColor;
      case 'WARNING':
        return warningColor;
      case 'DANGER':
        return dangerColor;
      default:
        return Colors.grey;
    }
  }
}
