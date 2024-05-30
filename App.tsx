import React, { useState, useEffect } from 'react';
import { SafeAreaView, Text, FlatList, TouchableOpacity, Alert, ActivityIndicator } from 'react-native';
import { Device } from 'react-native-ble-plx';
import { startBluetooth, scanDevices, connectToDevice, resetFallCount, getNetworkConfig, getBluetoothStatus, getWifiStatus } from './BluetoothService';
import styles from './styles/styles';

const App: React.FC = () => {
  const [devices, setDevices] = useState<Map<string, Device>>(new Map());
  const [connectedDevice, setConnectedDevice] = useState<Device | null>(null);
  const [fallCount, setFallCount] = useState<number>(0);
  const [networkConfig, setNetworkConfig] = useState<string>("");
  const [wifiStatus, setWifiStatus] = useState<string>("Inconnu");
  const [bluetoothStatus, setBluetoothStatus] = useState<string>("Déconnecté");
  const [isScanning, setIsScanning] = useState<boolean>(false);

  useEffect(() => {
    startBluetooth();

    return () => {
      if (connectedDevice) {
        connectedDevice.cancelConnection();
      }
    };
  }, [connectedDevice]);

  const startScan = () => {
    scanDevices(setDevices, setIsScanning);
  };

  const connectToSelectedDevice = async (device) => {
    try {
      await connectToDevice(device, setConnectedDevice, setBluetoothStatus, setFallCount, setWifiStatus);
  
      const config = await getNetworkConfig(device);
      setNetworkConfig(config);
      const bluetoothStatus = await getBluetoothStatus(device);
      setBluetoothStatus(bluetoothStatus);
  
      const wifiStatus = await getWifiStatus(device);
      setWifiStatus(wifiStatus);
    } catch (err) {
      const error = err as Error;
      Alert.alert("Erreur de connexion", "Échec de la connexion à l'appareil : " + error.message);
    }
  };

  const handleResetFallCount = async () => {
    if (connectedDevice) {
      await resetFallCount(connectedDevice);
      setFallCount(0);
    }
  };

  const disconnectDevice = async () => {
    if (connectedDevice) {
      await connectedDevice.cancelConnection();
      setConnectedDevice(null);
      setBluetoothStatus("Déconnecté");
      setFallCount(0);
      setWifiStatus("Inconnu");
    }
  };

  return (
    <SafeAreaView style={styles.container}>
      <TouchableOpacity style={styles.button} onPress={connectedDevice ? disconnectDevice : startScan} disabled={isScanning}>
        {isScanning ? (
          <ActivityIndicator size="small" color="#FFFFFF" />
        ) : (
          <Text style={styles.buttonText}>{connectedDevice ? 'Déconnecter' : 'Rechercher des appareils'}</Text>
        )}
      </TouchableOpacity>
      <FlatList
        data={Array.from(devices.values())}
        renderItem={({ item }) => (
          <TouchableOpacity
            style={[styles.deviceButton, { backgroundColor: connectedDevice?.id === item.id ? 'green' : 'grey' }]}
            onPress={() => connectToSelectedDevice(item)}
            disabled={!!connectedDevice}
          >
            <Text style={styles.buttonText}>{item.name}</Text>
          </TouchableOpacity>
        )}
        keyExtractor={(item) => item.id}
      />
      <Text style={styles.text}>Compteur de chutes: {fallCount}</Text>
      <TouchableOpacity style={styles.button} onPress={handleResetFallCount} disabled={!connectedDevice}>
        <Text style={styles.buttonText}>Réinitialiser le compteur de chutes</Text>
      </TouchableOpacity>
      <Text style={styles.text}>Configuration réseau: {networkConfig}</Text>
      <Text style={styles.text}>Statut WiFi: {wifiStatus}</Text>
      <Text style={styles.text}>Statut Bluetooth: {bluetoothStatus}</Text>
    </SafeAreaView>
  );
};

export default App;
