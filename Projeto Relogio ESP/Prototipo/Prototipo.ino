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
#define MAX_ALARMES 5 // <-- NOVO: Definimos que podemos ter até 5 alarmes

// NOVO: Criamos a "ficha" (struct) para guardar os dados de um alarme
struct Alarme {
  int id;             // Um identificador para cada alarme (de 0 a 4)
  int hora;
  int minuto;
  bool ativo;         // true se o alarme estiver programado, false se o espaço estiver livre
};

// NOVO: Criamos nosso "fichário" (array) para a lista de alarmes
Alarme listaDeAlarmes[MAX_ALARMES];

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
int ano = 0, mes = 0, dia = 0, hora = 0, minuto = 0, segundo = 0;
int semana;

// REMOVA ESTAS LINHAS
//int alarmeHora = -1;
//int alarmeMinuto = -1;
//bool alarmeProgramado = false;

bool alarmeDisparado = false; // Controla se o alarme está *tocando* no momento
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

// ========================================================================================================
// --- Callbacks BLE ---
class MyCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) override {
    String rxValue = pCharacteristic->getValue();
    if (rxValue.length() > 0) {
      Serial.print("Comando BLE recebido: ");
      bleEntrada = String(rxValue.c_str());
      bleEntrada.trim();
      bleNovoHorario = true;
      Serial.println(bleEntrada);
    }
  }
};

// Callback para saber quando o cliente BLE conecta ou desconecta
class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *pServer) override { Serial.println("✅ Dispositivo conectado via BLE"); }
  void onDisconnect(BLEServer *pServer) override {
    Serial.println("❌ Dispositivo desconectado");
    pServer->getAdvertising()->start();
  }
};

// ========================================================================================================
// --- Protótipos das Funções ---
void inicializarAlarmes();
bool adicionarAlarme(int h, int m);
void listarAlarmes();
void DS3231();
void processSerialCommand(String cmd);
void setTime(String cmd);
void verificarAlarme();
void gerenciarAlarme();
void ativarAlarme();
void pararAlarme();
void exibirDataHora();
void exibirAlarmeProgramado(int h, int m);
void exibirAlertaAlarme();
void setupBLE();
void receberHorarioBLE();

// ========================================================================================================
// --- Configurações Iniciais ---
void setup() {
  Serial.begin(115200);

  inicializarAlarmes(); // <-- NOVO: Chamamos a função para limpar a lista
  
  Wire.begin(sda_pin, scl_pin);  // Inicia o I2C nos pinos 2 e 1

  // --- Inicializa o Display OLED (I2C0) ---
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("Falha ao iniciar o display SSD1306"));
    while (1)
      ;
  }

  // --- Inicializa o Módulo RTC (I2C1) ---
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
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  setupBLE();

  pinMode(PINO_VIBRADOR, OUTPUT);
  pinMode(PINO_LED, OUTPUT);
  pinMode(SENSOR_PARADA, INPUT_PULLUP); // Usar PULLUP interno se for um botão para o GND
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

  Serial.println("Sistema iniciado. Comandos: 'add HH:MM', 'list', 'clear', 'set AAAA-MM-DD HH:MM:SS'");
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
  receberHorarioBLE();  // ✅ Nova função para processar entrada via BLE
  verificarAlarme();
  gerenciarAlarme();
}

// ========================================================================================================
// --- Desenvolvimento das Funções ---
// Atualização de horários via RTC -- 
void DS3231() {
  DateTime agora = rtc.now();
  dia = agora.day(); mes = agora.month(); ano = agora.year();
  hora = agora.hour(); minuto = agora.minute(); segundo = agora.second();
  semana = agora.dayOfTheWeek();
  if (!alarmeDisparado) exibirDataHora();
  delay(1000);
}

// ========================================================================================================
// --- Funções de Alarme ---
//Função que percorre a lista e "desativa" todos os alarmes. --
void inicializarAlarmes() {
  Serial.println("Inicializando e limpando a lista de alarmes...");
  for (int i = 0; i < MAX_ALARMES; i++) {
    listaDeAlarmes[i].id = i;
    listaDeAlarmes[i].ativo = false;
    listaDeAlarmes[i].hora = 0;   // Opcional, bom para limpar dados antigos
    listaDeAlarmes[i].minuto = 0; // Opcional
  }
}

// Função para adicionar um alarme na lista --
bool adicionarAlarme(int h, int m) {
  if(alarmeDisparado) {
      Serial.println("Relogio em modo Alarme, adicione novos alarmes após finalizar o alarme atual...");
      return false;
    } else {
      for (int i = 0; i < MAX_ALARMES; i++) {
        if (!listaDeAlarmes[i].ativo && !alarmeDisparado) { // Achou um espaço livre!
          listaDeAlarmes[i].hora = h;
          listaDeAlarmes[i].minuto = m;
          listaDeAlarmes[i].ativo = true;
          Serial.print("SUCESSO: Alarme ");
          Serial.print(i);
          Serial.print(" programado para ");
          if (h < 10) Serial.print("0");
          Serial.print(h);
          Serial.print(":");
          if (m < 10) Serial.print("0");
          Serial.println(m);
          exibirAlarmeProgramado(h, m); // Mostra no display
          return true; // Retorna sucesso
        }
      }
  Serial.println("ERRO: Lista de alarmes cheia!");
  return false; // Retorna falha
  }

}

// Função para listar os alarmes no Serial Monitor --
void listarAlarmes() {
  Serial.println("--- Lista de Alarmes Programados ---");
  bool encontrou = false;
  for (int i = 0; i < MAX_ALARMES; i++) {
    if (listaDeAlarmes[i].ativo) {
      encontrou = true;
      Serial.print("Alarme ID ");
      Serial.print(listaDeAlarmes[i].id);
      Serial.print(": ");
      if (listaDeAlarmes[i].hora < 10) Serial.print("0");
      Serial.print(listaDeAlarmes[i].hora);
      Serial.print(":");
      if (listaDeAlarmes[i].minuto < 10) Serial.print("0");
      Serial.println(listaDeAlarmes[i].minuto);
    }
  }
  if (!encontrou) {
    Serial.println("Nenhum alarme programado.");
  }
  Serial.println("------------------------------------");
}

void verificarAlarme() {
  if (alarmeDisparado) return; // Se um alarme já está tocando, não verifica por novos

  // Percorre toda a lista de alarmes
  for (int i = 0; i < MAX_ALARMES; i++) {
    // Verifica se ESTE alarme da lista está ativo E se a hora bate
    if (listaDeAlarmes[i].ativo && hora == listaDeAlarmes[i].hora && minuto == listaDeAlarmes[i].minuto) {
      
      // Evita disparar múltiplos alarmes ao mesmo tempo se eles coincidirem
      if (alarmeDisparado) { 
        Serial.println("Alarme ignorado, pois outro já está tocando.");
        continue; // Pula para o próximo alarme da lista
      }

      Serial.print("Disparando alarme ID "); Serial.println(i);
      
      ativarAlarme(); // Inicia o processo de vibração/LED
      
      // IMPORTANTE: Desativa o alarme para que ele não dispare novamente no próximo segundo
      listaDeAlarmes[i].ativo = false; 
      
      // Uma vez que um alarme disparou, não precisamos checar os outros neste ciclo.
      // O 'break' sai do loop 'for'.
      break; 
    }
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
  if (/*digitalRead(SENSOR_PARADA) == LOW || */bleEntrada == "parar") {
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

// A função pararAlarme agora só precisa se preocupar em parar o alerta físico
void pararAlarme() {
  digitalWrite(PINO_VIBRADOR, LOW);
  digitalWrite(PINO_LED, LOW);
  // A variável 'alarmeProgramado' não existe mais. Apenas 'alarmeDisparado' controla o estado de "tocando".
  alarmeDisparado = false;
  contagemRepeticoes = 0;
  bleEntrada = "";
  Serial.println("Alarme finalizado.");
}

// ========================================================================================================
// --- Comunicação ---
// Função para processar os comandos
void processSerialCommand(String cmd) {
  Serial.print("\n-> Comando recebido: "); Serial.println(cmd);

  if (cmd.startsWith("set ")) {
    setTime(cmd);
  } else if (cmd.startsWith("add ")) {
    String horarioStr = cmd.substring(4); // Pega a parte "HH:MM"
    if (horarioStr.length() == 5 && horarioStr.charAt(2) == ':') {
      int h = horarioStr.substring(0, 2).toInt();
      int m = horarioStr.substring(3, 5).toInt();
      if (h >= 0 && h < 24 && m >= 0 && m < 60) {
        adicionarAlarme(h, m);
      } else {
          Serial.println("ERRO: Horário inválido.");
      }
    } else {
        Serial.println("ERRO: Formato inválido. Use: add HH:MM");
    }
  } else if (cmd == "list") {
      listarAlarmes();
  } else if (cmd == "clear") {
      inicializarAlarmes(); // Reutilizamos a função para limpar tudo
      Serial.println("SUCESSO: Todos os alarmes foram removidos.");
  } else if (cmd == "parar"){
      pararAlarme();
  } else {
    Serial.println("ERRO: Comando desconhecido. Use 'set', 'add', 'list', ou 'clear'.");
  }
  Serial.println("========================================\n");
}

void receberHorarioBLE() {
  if (bleNovoHorario) {
    bleNovoHorario = false;
    
    // Adapta o processamento BLE para os novos comandos
    if (bleEntrada.startsWith("add ")) {
      String horarioStr = bleEntrada.substring(4);
      if (horarioStr.length() == 5 && horarioStr.charAt(2) == ':'){
        int h = bleEntrada.substring(0, 2).toInt();
        int m = bleEntrada.substring(3, 5).toInt();
        if (h >= 0 && h < 24 && m >= 0 && m < 60) {
          adicionarAlarme(h, m);
        } else {
          Serial.println("ERRO BLE: Horário com valores inválidos.");
        }
      } else {
          Serial.println("ERRO BLE: Formato inválido. Use: add HH:MM");
        }
    } else if (bleEntrada == "list") {
      listarAlarmes();
    } else if (bleEntrada == "clear") {
      inicializarAlarmes();
      Serial.println("SUCESSO BLE: Todos os alarmes foram removidos.");
    }
  }
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
// --- Funções de Exibição ---
void exibirDataHora() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // Centraliza a data
  char dataStr[11];
  sprintf(dataStr, "%02d/%02d/%04d", dia, mes, ano);
  Serial.print(dataStr);
  Serial.print(" | ");
  display.setCursor((SCREEN_WIDTH - (strlen(dataStr) * 6)) / 2, 5);
  display.print(dataStr);
  

  char horaStr[6];
  display.setTextSize(3);
  // Centraliza a hora
  sprintf(horaStr, "%02d:%02d", hora, minuto);
  Serial.print(horaStr);
  Serial.print(":"); Serial.print(segundo);
  Serial.print(" | ");
  Serial.println(diasDaSemana[semana]);
  display.setCursor((SCREEN_WIDTH - (5 * 18)) / 2 + 5, 30);
  display.print(horaStr);

  display.display();
}

// A função foi modificada para receber a hora/minuto a ser exibida
void exibirAlarmeProgramado(int h, int m) {
  display.clearDisplay();

  display.setTextSize(1);
  display.setCursor((128 - 15 * 6) / 2, 10);
  display.print("Alarme Programado");
  display.setCursor((128 - 15 * 1) / 2, 20);
  display.print("para");

  display.setTextSize(3);
  char alarmeStr[6];
  sprintf(alarmeStr, "%02d:%02d", h, m);
  display.setCursor((128 - (5 * 18)) / 2 + 5, 35);
  display.print(alarmeStr);
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
  char horaStr[6];
  sprintf(horaStr, "%02d:%02d", hora, minuto);
  display.setCursor((128 - (5 * 18)) / 2 + 5, 35);
  display.print(horaStr);
  display.display();
}

// ========================================================================================================
// --- Configuração do BLE ---
void setupBLE() {
  // Inicializa BLE
  BLEDevice::init("Dispositivo LEA");
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());  // <- adicionando callback de conexão

  BLEService *pService = pServer->createService(SERVICE_UUID);

  BLECharacteristic *pCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_WRITE);

  pCharacteristic->setCallbacks(new MyCallbacks());
  pCharacteristic->addDescriptor(new BLE2902());

  pService->start();
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->start();

  Serial.println("🔄 Aguardando conexão BLE...");
}
