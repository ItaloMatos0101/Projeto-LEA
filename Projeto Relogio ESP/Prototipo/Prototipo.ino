// ========================================================================================================
/*
  UNIVERSIDADE ESTADUAL DO MARANHÂO
  LEA
  PROJETISTA: ITALO MATOS
*/
// ========================================================================================================
// --- Bibliotecas Auxiliares ---
#include <Wire.h>              // Biblioteca I2C principal (será usada para o OLED)
#include <RTClib.h>            // Biblioteca do RTC
#include <Adafruit_GFX.h>      // Biblioteca gráfica
#include <Adafruit_SSD1306.h>  // Biblioteca do Display OLED

#include <BLEDevice.h>  //bibliotecas para o Bluetooth BLE
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// ========================================================================================================
// --- Mapeamento de Hardware ---
const int sda_pin = 2;
const int scl_pin = 1;

// Defina a largura e altura do seu display OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define PINO_VIBRADOR 4
#define PINO_LED 48
#define SENSOR_PARADA 5  // Sensor de parada do alarme

// ========================================================================================================
// --- Declaração de Objetos ---
//As linhas de codigo a seguir devem ser comentadas, ou descomentadas, de acordo com o modelo de RTC utilizado (DS1307 ou DS3132)
RTC_DS1307 rtc;  //Objeto rtc da classe DS1307
//RTC_DS3231 rtc; //Objeto rtc da classe DS3132

char diasDaSemana[7][12] = { "Domingo", "Segunda", "Terca", "Quarta", "Quinta", "Sexta", "Sabado" };  //Dias da semana

// Cria um objeto display com I2C no endereço padrão (0x3C)
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// --- Definições BLE ---
#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
//BLECharacteristic *pCharacteristic = nullptr;


// ========================================================================================================
// -------------------- Variáveis Globais --------------------
int ano = 0;
int mes = 0;
int dia = 0;
int hora = 0;
int minuto = 0;

int alarmeHora = -1;
int alarmeMinuto = -1;
bool alarmeProgramado = false;
bool alarmeDisparado = false;

unsigned long tempoInicioFase = 0;
unsigned long ultimoToggle = 0;
bool estadoOscilacao = false;

const unsigned long DURACAO_TOQUE_MS = 60000;  // 1 minuto de alarme
const unsigned long DURACAO_PAUSA_MS = 60000;  // 1 minuto de pausa
const int REPETICOES_ALARME = 4;

int contagemRepeticoes = 0;
bool emPausa = false;

bool bleNovoHorario = false;
String bleEntrada = "";

// Callback para leitura dos dados recebidos (características)
class MyCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) override {
    String rxValue = pCharacteristic->getValue();
    if (rxValue.length() > 0) {
      Serial.print("Comando recebido: ");
      bleEntrada = String(rxValue.c_str());
      bleEntrada.trim();
      bleNovoHorario = true;
      Serial.print("BLE recebido: ");
      Serial.println(bleEntrada);
    }
  }
};

// Callback para saber quando o cliente BLE conecta ou desconecta
class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *pServer) override {
    Serial.println("✅ Dispositivo conectado via BLE");
  }

  void onDisconnect(BLEServer *pServer) override {
    Serial.println("❌ Dispositivo desconectado");
    Serial.println("📢 Reiniciando advertising para novas conexões...");
    // Linha crucial adicionada para reiniciar o advertising
    pServer->getAdvertising()->start();
  }
};

// ========================================================================================================
// --- Protótipos das Funções ---
void DS3231();
void receberHorarioSerial();
void verificarAlarme();
void gerenciarAlarme();
void ativarAlarme();
void pararAlarme();
void exibirDataHora();
void exibirAlarmeProgramado();
void exibirAlertaAlarme();
void setupBLE();
void receberHorarioBLE();

// ========================================================================================================
// --- Configurações Iniciais ---
void setup() {
  Serial.begin(115200);

  // --- Inicializa o Display OLED (I2C0) ---
  Wire.begin(sda_pin, scl_pin);  // Inicia o primeiro I2C nos pinos 4 e 5
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("Falha ao iniciar o display SSD1306"));
    while (1)
      ;
  }

  // --- Inicializa o Módulo RTC (I2C1) ---
 // Inicia o segundo I2C nos pinos 2 e 1
  if (!rtc.begin()) {               // Passa o objeto I2C_RTC para a biblioteca do relógio
    Serial.println("Nao foi possivel encontrar o modulo RTC!");
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("Erro no RTC!");
    display.display();
    while (1)
      ;
  }

  // --- Calibração do RTC ---
  // ===================================================================================
  // A LINHA ABAIXO CALIBRA O RTC COM A HORA E DATA DO SEU COMPUTADOR.
  // 1. DESCOMENTE a linha abaixo e carregue o código UMA VEZ para acertar o relógio.
  // 2. DEPOIS, COMENTE a linha novamente e carregue o código mais uma vez.
  //    Isso evita que o relógio seja resetado toda vez que o ESP32 ligar.
  // ===================================================================================
  //rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  setupBLE();

  pinMode(PINO_VIBRADOR, OUTPUT);
  pinMode(PINO_LED, OUTPUT);
  pinMode(SENSOR_PARADA, INPUT);
  digitalWrite(PINO_VIBRADOR, LOW);
  digitalWrite(PINO_LED, LOW);


  // Mensagem de inicialização no display
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 8);
  display.println("INICIANDO O SISTEMA");
  display.setTextSize(3);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor((128 - 12 * 8) / 2, 30);
  display.println("LEA");
  display.display();
  delay(3000);


  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  exibirDataHora();  // Exibe hora inicial

  Serial.println("Sistema iniciado. Envie o horário no formato HH:MM");
}

// ========================================================================================================
// --- Loop Infinito ---
void loop() {
  //Processa comandos recebidos via Serial
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim(); // Limpa espaços extras
    processSerialCommand(command);
  }

  DS3231();
  receberHorarioSerial();
  receberHorarioBLE();  // ✅ Nova função para processar entrada via BLE
  verificarAlarme();
  gerenciarAlarme();
}

// ========================================================================================================
// --- Desenvolvimento das Funções ---
void DS3231() {
  // Lê a data e hora atuais do RTC
  DateTime agora = rtc.now();

  dia = agora.day();
  Serial.print("Data: ");
  Serial.print(agora.day(), DEC);  //Imprime dia
  Serial.print('/');               //Imprime barra
  mes = agora.month();
  Serial.print(agora.month(), DEC);  //Imprime mes
  Serial.print('/');                 //Imprime barra
  ano = agora.year();
  Serial.print(agora.year(), DEC);                   //Imprime ano
  Serial.print(" / Dia da semana: ");                //Imprime texto
  Serial.print(diasDaSemana[agora.dayOfTheWeek()]);  //Imprime dia da semana
  Serial.print(" / Horas: ");                        //Imprime texto
  hora = agora.hour();
  Serial.print(agora.hour(), DEC);  //Imprime hora
  Serial.print(':');                //Imprime dois pontos
  // Adiciona um zero à esquerda para minutos e segundos < 10
  minuto = agora.minute();
  if (agora.minute() < 10) Serial.print('0');
  Serial.print(agora.minute(), DEC);
  Serial.print(':');
  if (agora.second() < 10) Serial.print('0');
  Serial.println(agora.second(), DEC);

  if (!alarmeDisparado && !emPausa) exibirDataHora();

  delay(1000);
}

// ========================================================================================================

// Função para processar os comandos (permanece a mesma)
void processSerialCommand(String cmd) {
  Serial.print("\n-> Comando recebido: ");
  Serial.println(cmd);

  if (cmd.startsWith("set ")) {
    setTime(cmd);
  } else {
    Serial.println("ERRO: Comando desconhecido. Use 'set ...'");
  }
  Serial.println("========================================\n");
}

//Função para setar horário por Serial
void setTime(String cmd){
  int year, month, day, hour, minute, second;
    String datetimeStr = cmd.substring(4);
    int parsed_items = sscanf(datetimeStr.c_str(), "%d-%d-%d %d:%d:%d",
                              &year, &month, &day, &hour, &minute, &second);

    if (parsed_items == 6) {
      DateTime newTime = DateTime(year, month, day, hour, minute, second);
      rtc.adjust(newTime);
      Serial.println("SUCESSO: RTC ajustado com a nova data e hora!");
      // Após o ajuste, o dia da semana será calculado automaticamente.
    } else {
      Serial.println("ERRO: Formato invalido! Use: set AAAA-MM-DD HH:MM:SS");
    }
}

// ========================================================================================================
// --- Funções de Display ---
void exibirDataHora() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // Centraliza a data
  display.setCursor((128 - 12 * 5) / 2, 5);  // Aproximadamente 60px de largura
  display.print(dia < 10 ? "0" : "");
  display.print(dia);
  display.print("/");
  display.print(mes < 10 ? "0" : "");
  display.print(mes);
  display.print("/");
  display.print(ano);

  display.setTextSize(3);
  // Centraliza a hora
  display.setCursor((128 - 12 * 7) / 2, 30);
  display.print(hora < 10 ? "0" : "");
  display.print(hora);
  display.print(":");
  display.print(minuto < 10 ? "0" : "");
  display.print(minuto);

  display.display();
}


void exibirAlarmeProgramado() {
  display.clearDisplay();

  display.setTextSize(1);
  display.setCursor((128 - 15 * 6) / 2, 10);
  display.print("Alarme Programado");
  display.setCursor((128 - 15 * 1) / 2, 20);
  display.print("para");

  display.setTextSize(3);
  display.setCursor((128 - 12 * 7) / 2, 35);
  display.print(alarmeHora < 10 ? "0" : "");
  display.print(alarmeHora);
  display.print(":");
  display.print(alarmeMinuto < 10 ? "0" : "");
  display.print(alarmeMinuto);

  display.display();
  delay(3000);  // Mostrar por 3 segundos
}


void exibirAlertaAlarme() {
  display.clearDisplay();

  display.setTextSize(1.5);
  display.setCursor((128 - 15 * 8) / 2, 5);
  display.print("!!! MEDICAMENTO !!!");

  display.setTextSize(3);
  display.setCursor((128 - 12 * 7) / 2, 35);
  display.print(hora < 10 ? "0" : "");
  display.print(hora);
  display.print(":");
  display.print(minuto < 10 ? "0" : "");
  display.print(minuto);

  display.display();
}

// ========================================================================================================
// --- Configuração do BLE ---
void setupBLE() {
  // Inicializa BLE
  BLEDevice::init("ESP32_Alarme");
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());  // <- adicionando callback de conexão

  BLEService *pService = pServer->createService(SERVICE_UUID);

  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_WRITE);

  pCharacteristic->setCallbacks(new MyCallbacks());
  pCharacteristic->addDescriptor(new BLE2902());

  pService->start();
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->start();

  Serial.println("🔄 Aguardando conexão BLE...");
}

void receberHorarioBLE() {
  if (bleNovoHorario) {
    bleNovoHorario = false;

    if (bleEntrada.length() == 5 && bleEntrada.charAt(2) == ':') {
      int h = bleEntrada.substring(0, 2).toInt();
      int m = bleEntrada.substring(3, 5).toInt();

      if (h >= 0 && h < 24 && m >= 0 && m < 60) {
        alarmeHora = h;
        alarmeMinuto = m;
        alarmeProgramado = true;
        alarmeDisparado = false;
        contagemRepeticoes = 0;
        emPausa = false;

        Serial.print("Alarme programado via BLE para ");
        Serial.print(alarmeHora);
        Serial.print(":");
        if (alarmeMinuto < 10) Serial.print("0");
        Serial.println(alarmeMinuto);

        exibirAlarmeProgramado();

      } else {
        Serial.println("Horário via BLE inválido.");
      }
    } else {
      Serial.println("Formato via BLE incorreto. Use HH:MM");
    }
  }
}

// ========================================================================================================
// --- Funções de Alarme ---
void receberHorarioSerial() {
  if (Serial.available() > 0) {
    String entrada = Serial.readStringUntil('\n');
    entrada.trim();

    if (entrada.length() == 5 && entrada.charAt(2) == ':') {
      int h = entrada.substring(0, 2).toInt();
      int m = entrada.substring(3, 5).toInt();

      if (h >= 0 && h < 24 && m >= 0 && m < 60) {
        alarmeHora = h;
        alarmeMinuto = m;
        alarmeProgramado = true;
        alarmeDisparado = false;
        contagemRepeticoes = 0;
        emPausa = false;
        Serial.print("Alarme programado para ");
        Serial.print(alarmeHora);
        Serial.print(":");
        if (alarmeMinuto < 10) Serial.print("0");
        Serial.println(alarmeMinuto);
        exibirAlarmeProgramado();

      } else {
        Serial.println("Horário inválido.");
      }
    } else {
      Serial.println("Formato incorreto. Use HH:MM");
    }
  }
}

void verificarAlarme() {
  if (alarmeProgramado && !alarmeDisparado && hora == alarmeHora && minuto == alarmeMinuto) {
    ativarAlarme();
  }
}

void ativarAlarme() {
  Serial.println("Alarme ativado!");
  alarmeDisparado = true;
  tempoInicioFase = millis();
  ultimoToggle = millis();
  estadoOscilacao = false;
  emPausa = false;
  contagemRepeticoes = 0;
}

void gerenciarAlarme() {
  if (!alarmeDisparado) return;

  // Verifica o sensor digital
  //bool valorSensor = digitalRead(SENSOR_PARADA);
  if (/*valorSensor == LOW || */bleEntrada == "parar") {
    pararAlarme();
    return;
  }

  unsigned long tempoAtual = millis();

  if (!emPausa) {
    // Durante o toque
    exibirAlertaAlarme();
    if (tempoAtual - tempoInicioFase < DURACAO_TOQUE_MS) {
      if (tempoAtual - ultimoToggle >= 1000) {
        ultimoToggle = tempoAtual;
        estadoOscilacao = !estadoOscilacao;
        digitalWrite(PINO_VIBRADOR, estadoOscilacao ? HIGH : LOW);
        digitalWrite(PINO_LED, estadoOscilacao ? HIGH : LOW);
      }
    } else {
      // Fim do toque
      digitalWrite(PINO_VIBRADOR, LOW);
      digitalWrite(PINO_LED, LOW);
      tempoInicioFase = tempoAtual;
      emPausa = true;
      Serial.println("Iniciando pausa...");
    }
  } else {
    // Durante a pausa
    exibirDataHora();
    if (tempoAtual - tempoInicioFase >= DURACAO_PAUSA_MS) {
      contagemRepeticoes++;
      if (contagemRepeticoes >= REPETICOES_ALARME) {
        pararAlarme();
      } else {
        Serial.print("Nova repetição: ");
        Serial.println(contagemRepeticoes + 1);
        emPausa = false;
        tempoInicioFase = tempoAtual;
      }
    }
  }
}

void pararAlarme() {
  digitalWrite(PINO_VIBRADOR, LOW);
  digitalWrite(PINO_LED, LOW);
  alarmeProgramado = false;
  alarmeDisparado = false;
  contagemRepeticoes = 0;
  Serial.println("Alarme finalizado.");
}
