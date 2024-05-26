#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#define SERVICE_UUID           "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define FALL_COUNT_CHAR_UUID   "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define RESET_FALL_COUNT_UUID  "6d68efe5-04b6-4a4d-aeae-3e97b9b96c3b"
#define NETWORK_CONFIG_UUID    "12345678-1234-1234-1234-1234567890ab"
#define CONNECTION_STATUS_UUID "87654321-4321-4321-4321-abcdefabcdef"

int fallCount = 0;
bool isConnected = false;
unsigned long lastFallTime = 0;
const unsigned long fallInterval = 10000; // 10 seconds

BLECharacteristic fallCountCharacteristic(FALL_COUNT_CHAR_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
BLECharacteristic resetFallCountCharacteristic(RESET_FALL_COUNT_UUID, BLECharacteristic::PROPERTY_WRITE);
BLECharacteristic networkConfigCharacteristic(NETWORK_CONFIG_UUID, BLECharacteristic::PROPERTY_READ);
BLECharacteristic connectionStatusCharacteristic(CONNECTION_STATUS_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);

class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    isConnected = true;
    Serial.println("Device connected");
    pServer->getAdvertising()->stop();  // Stop advertising when connected
  }

  void onDisconnect(BLEServer* pServer) {
    isConnected = false;
    Serial.println("Device disconnected, restarting advertising");
    pServer->getAdvertising()->start();  // Restart advertising on disconnection
  }
};

void setup() {
  Serial.begin(115200);
  BLEDevice::init("ESP32 chambre 123");
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  
  BLEService *pService = pServer->createService(SERVICE_UUID);
  
  pService->addCharacteristic(&fallCountCharacteristic);
  pService->addCharacteristic(&resetFallCountCharacteristic);
  pService->addCharacteristic(&networkConfigCharacteristic);
  pService->addCharacteristic(&connectionStatusCharacteristic);

  pService->start();
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->start();
  Serial.println("Waiting for a client connection to notify...");
}

void loop() {
  // Check if it's time to simulate a fall
  unsigned long currentTime = millis();
  if (currentTime - lastFallTime >= fallInterval) {
    lastFallTime = currentTime;
    fallCount++;
    fallCountCharacteristic.setValue(String(fallCount).c_str());  // Set the value as a string
    fallCountCharacteristic.notify();
    Serial.print("Fall count notified: ");
    Serial.println(fallCount);
  }

  // Handle reset command
  std::string value = resetFallCountCharacteristic.getValue();
  if (value == "reset") {
    fallCount = 0;
    resetFallCountCharacteristic.setValue("");
    fallCountCharacteristic.setValue(String(fallCount).c_str());  // Update the fall count characteristic immediately
    fallCountCharacteristic.notify();  // Notify the client about the reset immediately
    Serial.println("Fall count reset");
  }

  // Set connection status
  if (isConnected) {
    connectionStatusCharacteristic.setValue("Connected");
  } else {
    connectionStatusCharacteristic.setValue("Disconnected");
  }
}
