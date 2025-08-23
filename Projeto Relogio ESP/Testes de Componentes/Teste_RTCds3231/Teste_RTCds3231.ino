#include <Wire.h>              // Biblioteca I2C principal (será usada para o OLED)
#include <RTClib.h>            // Biblioteca do RTC
#include <Adafruit_GFX.h>      // Biblioteca gráfica
#include <Adafruit_SSD1306.h>  // Biblioteca do Display OLED

#define led 48



// --- Pinos e Configurações do Display OLED (usará o I2C0 padrão) ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

RTC_DS3231 rtc;

// Defina os pinos que você está usando
const int sda_pin = 2;
const int scl_pin = 1;
const int vibra_pin = 8;

// Array para converter o número do dia da semana em texto
// A biblioteca RTClib usa 0 para Domingo, 1 para Segunda, etc.
const char* diasDaSemana[] = {"Domingo", "Segunda-feira", "Terca-feira", "Quarta-feira", "Quinta-feira", "Sexta-feira", "Sabado"};

int dia = 0;
int mes = 0;
int ano = 0;
int hora = 0;
int minuto = 0;
int segundo = 0; 
int horaA, minA;

void setTime(String cmd);
void setAlarme(String cmd);

void setup() {
  // Inicia a comunicação serial
  Serial.begin(115200);
  while (!Serial) {
    ; // Aguarda a porta serial conectar
  }

  pinMode(led, OUTPUT);
  digitalWrite(led, LOW);

  Serial.println("\n===== AJUSTE E LEITURA DO RTC DS3231 =====");

  // Inicia a comunicação I2C nos pinos especificados
  Wire.begin(sda_pin, scl_pin);

  pinMode(vibra_pin, OUTPUT);

  // --- Inicializa o Display OLED (I2C0) ---
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("Falha ao iniciar o display SSD1306"));
    while(1);
  }
  Serial.println("Display OLED OK!");

  // Inicia o módulo RTC
  if (!rtc.begin()) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0,0);
    display.println("Erro no RTC!");
    display.display();
    Serial.println("ERRO: Nao foi possivel encontrar o modulo RTC! Verifique as conexoes.");
    while (1) delay(10);
  }
  Serial.println("Modulo RTC OK!");

  // Verifica se o RTC perdeu energia (bateria fraca ou removida)
  if (rtc.lostPower()) {
    Serial.println("ATENCAO: RTC perdeu energia. A hora precisa ser ajustada!");
  }

  Serial.println("\nRTC pronto para receber comandos.");
  Serial.println("Para AJUSTAR A HORA, envie o comando 'set' seguido da data e hora.");
  Serial.println("FORMATO: set AAAA-MM-DD HH:MM:SS");
  Serial.println("EXEMPLO: set 2025-08-15 16:30:00");
  Serial.println("========================================\n");

  // Mensagem de inicialização no display
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 20);
  display.println("Tudo OK!");
  display.display();
  delay(3000);
}

void loop() {
  //Processa comandos recebidos via Serial
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim(); // Limpa espaços extras
    processSerialCommand(command);
  }

  //Exibir horário no monitor Serial
  exibirSerial();

  //Exibir horário no Display OLED
  exibirDisplay();

  verificaAlarme();

  delay(1000);
}

// Função para processar os comandos (permanece a mesma)
void processSerialCommand(String cmd) {
  Serial.print("\n-> Comando recebido: ");
  Serial.println(cmd);

  if (cmd.startsWith("set Alarme ")) {
    setAlarme(cmd);
  }else if (cmd.startsWith("set ")) {
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

//Função para setar alarme
void setAlarme(String cmd){
  int hour, minute;
  String acionaAlarmeStr = cmd.substring(11);

  int parsed_items = sscanf(acionaAlarmeStr.c_str(), "%d:%d",
                              &hour, &minute);
                              
  if (parsed_items == 2) {
    horaA = hour;
    minA = minute;
    Serial.print("SUCESSO: Alarme programado para: ");
    Serial.println(hour+":"+minute);
  } else {
      Serial.println("ERRO: Formato invalido! Use: set Alarme HH:MM");
  }
}

// Função para exibir horário no Serial Monitor
void exibirSerial(){
  //Exibe a data, hora e o dia da semana a cada segundo
  DateTime now = rtc.now();

  Serial.print("Data: ");
  ano = now.year();
  Serial.print(now.year());
  Serial.print('/');
  mes = now.month();
  if (now.month() < 10) Serial.print('0');
  Serial.print(now.month());
  Serial.print('/');
  dia = now.day();
  if (now.day() < 10) Serial.print('0');
  Serial.print(now.day());

  // Exibe o dia da semana (numérico e por extenso)
  Serial.print(" |");
  Serial.print(" (");
  Serial.print(diasDaSemana[now.dayOfTheWeek()]); // Exibe o nome
  Serial.print(")");

  Serial.print(" | Hora: ");
  hora = now.hour();
  if (now.hour() < 10) Serial.print('0');
  Serial.print(now.hour());
  Serial.print(':');
  minuto = now.minute();
  if (now.minute() < 10) Serial.print('0');
  Serial.print(now.minute());
  Serial.print(':');
  segundo = now.second();
  if (now.second() < 10) Serial.print('0');
  Serial.println(now.second());
}

//Função para exibir horário no Display OLED
void exibirDisplay(){
  display.clearDisplay();
  // --- Exibe a DATA ---
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(25, 5); // Posição para a data (coluna, linha)

  if (dia < 10) display.print('0');
  display.print(dia);
  display.print('/');
  if (mes < 10) display.print('0');
  display.print(mes);
  display.print('/');
  display.print(ano);
  
  // --- Exibe a HORA ---
  display.setTextSize(2); // Fonte maior para a hora
  display.setCursor(15, 25); // Posição para a hora

  if (hora < 10) display.print('0');
  display.print(hora);
  display.print(':');
  if (minuto < 10) display.print('0');
  display.print(minuto);
  display.print(':');
  if (segundo < 10) display.print('0');
  display.print(segundo);

  // Envia tudo para ser exibido na tela
  display.display();
}

void verificaAlarme(){
  if(hora == horaA && minuto == minA){
    digitalWrite(led , HIGH);
  }
}