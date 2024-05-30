#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <WiFi.h>

#define SERVICE_UUID                "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define FALL_COUNT_CHAR_UUID        "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define RESET_FALL_COUNT_UUID       "6d68efe5-04b6-4a4d-aeae-3e97b9b96c3b"
#define NETWORK_CONFIG_UUID         "12345678-1234-1234-1234-1234567890ab"
#define BLUETOOTH_STATUS_UUID       "87654321-4321-4321-4321-abcdefabcdef"
#define WIFI_STATUS_UUID            "abcdef12-1234-5678-9abc-def123456789"

BLECharacteristic fallCountCharacteristic(FALL_COUNT_CHAR_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
BLECharacteristic resetFallCountCharacteristic(RESET_FALL_COUNT_UUID, BLECharacteristic::PROPERTY_WRITE);
BLECharacteristic networkConfigCharacteristic(NETWORK_CONFIG_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
BLECharacteristic bluetoothStatusCharacteristic(BLUETOOTH_STATUS_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
BLECharacteristic wifiStatusCharacteristic(WIFI_STATUS_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);

const char* ssid = "rfid";
const char* password = "testtest";

int fallCount = 0;
bool isConnected = false;
unsigned long lastFallTime = 0;
const unsigned long fallInterval = 10000;
class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    isConnected = true;
    Serial.println("Bluetooth connecté");
    pServer->getAdvertising()->stop();
    
    bluetoothStatusCharacteristic.setValue("Bluetooth connecte");
    bluetoothStatusCharacteristic.notify();
    
    if (WiFi.status() == WL_CONNECTED) {
      wifiStatusCharacteristic.setValue("WiFi connecte");
      networkConfigCharacteristic.setValue(ssid);
    } else {
      wifiStatusCharacteristic.setValue("WiFi déconnecte");
    }
    wifiStatusCharacteristic.notify();
    networkConfigCharacteristic.notify();
  }

  void onDisconnect(BLEServer* pServer) {
    isConnected = false;
    Serial.println("Bluetooth déconnecte, relance de la publicite");
    pServer->getAdvertising()->start();
    
    bluetoothStatusCharacteristic.setValue("Bluetooth déconnecte");
    bluetoothStatusCharacteristic.notify();
  }
};

void WiFiEvent(WiFiEvent_t event) {
  switch (event) {
    case SYSTEM_EVENT_STA_GOT_IP:
      Serial.println("WiFi connecté");
      Serial.print("Adresse IP : ");
      Serial.println(WiFi.localIP());
      wifiStatusCharacteristic.setValue("WiFi connecte");
      wifiStatusCharacteristic.notify();
      networkConfigCharacteristic.setValue(ssid);
      networkConfigCharacteristic.notify();
      break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
      Serial.println("Connexion WiFi perdue");
      wifiStatusCharacteristic.setValue("WiFi déconnecte");
      wifiStatusCharacteristic.notify();
      WiFi.begin(ssid, password);
      break;
    default:
      break;
  }
}

void setup() {
  Serial.begin(115200);
  WiFi.onEvent(WiFiEvent);
  WiFi.begin(ssid, password);
  BLEDevice::init("ESP32 chambre 123");
  BLEServer* pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService* pService = pServer->createService(SERVICE_UUID);
  pService->addCharacteristic(&fallCountCharacteristic);
  pService->addCharacteristic(&resetFallCountCharacteristic);
  pService->addCharacteristic(&networkConfigCharacteristic);
  pService->addCharacteristic(&bluetoothStatusCharacteristic);  
  pService->addCharacteristic(&wifiStatusCharacteristic);
  
  pService->start();
  BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->start();
  
  Serial.println("Initialisation terminee");
  delay(1000);
}

void loop() {
  unsigned long currentTime = millis();
  if (currentTime - lastFallTime >= fallInterval) {
    lastFallTime = currentTime;
    fallCount++;
    fallCountCharacteristic.setValue(String(fallCount).c_str());
    fallCountCharacteristic.notify();
    Serial.print("Nombre de chute : ");
    Serial.println(fallCount);
  }

  std::string value = resetFallCountCharacteristic.getValue();
  if (value == "reset") {
    fallCount = 0;
    resetFallCountCharacteristic.setValue("");
    fallCountCharacteristic.setValue(String(fallCount).c_str());
    fallCountCharacteristic.notify();
    Serial.println("Nombre de chute reinitialise");
  }
}
