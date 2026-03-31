import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../providers/gas_detector_provider.dart';

class ControlsScreen extends StatelessWidget {
  const ControlsScreen({Key? key}) : super(key: key);

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('Controls'),
      ),
      body: Consumer<GasDetectorProvider>(
        builder: (context, provider, _) {
          return SingleChildScrollView(
            padding: const EdgeInsets.all(16),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.stretch,
              children: [
                _buildValveControls(provider),
                const SizedBox(height: 16),
                _buildFanControls(provider),
                const SizedBox(height: 16),
                _buildBuzzerControls(provider),
                const SizedBox(height: 16),
                _buildTestControls(provider),
                const SizedBox(height: 16),
                _buildSystemControls(provider),
              ],
            ),
          );
        },
      ),
    );
  }

  Widget _buildValveControls(GasDetectorProvider provider) {
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Row(
              children: [
                const Icon(Icons.power, size: 24),
                const SizedBox(width: 12),
                const Text(
                  'Valve Controls',
                  style: TextStyle(fontSize: 18, fontWeight: FontWeight.bold),
                ),
              ],
            ),
            const Divider(height: 24),
            Row(
              children: [
                Expanded(
                  child: ElevatedButton.icon(
                    onPressed: () => provider.openValve(),
                    icon: const Icon(Icons.lock_open),
                    label: const Text('Open Valve'),
                    style: ElevatedButton.styleFrom(
                      backgroundColor: Colors.green,
                      padding: const EdgeInsets.symmetric(vertical: 16),
                    ),
                  ),
                ),
                const SizedBox(width: 12),
                Expanded(
                  child: ElevatedButton.icon(
                    onPressed: () => provider.closeValve(),
                    icon: const Icon(Icons.lock),
                    label: const Text('Close Valve'),
                    style: ElevatedButton.styleFrom(
                      backgroundColor: Colors.red,
                      padding: const EdgeInsets.symmetric(vertical: 16),
                    ),
                  ),
                ),
              ],
            ),
            const SizedBox(height: 12),
            Container(
              padding: const EdgeInsets.all(12),
              decoration: BoxDecoration(
                color: Colors.grey.shade900,
                borderRadius: BorderRadius.circular(8),
              ),
              child: Row(
                mainAxisAlignment: MainAxisAlignment.spaceBetween,
                children: [
                  const Text('Current State:'),
                  Text(
                    provider.state.valveOpen ? 'OPEN' : 'CLOSED',
                    style: TextStyle(
                      fontWeight: FontWeight.bold,
                      color: provider.state.valveOpen ? Colors.green : Colors.red,
                    ),
                  ),
                ],
              ),
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildFanControls(GasDetectorProvider provider) {
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Row(
              children: [
                const Icon(Icons.air, size: 24),
                const SizedBox(width: 12),
                const Text(
                  'Ventilation Fan Controls',
                  style: TextStyle(fontSize: 18, fontWeight: FontWeight.bold),
                ),
              ],
            ),
            const Divider(height: 24),
            Row(
              children: [
                Expanded(
                  child: ElevatedButton(
                    onPressed: () => provider.setFanOff(),
                    child: const Text('OFF\n(0°)'),
                    style: ElevatedButton.styleFrom(
                      backgroundColor: Colors.grey.shade700,
                      padding: const EdgeInsets.symmetric(vertical: 16),
                    ),
                  ),
                ),
                const SizedBox(width: 8),
                Expanded(
                  child: ElevatedButton(
                    onPressed: () => provider.setFanPartial(),
                    child: const Text('PARTIAL\n(90°)'),
                    style: ElevatedButton.styleFrom(
                      backgroundColor: Colors.orange,
                      padding: const EdgeInsets.symmetric(vertical: 16),
                    ),
                  ),
                ),
                const SizedBox(width: 8),
                Expanded(
                  child: ElevatedButton(
                    onPressed: () => provider.setFanOn(),
                    child: const Text('ON\n(180°)'),
                    style: ElevatedButton.styleFrom(
                      backgroundColor: Colors.blue,
                      padding: const EdgeInsets.symmetric(vertical: 16),
                    ),
                  ),
                ),
              ],
            ),
            const SizedBox(height: 12),
            Container(
              padding: const EdgeInsets.all(12),
              decoration: BoxDecoration(
                color: Colors.grey.shade900,
                borderRadius: BorderRadius.circular(8),
              ),
              child: Row(
                mainAxisAlignment: MainAxisAlignment.spaceBetween,
                children: [
                  const Text('Current State:'),
                  Text(
                    '${provider.state.fanStateText} (${provider.state.fanAngle}°)',
                    style: const TextStyle(fontWeight: FontWeight.bold),
                  ),
                ],
              ),
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildBuzzerControls(GasDetectorProvider provider) {
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Row(
              children: [
                const Icon(Icons.volume_up, size: 24),
                const SizedBox(width: 12),
                const Text(
                  'Buzzer Controls',
                  style: TextStyle(fontSize: 18, fontWeight: FontWeight.bold),
                ),
              ],
            ),
            const Divider(height: 24),
            Row(
              children: [
                Expanded(
                  child: ElevatedButton.icon(
                    onPressed: () => provider.silenceBuzzer(),
                    icon: const Icon(Icons.volume_off),
                    label: const Text('Silence'),
                    style: ElevatedButton.styleFrom(
                      backgroundColor: Colors.orange,
                      padding: const EdgeInsets.symmetric(vertical: 16),
                    ),
                  ),
                ),
                const SizedBox(width: 12),
                Expanded(
                  child: ElevatedButton.icon(
                    onPressed: () => provider.enableBuzzer(),
                    icon: const Icon(Icons.volume_up),
                    label: const Text('Enable'),
                    style: ElevatedButton.styleFrom(
                      backgroundColor: Colors.green,
                      padding: const EdgeInsets.symmetric(vertical: 16),
                    ),
                  ),
                ),
              ],
            ),
            const SizedBox(height: 12),
            ElevatedButton.icon(
              onPressed: () => provider.testBuzzer(),
              icon: const Icon(Icons.notifications_active),
              label: const Text('Test Buzzer (2 sec beep)'),
              style: ElevatedButton.styleFrom(
                backgroundColor: Colors.blue,
                padding: const EdgeInsets.symmetric(vertical: 16),
              ),
            ),
            const SizedBox(height: 12),
            Container(
              padding: const EdgeInsets.all(12),
              decoration: BoxDecoration(
                color: Colors.grey.shade900,
                borderRadius: BorderRadius.circular(8),
              ),
              child: Row(
                mainAxisAlignment: MainAxisAlignment.spaceBetween,
                children: [
                  const Text('Current State:'),
                  Text(
                    provider.state.buzzerSilenced ? 'SILENCED' : 'ENABLED',
                    style: TextStyle(
                      fontWeight: FontWeight.bold,
                      color: provider.state.buzzerSilenced ? Colors.orange : Colors.green,
                    ),
                  ),
                ],
              ),
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildTestControls(GasDetectorProvider provider) {
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Row(
              children: [
                const Icon(Icons.science, size: 24),
                const SizedBox(width: 12),
                const Text(
                  'Testing & Diagnostics',
                  style: TextStyle(fontSize: 18, fontWeight: FontWeight.bold),
                ),
              ],
            ),
            const Divider(height: 24),
            ElevatedButton.icon(
              onPressed: () => provider.testAlarm(),
              icon: const Icon(Icons.warning),
              label: const Text('Test Alarm (10 sec simulation)'),
              style: ElevatedButton.styleFrom(
                backgroundColor: Colors.red,
                padding: const EdgeInsets.symmetric(vertical: 16),
              ),
            ),
            const SizedBox(height: 12),
            ElevatedButton.icon(
              onPressed: () => provider.testWarning(),
              icon: const Icon(Icons.warning_amber),
              label: const Text('Test Warning (10 sec simulation)'),
              style: ElevatedButton.styleFrom(
                backgroundColor: Colors.orange,
                padding: const EdgeInsets.symmetric(vertical: 16),
              ),
            ),
            const SizedBox(height: 12),
            ElevatedButton.icon(
              onPressed: () => provider.testLed(),
              icon: const Icon(Icons.lightbulb),
              label: const Text('Test LEDs'),
              style: ElevatedButton.styleFrom(
                backgroundColor: Colors.purple,
                padding: const EdgeInsets.symmetric(vertical: 16),
              ),
            ),
            const SizedBox(height: 12),
            ElevatedButton.icon(
              onPressed: () => provider.calibrateSensor(),
              icon: const Icon(Icons.tune),
              label: const Text('Calibrate Sensor'),
              style: ElevatedButton.styleFrom(
                backgroundColor: Colors.teal,
                padding: const EdgeInsets.symmetric(vertical: 16),
              ),
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildSystemControls(GasDetectorProvider provider) {
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Row(
              children: [
                const Icon(Icons.settings, size: 24),
                const SizedBox(width: 12),
                const Text(
                  'System Management',
                  style: TextStyle(fontSize: 18, fontWeight: FontWeight.bold),
                ),
              ],
            ),
            const Divider(height: 24),
            ElevatedButton.icon(
              onPressed: () => provider.systemReset(),
              icon: const Icon(Icons.restart_alt),
              label: const Text('System Reset (return to safe state)'),
              style: ElevatedButton.styleFrom(
                backgroundColor: Colors.blue,
                padding: const EdgeInsets.symmetric(vertical: 16),
              ),
            ),
            const SizedBox(height: 12),
            ElevatedButton.icon(
              onPressed: () => provider.systemStatus(),
              icon: const Icon(Icons.info),
              label: const Text('Request Full Status Update'),
              style: ElevatedButton.styleFrom(
                backgroundColor: Colors.green,
                padding: const EdgeInsets.symmetric(vertical: 16),
              ),
            ),
            const SizedBox(height: 12),
            ElevatedButton.icon(
              onPressed: () => provider.healthPing(),
              icon: const Icon(Icons.favorite),
              label: const Text('Health Ping'),
              style: ElevatedButton.styleFrom(
                backgroundColor: Colors.cyan,
                padding: const EdgeInsets.symmetric(vertical: 16),
              ),
            ),
            const SizedBox(height: 12),
            ElevatedButton.icon(
              onPressed: () => _showRebootDialog(provider),
              icon: const Icon(Icons.power_settings_new),
              label: const Text('Reboot ESP32 Device'),
              style: ElevatedButton.styleFrom(
                backgroundColor: Colors.red.shade700,
                padding: const EdgeInsets.symmetric(vertical: 16),
              ),
            ),
          ],
        ),
      ),
    );
  }

  void _showRebootDialog(GasDetectorProvider provider) {
    // This would be shown in a BuildContext
    // For now, just call the reboot directly
    provider.systemReboot();
  }
}
