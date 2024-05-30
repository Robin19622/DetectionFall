import { PermissionsAndroid, Platform, Alert } from 'react-native';
import { BleManager, Device } from 'react-native-ble-plx';
import base64 from 'base-64';

const manager = new BleManager();

const SERVICE_UUID = '4fafc201-1fb5-459e-8fcc-c5c9c331914b';
const FALL_COUNT_CHAR_UUID = 'beb5483e-36e1-4688-b7f5-ea07361b26a8';
const RESET_FALL_COUNT_UUID = '6d68efe5-04b6-4a4d-aeae-3e97b9b96c3b';
const NETWORK_CONFIG_UUID = '12345678-1234-1234-1234-1234567890ab';
const BLUETOOTH_STATUS_UUID = '87654321-4321-4321-4321-abcdefabcdef';
const WIFI_STATUS_UUID = 'abcdef12-1234-5678-9abc-def123456789';

// Function to start Bluetooth
export async function startBluetooth() {
  if (Platform.OS === 'android' && Platform.Version >= 23) {
    const granted = await PermissionsAndroid.requestMultiple([
      PermissionsAndroid.PERMISSIONS.BLUETOOTH_SCAN,
      PermissionsAndroid.PERMISSIONS.BLUETOOTH_CONNECT,
      PermissionsAndroid.PERMISSIONS.ACCESS_FINE_LOCATION,
      PermissionsAndroid.PERMISSIONS.ACCESS_COARSE_LOCATION,
    ]);

    if (
      granted[PermissionsAndroid.PERMISSIONS.BLUETOOTH_SCAN] !== PermissionsAndroid.RESULTS.GRANTED ||
      granted[PermissionsAndroid.PERMISSIONS.BLUETOOTH_CONNECT] !== PermissionsAndroid.RESULTS.GRANTED ||
      granted[PermissionsAndroid.PERMISSIONS.ACCESS_FINE_LOCATION] !== PermissionsAndroid.RESULTS.GRANTED ||
      granted[PermissionsAndroid.PERMISSIONS.ACCESS_COARSE_LOCATION] !== PermissionsAndroid.RESULTS.GRANTED
    ) {
      Alert.alert("Permissions non accordées", "Veuillez accorder toutes les permissions pour utiliser les fonctionnalités Bluetooth.");
      return;
    }
  }
  console.log('Bluetooth démarré');
}

// Function to scan for devices
export async function scanDevices(setDevices, setIsScanning) {
  console.log('Recherche des appareils...');
  setIsScanning(true);

  manager.startDeviceScan(null, null, (error, device) => {
    if (error) {
      console.error('Échec du scan :', error);
      Alert.alert('Erreur de scan', 'Échec du démarrage de la recherche des appareils.');
      setIsScanning(false);
      return;
    }

    if (device && device.name && device.name.startsWith('ESP32')) {
      console.log('Périphérique découvert :', device);
      setDevices((prevDevices) => new Map(prevDevices.set(device.id, device)));
    }
  });

  setTimeout(() => {
    manager.stopDeviceScan();
    setIsScanning(false);
    console.log('Scan arrêté.');
  }, 10000);
}

export async function connectToDevice(device, setConnectedDevice, setBluetoothStatus, setFallCount, setWifiStatus) {
  try {
    await device.connect();
    console.log('Connecté à ', device.name);
    await device.discoverAllServicesAndCharacteristics();
    setConnectedDevice(device);
    setBluetoothStatus("Bluetooth Connected");

    const services = await device.services();
    for (const service of services) {
      const characteristics = await service.characteristics();
      for (const characteristic of characteristics) {
        if (characteristic.uuid === FALL_COUNT_CHAR_UUID) {
          await characteristic.monitor((error, char) => {
            if (error) {
              if (device.isConnected) {
                console.error('Échec de la configuration des notifications :', error);
              }
              return;
            }
            if (char?.value) {
              const decodedValue = parseInt(base64.decode(char.value), 10);
              if (!isNaN(decodedValue)) {
                console.log('Notification reçue pour le compteur de chutes:', decodedValue);
                setFallCount(decodedValue);
              } else {
                console.log('Valeur de compteur de chutes invalide reçue:', char.value);
              }
            }
          });
        }
        if (characteristic.uuid === BLUETOOTH_STATUS_UUID) {
          await characteristic.monitor((error, char) => {
            if (error) {
              if (device.isConnected) {
                console.error('Échec de la configuration des notifications :', error);
              }
              return;
            }
            if (char?.value) {
              const decodedValue = base64.decode(char.value);
              console.log('Notification reçue pour le statut Bluetooth:', decodedValue);
              setBluetoothStatus(decodedValue);
            }
          });
        }
        if (characteristic.uuid === WIFI_STATUS_UUID) {
          await characteristic.monitor((error, char) => {
            if (error) {
              if (device.isConnected) {
                console.error('Échec de la configuration des notifications :', error);
              }
              return;
            }
            if (char?.value) {
              const decodedValue = base64.decode(char.value);
              console.log('Notification reçue pour le statut WiFi:', decodedValue);
              setWifiStatus(decodedValue);
            }
          });
        }
        if (characteristic.uuid === NETWORK_CONFIG_UUID) {
          await characteristic.monitor((error, char) => {
            if (error) {
              if (device.isConnected) {
                console.error('Échec de la configuration des notifications :', error);
              }
              return;
            }
            if (char?.value) {
              const decodedValue = base64.decode(char.value);
              console.log('Notification reçue pour la configuration réseau:', decodedValue);
              setNetworkConfig(decodedValue);
            }
          });
        }
      }
    }
  } catch (err) {
    console.error('Échec de la connexion :', err);
    throw new Error('Échec de la connexion à l\'appareil');
  }
}

export async function readFallCount(device) {
  try {
    const characteristic = await device.readCharacteristicForService(SERVICE_UUID, FALL_COUNT_CHAR_UUID);
    return parseInt(base64.decode(characteristic.value || ''), 10);
  } catch (err) {
    console.error('Échec de la lecture du compteur de chutes :', err);
    throw new Error('Échec de la lecture du compteur de chutes');
  }
}

export async function resetFallCount(device) {
  try {
    const resetCommand = base64.encode('reset');
    await device.writeCharacteristicWithResponseForService(SERVICE_UUID, RESET_FALL_COUNT_UUID, resetCommand);
  } catch (err) {
    console.error('Échec de la réinitialisation du compteur de chutes :', err);
    throw new Error('Échec de la réinitialisation du compteur de chutes');
  }
}

export async function getNetworkConfig(device) {
  try {
    const characteristic = await device.readCharacteristicForService(SERVICE_UUID, NETWORK_CONFIG_UUID);
    return base64.decode(characteristic.value || '');
  } catch (err) {
    console.error('Échec de l\'obtention de la configuration réseau :', err);
    throw new Error('Échec de l\'obtention de la configuration réseau');
  }
}

export async function getBluetoothStatus(device) {
  try {
    const characteristic = await device.readCharacteristicForService(SERVICE_UUID, BLUETOOTH_STATUS_UUID);
    return base64.decode(characteristic.value || '');
  } catch (err) {
    console.error('Échec de l\'obtention du statut Bluetooth :', err);
    throw new Error('Échec de l\'obtention du statut Bluetooth');
  }
}

export async function getWifiStatus(device) {
  try {
    const characteristic = await device.readCharacteristicForService(SERVICE_UUID, WIFI_STATUS_UUID);
    return base64.decode(characteristic.value || '');
  } catch (err) {
    console.error('Échec de l\'obtention du statut WiFi :', err);
    throw new Error('Échec de l\'obtention du statut WiFi');
  }
}
