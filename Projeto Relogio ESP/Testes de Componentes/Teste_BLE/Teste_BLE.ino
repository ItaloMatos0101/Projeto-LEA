#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

BLEServer* pServer = nullptr;
BLECharacteristic* pCharacteristic = nullptr;

bool deviceConnected = false;

#define SERVICE_UUID        "12345678-1234-1234-1234-1234567890ab"
#define CHARACTERISTIC_UUID "abcd1234-5678-90ab-cdef-1234567890ab"

class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
    Serial.println("Dispositivo conectado via BLE!");
  }

  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
    Serial.println("Dispositivo desconectado.");
  }
};

void setup() {
  Serial.begin(115200);
  Serial.println("Inicializando BLE no ESP32-S3...");

  BLEDevice::init("ESP32S3-BLE-Test"); // Nome do dispositivo BLE
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService* pService = pServer->createService(SERVICE_UUID);

  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE  |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );

  pCharacteristic->setValue("Hello BLE");
  pCharacteristic->addDescriptor(new BLE2902());

  pService->start();

  // Começa a anunciar
  BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->start();

  Serial.println("BLE em modo de anúncio.");
}

void loop() {
  // Se estiver conectado, envia uma notificação a cada 2 segundos
  if (deviceConnected) {
    static unsigned long lastTime = 0;
    if (millis() - lastTime > 2000) {
      lastTime = millis();
      String value = "Ping " + String(millis() / 1000);
      pCharacteristic->setValue(value.c_str());
      pCharacteristic->notify();
      Serial.println("Notificação enviada: " + value);
    }
  }
}
