// Define o pino que irá controlar o transistor do motor de vibração
// Usamos o pino 25, que é uma escolha segura no ESP32.
const int pinoVibracall = 25;

void setup() {
  // Inicia a comunicação serial para mensagens de status
  Serial.begin(115200);
  Serial.println("Teste do Motor de Vibracao");

  // Configura o pino do vibracall como uma saída (OUTPUT)
  pinMode(pinoVibracall, OUTPUT);
  
  // Garante que o motor comece desligado
  digitalWrite(pinoVibracall, LOW); 
}

void loop() {
  Serial.println("Vibrando...");
  digitalWrite(pinoVibracall, HIGH); // Liga o transistor, que liga o motor
  delay(500); // Mantém vibrando por meio segundo

  Serial.println("Parado.");
  digitalWrite(pinoVibracall, LOW); // Desliga o transistor, que desliga o motor
  delay(2000); // Mantém parado por 2 segundos
}
