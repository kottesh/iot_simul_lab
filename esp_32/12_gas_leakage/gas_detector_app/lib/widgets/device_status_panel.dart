import 'package:flutter/material.dart';

class DeviceStatusPanel extends StatelessWidget {
  final bool valveOpen;
  final String fanState;
  final int fanAngle;
  final bool buzzerSilenced;
  final bool autoRecovery;
  final bool isOnline;
  final String deviceIp;
  final String uptime;

  const DeviceStatusPanel({
    Key? key,
    required this.valveOpen,
    required this.fanState,
    required this.fanAngle,
    required this.buzzerSilenced,
    required this.autoRecovery,
    required this.isOnline,
    required this.deviceIp,
    required this.uptime,
  }) : super(key: key);

  @override
  Widget build(BuildContext context) {
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Row(
              mainAxisAlignment: MainAxisAlignment.spaceBetween,
              children: [
                const Text(
                  'Device Status',
                  style: TextStyle(
                    fontSize: 18,
                    fontWeight: FontWeight.bold,
                  ),
                ),
                Row(
                  children: [
                    Container(
                      width: 8,
                      height: 8,
                      decoration: BoxDecoration(
                        color: isOnline ? Colors.green : Colors.red,
                        shape: BoxShape.circle,
                      ),
                    ),
                    const SizedBox(width: 8),
                    Text(
                      isOnline ? 'Online' : 'Offline',
                      style: TextStyle(
                        color: isOnline ? Colors.green : Colors.red,
                        fontWeight: FontWeight.bold,
                      ),
                    ),
                  ],
                ),
              ],
            ),
            const Divider(height: 24),
            _buildStatusRow(
              icon: Icons.power,
              label: 'Gas Valve',
              value: valveOpen ? 'OPEN' : 'CLOSED',
              valueColor: valveOpen ? Colors.green : Colors.red,
            ),
            const SizedBox(height: 12),
            _buildStatusRow(
              icon: Icons.air,
              label: 'Ventilation Fan',
              value: '$fanState ($fanAngle°)',
              valueColor: fanAngle > 0 ? Colors.blue : Colors.grey,
            ),
            const SizedBox(height: 12),
            _buildStatusRow(
              icon: buzzerSilenced ? Icons.volume_off : Icons.volume_up,
              label: 'Buzzer',
              value: buzzerSilenced ? 'SILENCED' : 'ENABLED',
              valueColor: buzzerSilenced ? Colors.orange : Colors.green,
            ),
            const SizedBox(height: 12),
            _buildStatusRow(
              icon: Icons.autorenew,
              label: 'Auto-Recovery',
              value: autoRecovery ? 'ON' : 'OFF',
              valueColor: autoRecovery ? Colors.green : Colors.grey,
            ),
            const Divider(height: 24),
            _buildInfoRow('IP Address', deviceIp.isNotEmpty ? deviceIp : 'N/A'),
            const SizedBox(height: 8),
            _buildInfoRow('Uptime', uptime),
          ],
        ),
      ),
    );
  }

  Widget _buildStatusRow({
    required IconData icon,
    required String label,
    required String value,
    required Color valueColor,
  }) {
    return Row(
      children: [
        Icon(icon, size: 24, color: Colors.grey.shade400),
        const SizedBox(width: 12),
        Expanded(
          child: Text(
            label,
            style: const TextStyle(fontSize: 14),
          ),
        ),
        Text(
          value,
          style: TextStyle(
            fontSize: 14,
            fontWeight: FontWeight.bold,
            color: valueColor,
          ),
        ),
      ],
    );
  }

  Widget _buildInfoRow(String label, String value) {
    return Row(
      mainAxisAlignment: MainAxisAlignment.spaceBetween,
      children: [
        Text(
          label,
          style: TextStyle(
            fontSize: 12,
            color: Colors.grey.shade400,
          ),
        ),
        Text(
          value,
          style: const TextStyle(
            fontSize: 12,
            fontWeight: FontWeight.w500,
          ),
        ),
      ],
    );
  }
}
