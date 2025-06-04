#include <SoftwareSerial.h>
#include <LowPower.h>

class LoRaWaterSensor {
  private:
    SoftwareSerial loraSerial;
    unsigned long DUREE_ACTIVE = 15000;
    unsigned long INTERVAL_ENVOI = 5000;
    unsigned long dernierEnvoi = 0;
    unsigned long debutCycle = 0;
    int TURBIDITE_PIN;
    int DEBIT_INTERRUPT_PIN;
    
    // Make waterFlow static so it can be accessed from static pulse function
    static volatile double waterFlow;
    
  public:
    LoRaWaterSensor(int rxPin, int txPin, int turbidityPin, int flowPin)
      : loraSerial(rxPin, txPin), TURBIDITE_PIN(turbidityPin), DEBIT_INTERRUPT_PIN(flowPin) {}
      
    void setup() {
      Serial.begin(9600);
      loraSerial.begin(9600);
      pinMode(TURBIDITE_PIN, INPUT);
      attachInterrupt(digitalPinToInterrupt(DEBIT_INTERRUPT_PIN), pulse, RISING);
      Serial.println("Démarrage du cycle actif...");
      debutCycle = millis();
      waterFlow = 0; // Reset waterFlow at setup
    }
    
    void loop() {
      unsigned long maintenant = millis();
      if (maintenant - debutCycle < DUREE_ACTIVE) {
        if (maintenant - dernierEnvoi >= INTERVAL_ENVOI) {
          dernierEnvoi = maintenant;
          envoyerMessageAuto();
          waterFlow = 0;
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
        waterFlow = 0;
      }
    }
    
    static void pulse() {
      waterFlow += 1.0 / 150.0;
    }
    
    void envoyerMessageAuto() {
      int sensorValue = analogRead(TURBIDITE_PIN);
      float voltage = sensorValue * (5.0 / 1024.0);
      int turbInt = voltage * 100;
      int debitInt = waterFlow * 100;
      String messageAuto = "TURB" + String(turbInt) + "DEB" + String(debitInt);
      int longueur = messageAuto.length();
      String commandeAT = "AT+SEND=0,2," + String(longueur) + "," + messageAuto + "\r\n";
      loraSerial.print(commandeAT);
      Serial.print("Message envoyé : ");
      Serial.println(commandeAT);
    }
};

// Initialize the static member variable
volatile double LoRaWaterSensor::waterFlow = 0;

LoRaWaterSensor sensor(10, 11, A0, 3);

void setup() {
  sensor.setup();
}

void loop() {
  sensor.loop();
}
