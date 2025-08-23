void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
}

void loop() {
  // put your main code here, to run repeatedly:
  Serial.print("Valor 5: ");
  Serial.println(analogRead(5));
  Serial.println("=================================================");
  Serial.print("Valor 6: ");
  Serial.println(analogRead(6));
  Serial.println("=================================================");
  Serial.print("Valor 7: ");
  Serial.println(analogRead(7));
  Serial.println("=================================================");
  delay(2000);
}
