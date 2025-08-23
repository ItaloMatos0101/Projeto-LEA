#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#define SERVICE_UUID        "91bad492-b950-4226-aa2b-4ede9fa42f59"
#define CHARACTERISTIC_UUID "cba1d466-344c-4be3-ab3f-189f80dd7518"
#define LED_PIN 48

// Callback para leitura dos dados recebidos (características)
class MyCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pCharacteristic) override {
    String rxValue = pCharacteristic->getValue();

    if (rxValue.length() > 0) {
      Serial.print("Comando recebido: ");
      Serial.println(rxValue);

      if (rxValue == "ON") {
        digitalWrite(LED_PIN, HIGH);
        Serial.println("LED LIGADO");
      } else if (rxValue == "OFF") {
        digitalWrite(LED_PIN, LOW);
        Serial.println("LED DESLIGADO");
      } else {
        Serial.println("Comando desconhecido.");
      }
    }
  }
};

// Callback para saber quando o cliente BLE conecta ou desconecta
class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) override {
    Serial.println("✅ Dispositivo conectado via BLE");
  }

  void onDisconnect(BLEServer* pServer) override {
    Serial.println("❌ Dispositivo desconectado");
  }
};

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);

  // Inicializa BLE
  BLEDevice::init("ESP32_LED_Control");
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());  // <- adicionando callback de conexão

  BLEService *pService = pServer->createService(SERVICE_UUID);

  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID,
                                         BLECharacteristic::PROPERTY_WRITE
                                       );

  pCharacteristic->setCallbacks(new MyCallbacks());
  pCharacteristic->addDescriptor(new BLE2902());

  pService->start();
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->start();

  Serial.println("🔄 Aguardando conexão BLE...");
}

void loop() {
  // Nada no loop por enquanto
}
