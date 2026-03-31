import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../providers/gas_detector_provider.dart';
import '../services/storage_service.dart';

class SettingsScreen extends StatefulWidget {
  const SettingsScreen({Key? key}) : super(key: key);

  @override
  State<SettingsScreen> createState() => _SettingsScreenState();
}

class _SettingsScreenState extends State<SettingsScreen> {
  final _storageService = StorageService();
  final _formKey = GlobalKey<FormState>();

  late TextEditingController _brokerController;
  late TextEditingController _portController;
  late TextEditingController _clientIdController;
  late TextEditingController _warningThresholdController;
  late TextEditingController _dangerThresholdController;

  bool _autoRecovery = false;
  bool _isLoading = true;

  @override
  void initState() {
    super.initState();
    _brokerController = TextEditingController();
    _portController = TextEditingController();
    _clientIdController = TextEditingController();
    _warningThresholdController = TextEditingController();
    _dangerThresholdController = TextEditingController();
    _loadSettings();
  }

  Future<void> _loadSettings() async {
    final broker = await _storageService.getMqttBroker();
    final port = await _storageService.getMqttPort();
    final clientId = await _storageService.getMqttClientId();
    final warning = await _storageService.getThresholdWarning();
    final danger = await _storageService.getThresholdDanger();
    final autoRec = await _storageService.getAutoRecovery();

    setState(() {
      _brokerController.text = broker;
      _portController.text = port.toString();
      _clientIdController.text = clientId;
      _warningThresholdController.text = warning.toString();
      _dangerThresholdController.text = danger.toString();
      _autoRecovery = autoRec;
      _isLoading = false;
    });
  }

  Future<void> _saveSettings() async {
    if (!_formKey.currentState!.validate()) return;

    await _storageService.setMqttBroker(_brokerController.text);
    await _storageService.setMqttPort(int.parse(_portController.text));
    await _storageService.setMqttClientId(_clientIdController.text);
    await _storageService.setThresholdWarning(int.parse(_warningThresholdController.text));
    await _storageService.setThresholdDanger(int.parse(_dangerThresholdController.text));
    await _storageService.setAutoRecovery(_autoRecovery);

    if (mounted) {
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(content: Text('Settings saved successfully!')),
      );
    }
  }

  Future<void> _applyThresholds() async {
    if (!_formKey.currentState!.validate()) return;

    final warning = int.parse(_warningThresholdController.text);
    final danger = int.parse(_dangerThresholdController.text);

    context.read<GasDetectorProvider>().setThresholds(warning, danger);

    if (mounted) {
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(content: Text('Thresholds sent to device!')),
      );
    }
  }

  Future<void> _reconnect() async {
    final provider = context.read<GasDetectorProvider>();
    provider.disconnect();
    await Future.delayed(const Duration(seconds: 1));
    await _saveSettings();
    final success = await provider.connect();

    if (mounted) {
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(
          content: Text(success ? 'Connected successfully!' : 'Connection failed'),
          backgroundColor: success ? Colors.green : Colors.red,
        ),
      );
    }
  }

  @override
  void dispose() {
    _brokerController.dispose();
    _portController.dispose();
    _clientIdController.dispose();
    _warningThresholdController.dispose();
    _dangerThresholdController.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    if (_isLoading) {
      return const Scaffold(
        body: Center(child: CircularProgressIndicator()),
      );
    }

    return Scaffold(
      appBar: AppBar(
        title: const Text('Settings'),
        actions: [
          IconButton(
            icon: const Icon(Icons.save),
            onPressed: _saveSettings,
          ),
        ],
      ),
      body: SingleChildScrollView(
        padding: const EdgeInsets.all(16),
        child: Form(
          key: _formKey,
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.stretch,
            children: [
              _buildMqttSettings(),
              const SizedBox(height: 16),
              _buildThresholdSettings(),
              const SizedBox(height: 16),
              _buildAutoRecoverySettings(),
              const SizedBox(height: 16),
              _buildDeviceInfo(),
              const SizedBox(height: 24),
              _buildActionButtons(),
            ],
          ),
        ),
      ),
    );
  }

  Widget _buildMqttSettings() {
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Row(
              children: [
                const Icon(Icons.cloud, size: 24),
                const SizedBox(width: 12),
                const Text(
                  'MQTT Configuration',
                  style: TextStyle(fontSize: 18, fontWeight: FontWeight.bold),
                ),
              ],
            ),
            const Divider(height: 24),
            TextFormField(
              controller: _brokerController,
              decoration: const InputDecoration(
                labelText: 'MQTT Broker Address',
                hintText: '100.64.0.1',
                border: OutlineInputBorder(),
                prefixIcon: Icon(Icons.dns),
              ),
              validator: (value) {
                if (value == null || value.isEmpty) {
                  return 'Please enter broker address';
                }
                return null;
              },
            ),
            const SizedBox(height: 12),
            TextFormField(
              controller: _portController,
              decoration: const InputDecoration(
                labelText: 'MQTT Port',
                hintText: '1883',
                border: OutlineInputBorder(),
                prefixIcon: Icon(Icons.settings_ethernet),
              ),
              keyboardType: TextInputType.number,
              validator: (value) {
                if (value == null || value.isEmpty) {
                  return 'Please enter port';
                }
                final port = int.tryParse(value);
                if (port == null || port < 1 || port > 65535) {
                  return 'Invalid port number';
                }
                return null;
              },
            ),
            const SizedBox(height: 12),
            TextFormField(
              controller: _clientIdController,
              decoration: const InputDecoration(
                labelText: 'Client ID',
                hintText: 'FlutterGasDetectorApp',
                border: OutlineInputBorder(),
                prefixIcon: Icon(Icons.fingerprint),
              ),
              validator: (value) {
                if (value == null || value.isEmpty) {
                  return 'Please enter client ID';
                }
                return null;
              },
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildThresholdSettings() {
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Row(
              children: [
                const Icon(Icons.speed, size: 24),
                const SizedBox(width: 12),
                const Text(
                  'Gas Level Thresholds',
                  style: TextStyle(fontSize: 18, fontWeight: FontWeight.bold),
                ),
              ],
            ),
            const Divider(height: 24),
            TextFormField(
              controller: _warningThresholdController,
              decoration: const InputDecoration(
                labelText: 'Warning Threshold (PPM)',
                hintText: '300',
                border: OutlineInputBorder(),
                prefixIcon: Icon(Icons.warning_amber),
              ),
              keyboardType: TextInputType.number,
              validator: (value) {
                if (value == null || value.isEmpty) {
                  return 'Please enter warning threshold';
                }
                final threshold = int.tryParse(value);
                if (threshold == null || threshold < 0) {
                  return 'Invalid threshold value';
                }
                return null;
              },
            ),
            const SizedBox(height: 12),
            TextFormField(
              controller: _dangerThresholdController,
              decoration: const InputDecoration(
                labelText: 'Danger Threshold (PPM)',
                hintText: '600',
                border: OutlineInputBorder(),
                prefixIcon: Icon(Icons.dangerous),
              ),
              keyboardType: TextInputType.number,
              validator: (value) {
                if (value == null || value.isEmpty) {
                  return 'Please enter danger threshold';
                }
                final threshold = int.tryParse(value);
                if (threshold == null || threshold < 0) {
                  return 'Invalid threshold value';
                }
                final warning = int.tryParse(_warningThresholdController.text);
                if (warning != null && threshold <= warning) {
                  return 'Danger threshold must be higher than warning';
                }
                return null;
              },
            ),
            const SizedBox(height: 12),
            ElevatedButton.icon(
              onPressed: _applyThresholds,
              icon: const Icon(Icons.send),
              label: const Text('Apply Thresholds to Device'),
              style: ElevatedButton.styleFrom(
                backgroundColor: Colors.blue,
                padding: const EdgeInsets.symmetric(vertical: 12),
              ),
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildAutoRecoverySettings() {
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Row(
              children: [
                const Icon(Icons.autorenew, size: 24),
                const SizedBox(width: 12),
                const Text(
                  'Auto-Recovery',
                  style: TextStyle(fontSize: 18, fontWeight: FontWeight.bold),
                ),
              ],
            ),
            const Divider(height: 24),
            SwitchListTile(
              title: const Text('Enable Auto-Recovery'),
              subtitle: const Text(
                'Automatically open valve when gas levels return to safe',
              ),
              value: _autoRecovery,
              onChanged: (value) {
                setState(() {
                  _autoRecovery = value;
                });
                context.read<GasDetectorProvider>().setAutoRecovery(value);
              },
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildDeviceInfo() {
    return Consumer<GasDetectorProvider>(
      builder: (context, provider, _) {
        return Card(
          child: Padding(
            padding: const EdgeInsets.all(16),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Row(
                  children: [
                    const Icon(Icons.info, size: 24),
                    const SizedBox(width: 12),
                    const Text(
                      'Device Information',
                      style: TextStyle(fontSize: 18, fontWeight: FontWeight.bold),
                    ),
                  ],
                ),
                const Divider(height: 24),
                _buildInfoRow('Device ID', 'ESP32-GasDetector'),
                const SizedBox(height: 8),
                _buildInfoRow('IP Address', provider.state.deviceIp.isNotEmpty 
                    ? provider.state.deviceIp 
                    : 'Not connected'),
                const SizedBox(height: 8),
                _buildInfoRow('Connection Status', 
                    provider.isConnected ? 'Connected' : 'Disconnected'),
                const SizedBox(height: 8),
                _buildInfoRow('Uptime', provider.state.uptimeFormatted),
              ],
            ),
          ),
        );
      },
    );
  }

  Widget _buildInfoRow(String label, String value) {
    return Row(
      mainAxisAlignment: MainAxisAlignment.spaceBetween,
      children: [
        Text(
          label,
          style: TextStyle(color: Colors.grey.shade400),
        ),
        Text(
          value,
          style: const TextStyle(fontWeight: FontWeight.w500),
        ),
      ],
    );
  }

  Widget _buildActionButtons() {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.stretch,
      children: [
        ElevatedButton.icon(
          onPressed: _saveSettings,
          icon: const Icon(Icons.save),
          label: const Text('Save Settings'),
          style: ElevatedButton.styleFrom(
            backgroundColor: Colors.green,
            padding: const EdgeInsets.symmetric(vertical: 16),
          ),
        ),
        const SizedBox(height: 12),
        ElevatedButton.icon(
          onPressed: _reconnect,
          icon: const Icon(Icons.refresh),
          label: const Text('Save & Reconnect'),
          style: ElevatedButton.styleFrom(
            backgroundColor: Colors.blue,
            padding: const EdgeInsets.symmetric(vertical: 16),
          ),
        ),
      ],
    );
  }
}
