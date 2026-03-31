import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import 'providers/gas_detector_provider.dart';
import 'screens/dashboard_screen.dart';
import 'screens/controls_screen.dart';
import 'screens/settings_screen.dart';
import 'screens/history_screen.dart';
import 'utils/app_theme.dart';

void main() {
  WidgetsFlutterBinding.ensureInitialized();
  runApp(const MyApp());
}

class MyApp extends StatelessWidget {
  const MyApp({super.key});

  @override
  Widget build(BuildContext context) {
    return ChangeNotifierProvider(
      // Create the provider WITHOUT calling connect() here.
      // MainScreen calls connect() in initState() after the tree is mounted.
      create: (_) => GasDetectorProvider(),
      child: MaterialApp(
        title: 'Gas Detector',
        theme: AppTheme.darkTheme,
        debugShowCheckedModeBanner: false,
        home: const MainScreen(),
      ),
    );
  }
}

class MainScreen extends StatefulWidget {
  const MainScreen({super.key});

  @override
  State<MainScreen> createState() => _MainScreenState();
}

class _MainScreenState extends State<MainScreen> {
  int _currentIndex = 0;

  final List<Widget> _screens = const [
    DashboardScreen(),
    ControlsScreen(),
    HistoryScreen(),
    SettingsScreen(),
  ];

  @override
  void initState() {
    super.initState();
    // Connect after the first frame so WidgetsBinding is fully ready
    // and notifyListeners() can safely rebuild widgets.
    WidgetsBinding.instance.addPostFrameCallback((_) {
      context.read<GasDetectorProvider>().connect();
    });
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      body: _screens[_currentIndex],
      bottomNavigationBar: NavigationBar(
        selectedIndex: _currentIndex,
        onDestinationSelected: (index) {
          setState(() => _currentIndex = index);
        },
        destinations: const [
          NavigationDestination(
            icon: Icon(Icons.dashboard),
            label: 'Dashboard',
          ),
          NavigationDestination(
            icon: Icon(Icons.tune),
            label: 'Controls',
          ),
          NavigationDestination(
            icon: Icon(Icons.history),
            label: 'History',
          ),
          NavigationDestination(
            icon: Icon(Icons.settings),
            label: 'Settings',
          ),
        ],
      ),
    );
  }
}
