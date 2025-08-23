#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// --- Configuração do Display OLED ---
// A maioria dos displays 128x64 usa este tamanho
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// Declara o objeto do display. -1 significa que não estamos usando um pino de reset.
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// --- Configuração dos Pinos I2C ---
const int sda_pin = 5;
const int scl_pin = 4;

void setup() {
  Serial.begin(115200);

  // Inicia a comunicação I2C nos pinos customizados
  Wire.begin(sda_pin, scl_pin);

  // Tenta inicializar o display. O endereço I2C 0x3C é o mais comum para displays 128x64.
  // Se não funcionar, tente 0x3D.
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("Falha ao iniciar o display SSD1306"));
    // Trava a execução se não encontrar o display
    for(;;);
  }

  Serial.println("Display OLED iniciado com sucesso!");

  // Limpa o buffer do display (deixa a tela preta)
  display.clearDisplay();

  // Define as propriedades do texto
  display.setTextSize(1);      // Tamanho da fonte. 1 é o menor.
  display.setTextColor(SSD1306_WHITE); // Cor do texto (branco)
  
  // Define a posição do cursor (coluna=0, linha=0)
  display.setCursor(0, 0);

  // Escreve um texto de teste
  display.println(F("Ola, Mundo!"));
  display.println(); // Pula uma linha

  display.setTextSize(2); // Aumenta o tamanho da fonte
  display.println(F("Teste OK!"));

  // Envia todo o conteúdo do buffer para ser exibido na tela
  display.display(); 
}

void loop() {
  // Para este teste inicial, não precisamos fazer nada no loop.
}