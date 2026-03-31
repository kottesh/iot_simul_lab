import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../providers/gas_detector_provider.dart';
import '../models/gas_detector_model.dart';
import '../widgets/gas_level_gauge.dart';
import '../widgets/status_card.dart';
import '../widgets/device_status_panel.dart';
import 'package:flutter_animate/flutter_animate.dart';

class DashboardScreen extends StatelessWidget {
  const DashboardScreen({Key? key}) : super(key: key);

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('Gas Detector Dashboard'),
        actions: [
          Consumer<GasDetectorProvider>(
            builder: (context, provider, _) {
              if (provider.isConnecting) {
                return const Padding(
                  padding: EdgeInsets.all(16),
                  child: SizedBox(
                    width: 20,
                    height: 20,
                    child: CircularProgressIndicator(strokeWidth: 2),
                  ),
                );
              }
              return IconButton(
                icon: Icon(
                  provider.isConnected ? Icons.cloud_done : Icons.cloud_off,
                  color: provider.isConnected ? Colors.green : Colors.red,
                ),
                tooltip: provider.isConnected ? 'Connected — tap to disconnect' : 'Disconnected — tap to connect',
                onPressed: () {
                  if (provider.isConnected) {
                    provider.disconnect();
                  } else {
                    provider.connect();
                  }
                },
              );
            },
          ),
          IconButton(
            icon: const Icon(Icons.refresh),
            tooltip: 'Request status update',
            onPressed: () => context.read<GasDetectorProvider>().systemStatus(),
          ),
        ],
      ),
      body: Consumer<GasDetectorProvider>(
        builder: (context, provider, _) {
          final state = provider.state;

          return RefreshIndicator(
            onRefresh: () async {
              if (provider.isConnected) {
                provider.systemStatus();
              } else {
                await provider.connect();
              }
            },
            child: SingleChildScrollView(
              physics: const AlwaysScrollableScrollPhysics(),
              padding: const EdgeInsets.all(16),
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.stretch,
                children: [
                  // Connection error banner
                  if (!provider.isConnected && provider.connectionError.isNotEmpty)
                    _buildConnectionBanner(context, provider),

                  // Alert Banner
                  if (state.activeAlert != null)
                    _buildAlertBanner(context, state.activeAlert!, provider),

                  // Gas reading note
                  _buildReadingNote(state),

                  const SizedBox(height: 8),

                  // Gas Level Gauge — maxPpm auto-scales to current reading
                  Center(
                    child: GasLevelGauge(
                      ppm: state.ppm,
                      warningThreshold: state.thresholdWarning,
                      dangerThreshold: state.thresholdDanger,
                    ),
                  ),

                  const SizedBox(height: 24),

                  // Status Card
                  StatusCard(
                    status: state.status,
                    ppm: state.ppm,
                  ).animate().fadeIn(duration: 500.ms, delay: 100.ms),

                  const SizedBox(height: 16),

                  // Device Status Panel
                  DeviceStatusPanel(
                    valveOpen: state.valveOpen,
                    fanState: state.fanStateText,
                    fanAngle: state.fanAngle,
                    buzzerSilenced: state.buzzerSilenced,
                    autoRecovery: state.autoRecovery,
                    isOnline: state.isOnline,
                    deviceIp: state.deviceIp,
                    uptime: state.uptimeFormatted,
                  ).animate().fadeIn(duration: 500.ms, delay: 200.ms),

                  const SizedBox(height: 16),

                  // Quick Actions
                  _buildQuickActions(context, provider),

                  const SizedBox(height: 16),

                  // Threshold Info
                  _buildThresholdInfo(state),

                  const SizedBox(height: 16),
                ],
              ),
            ),
          );
        },
      ),
    );
  }

  Widget _buildConnectionBanner(BuildContext context, GasDetectorProvider provider) {
    return Container(
      margin: const EdgeInsets.only(bottom: 12),
      padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 12),
      decoration: BoxDecoration(
        color: Colors.red.shade900.withValues(alpha: 0.8),
        borderRadius: BorderRadius.circular(10),
        border: Border.all(color: Colors.red.shade700),
      ),
      child: Row(
        children: [
          const Icon(Icons.wifi_off, color: Colors.white),
          const SizedBox(width: 12),
          Expanded(
            child: Text(
              provider.connectionError,
              style: const TextStyle(color: Colors.white, fontSize: 13),
            ),
          ),
          TextButton(
            onPressed: () => provider.connect(),
            child: const Text('RETRY', style: TextStyle(color: Colors.white)),
          ),
        ],
      ),
    );
  }

  Widget _buildReadingNote(GasDetectorModel state) {
    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 14, vertical: 8),
      decoration: BoxDecoration(
        color: Colors.grey.shade900,
        borderRadius: BorderRadius.circular(8),
      ),
      child: Row(
        mainAxisAlignment: MainAxisAlignment.center,
        children: [
          Icon(Icons.sensors, size: 16, color: Colors.grey.shade400),
          const SizedBox(width: 8),
          Text(
            'Sensor: Digital mode — Gas ${state.ppm > 0 ? "DETECTED (1000 PPM)" : "not detected (0 PPM)"}',
            style: TextStyle(fontSize: 12, color: Colors.grey.shade400),
          ),
        ],
      ),
    );
  }

  Widget _buildAlertBanner(
      BuildContext context, AlertData alert, GasDetectorProvider provider) {
    return Container(
      margin: const EdgeInsets.only(bottom: 12),
      padding: const EdgeInsets.all(16),
      decoration: BoxDecoration(
        color: Colors.red.shade900,
        borderRadius: BorderRadius.circular(12),
        border: Border.all(color: Colors.red, width: 2),
      ),
      child: Row(
        children: [
          const Icon(Icons.warning_amber, color: Colors.white, size: 32),
          const SizedBox(width: 16),
          Expanded(
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Text(
                  '🚨 ${alert.level}',
                  style: const TextStyle(
                    color: Colors.white,
                    fontSize: 18,
                    fontWeight: FontWeight.bold,
                  ),
                ),
                const SizedBox(height: 4),
                Text(alert.message,
                    style: const TextStyle(color: Colors.white, fontSize: 13)),
                Text('${alert.ppm} PPM',
                    style: TextStyle(
                        color: Colors.red.shade200, fontSize: 12)),
              ],
            ),
          ),
          IconButton(
            icon: const Icon(Icons.close, color: Colors.white),
            onPressed: () => provider.clearAlert(),
          ),
        ],
      ),
    )
        .animate(onPlay: (controller) => controller.repeat())
        .shimmer(duration: 2000.ms, color: Colors.red.withValues(alpha: 0.3));
  }

  Widget _buildQuickActions(
      BuildContext context, GasDetectorProvider provider) {
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            const Text(
              'Quick Actions',
              style: TextStyle(fontSize: 18, fontWeight: FontWeight.bold),
            ),
            const SizedBox(height: 16),
            Row(
              children: [
                Expanded(
                  child: ElevatedButton.icon(
                    onPressed: provider.isConnected
                        ? (provider.state.valveOpen
                            ? () => provider.closeValve()
                            : () => provider.openValve())
                        : null,
                    icon: Icon(provider.state.valveOpen
                        ? Icons.lock
                        : Icons.lock_open),
                    label: Text(
                        provider.state.valveOpen ? 'Close Valve' : 'Open Valve'),
                    style: ElevatedButton.styleFrom(
                      backgroundColor:
                          provider.state.valveOpen ? Colors.red : Colors.green,
                      padding: const EdgeInsets.symmetric(vertical: 14),
                    ),
                  ),
                ),
              ],
            ),
            const SizedBox(height: 12),
            Row(
              children: [
                Expanded(
                  child: ElevatedButton.icon(
                    onPressed: provider.isConnected
                        ? () => provider.silenceBuzzer()
                        : null,
                    icon: const Icon(Icons.volume_off),
                    label: const Text('Silence Buzzer'),
                    style: ElevatedButton.styleFrom(
                      backgroundColor: Colors.orange,
                      padding: const EdgeInsets.symmetric(vertical: 14),
                    ),
                  ),
                ),
                const SizedBox(width: 8),
                Expanded(
                  child: ElevatedButton.icon(
                    onPressed:
                        provider.isConnected ? () => provider.systemReset() : null,
                    icon: const Icon(Icons.restart_alt),
                    label: const Text('System Reset'),
                    style: ElevatedButton.styleFrom(
                      backgroundColor: Colors.blue,
                      padding: const EdgeInsets.symmetric(vertical: 14),
                    ),
                  ),
                ),
              ],
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildThresholdInfo(GasDetectorModel state) {
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            const Text(
              'Threshold Levels',
              style: TextStyle(fontSize: 18, fontWeight: FontWeight.bold),
            ),
            const SizedBox(height: 12),
            Row(
              mainAxisAlignment: MainAxisAlignment.spaceAround,
              children: [
                _buildThresholdItem('Safe', '< ${state.thresholdWarning}', Colors.green),
                _buildThresholdItem('Warning', '≥ ${state.thresholdWarning}', Colors.orange),
                _buildThresholdItem('Danger', '≥ ${state.thresholdDanger}', Colors.red),
              ],
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildThresholdItem(String label, String value, Color color) {
    return Column(
      children: [
        Text(
          label,
          style: TextStyle(fontSize: 12, color: Colors.grey.shade400),
        ),
        const SizedBox(height: 4),
        Text(
          '$value PPM',
          style: TextStyle(
              fontSize: 14, fontWeight: FontWeight.bold, color: color),
        ),
      ],
    );
  }
}
