void setup() {
  Serial.begin(9600);
}

void loop() {
  int sensorValue = analogRead(A0);
  float voltage = sensorValue * (5.0 / 1024.0);

  // Formule de conversion calibrée par tes mesures
 // float ntu = -992.56 * voltage + 2084.38;
  float ntu = 895.8 * voltage * voltage - 3667.86 * voltage + 3753.15;
  ntu = constrain(ntu, 0, 1200); // Limiter à la plage utile


  Serial.print("Voltage : ");
  Serial.print(voltage);
  Serial.print(" V\tNTU : ");
  Serial.println(ntu);

  delay(500);
}
