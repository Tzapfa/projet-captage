#include <SoftwareSerial.h>
#include <LowPower.h>

class LoRaMonitor {
private:
  const int rxPin = 10;
  const int txPin = 11;
  const int turbiditePin = A0;
  const int debitInterruptPin = 3;
  const unsigned long dureeActive = 150000;  // 150 secondes en mode actif
  const unsigned long intervalEnvoi = 5000;  // 5 secondes entre chaque envoi
  
  // Variables pour le capteur de débit
  volatile unsigned long pulseCount = 0;     // Compteur d'impulsions brut
  unsigned long lastPulseTime = 0;           // Pour calculer le débit instantané
  float debitLitresParSeconde = 0;           // Débit en L/s
  const float IMPULSIONS_PAR_LITRE = 450.0;  // Selon la documentation DFRobot SEN0545
  
  unsigned long dernierEnvoi = 0;
  unsigned long debutCycle = 0;
  SoftwareSerial loraSerial;

public:
  LoRaMonitor() : loraSerial(rxPin, txPin) {}
  
  void begin() {
    Serial.begin(9600);
    loraSerial.begin(9600);
    pinMode(turbiditePin, INPUT);
    attachInterrupt(digitalPinToInterrupt(debitInterruptPin), pulseISR, RISING);
    instance = this;
    Serial.println("Démarrage du cycle actif...");
    debutCycle = millis();
    lastPulseTime = millis();
  }
  
  void update() {
    unsigned long maintenant = millis();
    
    // Calculer le débit en L/s toutes les 1000ms
    if (maintenant - lastPulseTime >= 1000) {
      // Calculer le débit en litres par seconde
      noInterrupts(); // Désactiver les interruptions pendant la lecture
      unsigned long countCopy = pulseCount;
      pulseCount = 0; // Réinitialiser le compteur
      interrupts(); // Réactiver les interruptions
      
      // Calculer le débit en L/s
      float litres = countCopy / IMPULSIONS_PAR_LITRE;
      debitLitresParSeconde = litres; // Déjà sur 1 seconde
      
      lastPulseTime = maintenant;
    }
    
    if (maintenant - debutCycle < dureeActive) {
      if (maintenant - dernierEnvoi >= intervalEnvoi) {
        dernierEnvoi = maintenant;
        envoyerMessageAuto();
      }
    } else {
      Serial.println("Fin du cycle actif. Passage en sommeil...");
      loraSerial.print("AT+SLEEP\r\n");
      delay(100);
      for (int i = 0; i < 224; i++) {
        LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
        LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
      }
      Serial.println("Réveil du cycle suivant...");
      debutCycle = millis();
      dernierEnvoi = 0;
      noInterrupts();
      pulseCount = 0;
      interrupts();
      debitLitresParSeconde = 0;
      lastPulseTime = millis();
    }
  }
  
private:
  void envoyerMessageAuto() {
    // Lecture du capteur de turbidité
    int sensorValue = analogRead(turbiditePin);
    float voltage = sensorValue * (5.0 / 1024.0);
    
    // Conversion en NTU (UTN) selon la formule de calibration du capteur Grove
    float ntu = -1120.4 * voltage * voltage + 5742.3 * voltage - 4352.9;
    // Limiter les valeurs négatives ou trop grandes
    if (ntu < 0) ntu = 0;
    if (ntu > 3000) ntu = 3000;
    
    // Préparer les valeurs pour l'envoi (multiplier par 100 pour conserver 2 décimales)
    int turbInt = ntu * 100;
    int debitInt = debitLitresParSeconde * 100;
    
    // Création du message
    String messageAuto = "TURB" + String(turbInt) + "DEB" + String(debitInt);
    int longueur = messageAuto.length();
    String commandeAT = "AT+SEND=0,2," + String(longueur) + "," + messageAuto + "\r\n";

    
    // Envoi du message
    loraSerial.print(commandeAT);
    
    // Debug sur le port série
    Serial.print("Message envoyé : ");
    Serial.println(commandeAT);
    Serial.print("Turbidité: ");
    Serial.print(ntu);
    Serial.print(" NTU, Débit: ");
    Serial.print(debitLitresParSeconde);
    Serial.println(" L/s");
  }
  
  static void pulseISR() {
    if (instance != nullptr) {
      instance->pulseCount++;
    }
  }
  
  static LoRaMonitor* instance;
};

// Déclaration du pointeur statique
LoRaMonitor* LoRaMonitor::instance = nullptr;

// --- Programme principal ---
LoRaMonitor monitor;

void setup() {
  monitor.begin();
}

void loop() {
  monitor.update();
}
