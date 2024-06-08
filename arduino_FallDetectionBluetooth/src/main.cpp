#include <NimBLEDevice.h>
#include <painlessMesh.h>
#include <set>

#define SERVICE_UUID                "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define FALL_COUNT_CHAR_UUID        "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define RESET_FALL_COUNT_UUID       "6d68efe5-04b6-4a4d-aeae-3e97b9b96c3b"
#define NETWORK_CONFIG_UUID         "12345678-1234-1234-1234-1234567890ab"
#define BLUETOOTH_STATUS_UUID       "87654321-4321-4321-4321-abcdefabcdef"
#define WIFI_STATUS_UUID            "abcdef12-1234-5678-9abc-def123456789"

// #define MESH_PREFIX "alexis_robin"
// #define MESH_PASSWORD "alexis_robin"
// #define MESH_PORT 5555

#define MESH_PREFIX "alexis_robin"
#define MESH_PASSWORD "alexis_robin"
#define MESH_PORT 5555

Scheduler userScheduler;
painlessMesh mesh;

const int touchPin = 1;
const int roomNumber = 043;
bool lastTouchState = LOW;
int touchCount = 0;
std::set<uint32_t> connectedNodes;

NimBLECharacteristic* fallCountCharacteristic;
NimBLECharacteristic* resetFallCountCharacteristic;
NimBLECharacteristic* networkConfigCharacteristic;
NimBLECharacteristic* bluetoothStatusCharacteristic;
NimBLECharacteristic* wifiStatusCharacteristic;

bool isConnected = false;

void broadcastNodeCount();

Task taskBroadcastNodeCount(TASK_SECOND * 10, TASK_FOREVER, &broadcastNodeCount);

class MyServerCallbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer* pServer) {
    isConnected = true;
    Serial.println("Bluetooth connecte");
    pServer->getAdvertising()->stop();

    bluetoothStatusCharacteristic->setValue("Bluetooth connecte");
    bluetoothStatusCharacteristic->notify();

    wifiStatusCharacteristic->setValue("WiFi mesh connecte");
    networkConfigCharacteristic->setValue(MESH_PREFIX);
    wifiStatusCharacteristic->notify();
    networkConfigCharacteristic->notify();

    Serial.printf("Current fall count: %d\n", touchCount);
    fallCountCharacteristic->setValue(String(touchCount));
    fallCountCharacteristic->notify();
  }

  void onDisconnect(NimBLEServer* pServer) {
    isConnected = false;
    Serial.println("Bluetooth déconnecté, relance de la publicité");
    pServer->getAdvertising()->start();

    bluetoothStatusCharacteristic->setValue("Bluetooth déconnecté");
    bluetoothStatusCharacteristic->notify();
  }
};

void sendMessage() {
  touchCount++;
  String msg = "Chute: chambre " + String(roomNumber) + "; Count: " + String(touchCount) + "; Node ID: " + String(mesh.getNodeId());
  //String msg = " ,Patient ID: robin, Room: 003 ,  ";
  mesh.sendBroadcast(msg);
  Serial.printf("%s\n", msg.c_str());
  fallCountCharacteristic->setValue(String(touchCount));
  fallCountCharacteristic->notify();
}

void receivedCallback(uint32_t from, String &msg) {
    Serial.printf("%s\n", msg.c_str());
}


void newConnectionCallback(uint32_t nodeId) {
  Serial.printf("--> startHere: New Connection, nodeId = %u\n", nodeId);
  connectedNodes.insert(nodeId);
  Serial.printf("Number of connected nodes: %d\n", connectedNodes.size());
}

void changedConnectionCallback() {
  Serial.printf("Changed connections\n");
  connectedNodes.clear();
  auto nodes = mesh.getNodeList();
  for (auto node : nodes) {
    connectedNodes.insert(node);
  }
  Serial.printf("Number of connected nodes: %d\n", connectedNodes.size());
}

void nodeTimeAdjustedCallback(int32_t offset) {
  Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(), offset);
}

void broadcastNodeCount() {
  String msg = "Number of connected nodes: " + String(connectedNodes.size());
  mesh.sendBroadcast(msg);
  Serial.printf("%s\n", msg.c_str());
}

void setup() {
  Serial.begin(115200);
  pinMode(touchPin, INPUT);

  NimBLEDevice::init("ESP32 chambre 043");
  NimBLEServer* pServer = NimBLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  NimBLEService* pService = pServer->createService(SERVICE_UUID);

  fallCountCharacteristic = pService->createCharacteristic(FALL_COUNT_CHAR_UUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
  resetFallCountCharacteristic = pService->createCharacteristic(RESET_FALL_COUNT_UUID, NIMBLE_PROPERTY::WRITE);
  networkConfigCharacteristic = pService->createCharacteristic(NETWORK_CONFIG_UUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
  bluetoothStatusCharacteristic = pService->createCharacteristic(BLUETOOTH_STATUS_UUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
  wifiStatusCharacteristic = pService->createCharacteristic(WIFI_STATUS_UUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);

  pService->start();
  NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->start();

  Serial.println("Bluetooth initialisation terminée");

  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);

  userScheduler.addTask(taskBroadcastNodeCount);
  taskBroadcastNodeCount.enable();

  Serial.println("Mesh initialisation terminee");
  delay(1000);
}

void loop() {
  mesh.update();

  bool currentTouchState = digitalRead(touchPin);
  if (currentTouchState == HIGH && lastTouchState == LOW) {
    sendMessage();
  }
  lastTouchState = currentTouchState;

  std::string value = resetFallCountCharacteristic->getValue();
  if (value == "reset") {
    touchCount = 0;
    resetFallCountCharacteristic->setValue("");
    fallCountCharacteristic->setValue(String(touchCount));
    fallCountCharacteristic->notify();
    Serial.println("Nombre de chute reinitialise");
  }
}