volatile unsigned int pulseCount = 0;  // nombre d'impulsions dans la période
double flowRate = 0;                   // débit en L/s

unsigned long oldTime = 0;

void setup() {
  Serial.begin(9600);
  pulseCount = 0;
  attachInterrupt(digitalPinToInterrupt(3), pulse, RISING);  // Interruption sur pin 3 (INT1)
  oldTime = millis();
}

void loop() {
  unsigned long currentTime = millis();
  if (currentTime - oldTime >= 1000) {  // toutes les 1 seconde
    detachInterrupt(digitalPinToInterrupt(3));  // bloquer interruption pendant calcul

    flowRate = (double)pulseCount / 150.0;  // calcul débit L/s
    Serial.print("Débit instantané : ");
    Serial.print(flowRate);
    Serial.println(" L/s");

    pulseCount = 0;  // remise à zéro du compteur
    oldTime = currentTime;

    attachInterrupt(digitalPinToInterrupt(3), pulse, RISING);  // réactiver interruption
  }
}

void pulse() {
  pulseCount++;  // compteur d'impulsions
}
