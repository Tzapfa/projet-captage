#include <SoftwareSerial.h>
#include <LowPower.h>

// ========================
// Classe BluetoothManager
// ========================
class BluetoothManager {
private:
  const int rxPin = 8;
  const int txPin = 9;
  SoftwareSerial bluetoothSerial;

public:
  BluetoothManager() : bluetoothSerial(rxPin, txPin) {}

  void begin() {
    bluetoothSerial.begin(9600);
  }

  String lireCommande() {
    String message = "";
    while (bluetoothSerial.available()) {
      char c = bluetoothSerial.read();
      message += c;
      delay(2); // Petit délai pour laisser arriver les caractères suivants
    }
    return message;
  }
};

// ========================
// Classe CapteurManager
// ========================
class CapteurManager {
private:
  const int turbiditePin = A0;
  const int debitInterruptPin = 3;
  volatile unsigned long pulseCount = 0;
  unsigned long lastPulseTime = 0;
  float debitLps = 0;
  const float IMPULSIONS_PAR_LITRE = 450.0;
  static CapteurManager* instance;

public:
  CapteurManager() {}

  void begin() {
    pinMode(turbiditePin, INPUT);
    attachInterrupt(digitalPinToInterrupt(debitInterruptPin), pulseISR, RISING);
    lastPulseTime = millis();
    instance = this;
  }

  void update() {
    unsigned long maintenant = millis();
    if (maintenant - lastPulseTime >= 1000) {
      noInterrupts();
      unsigned long countCopy = pulseCount;
      pulseCount = 0;
      interrupts();
      debitLps = countCopy / IMPULSIONS_PAR_LITRE;
      lastPulseTime = maintenant;
    }
  }

  float getDebitLps() const { return debitLps; }

  float lireTurbidite() {
    int sensorValue = analogRead(turbiditePin);
    float voltage = sensorValue * (5.0 / 1024.0);
    float ntu = 130.75 * voltage * voltage - 1721.43 * voltage + 5124.52;
    ntu = constrain(ntu, 0, 2000);
    return ntu;
  }

  static void pulseISR() {
    if (instance) instance->pulseCount++;
  }
};

CapteurManager* CapteurManager::instance = nullptr;

// ========================
// Classe LoRaManager
// ========================
class LoRaManager {
private:
  const int rxPin = 10;
  const int txPin = 11;
  SoftwareSerial loraSerial;

public:
  LoRaManager() : loraSerial(rxPin, txPin) {}

  void begin() {
    loraSerial.begin(9600);
  }

  void envoyer(float ntu, float debitLps, const String& commandeBluetooth = "") {
    int turbInt = ntu * 100;
    int debitInt = debitLps * 100;
    String message = "TURB" + String(turbInt) + "DEB" + String(debitInt);
    
    if (commandeBluetooth.length() > 0) {
      message += "CMD" + commandeBluetooth;
    }

    int longueur = message.length();
    String commandeAT = "AT+SEND=0,2," + String(longueur) + "," + message + "\r\n";

    loraSerial.print(commandeAT);

    Serial.print("Message envoyer : ");
    Serial.println(commandeAT);
  }

  void mettreEnSommeil() {
    loraSerial.print("AT+SLEEP\r\n");
  }
};

// ========================
// Classe EnergieManager
// ========================
class EnergieManager {
public:
  void sommeilProlonge(int secondes) {
    int cycles = secondes / 8;
    for (int i = 0; i < cycles; i++) {
      LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
    }
  }
};

// ========================
// Classe principale LoRaMonitor
// ========================
class LoRaMonitor {
private:
  CapteurManager capteurs;
  LoRaManager lora;
  EnergieManager energie;
  BluetoothManager bluetooth;

  const unsigned long dureeActive = 15000;
  const unsigned long intervalEnvoi = 5000;
  const unsigned long dureeSommeil = 59 * 60 + 45;

  unsigned long dernierEnvoi = 0;
  unsigned long debutCycle = 0;

public:
  void begin() {
    Serial.begin(9600);
    capteurs.begin();
    lora.begin();
    bluetooth.begin();
    Serial.println("Demarrage du cycle actif...");
    debutCycle = millis();
  }

  void update() {
    unsigned long maintenant = millis();
    capteurs.update();

    if (maintenant - debutCycle < dureeActive) {
      if (maintenant - dernierEnvoi >= intervalEnvoi) {
        dernierEnvoi = maintenant;

        float ntu = capteurs.lireTurbidite();
        float debit = capteurs.getDebitLps();
        String commandeBluetooth = bluetooth.lireCommande();
        lora.envoyer(ntu, debit, commandeBluetooth);
      }
    } else {
      Serial.println("Fin du cycle actif. Passage en sommeil...");
      lora.mettreEnSommeil();
      delay(100);
      energie.sommeilProlonge(dureeSommeil);
      Serial.println("Reveil du cycle suivant...");
      debutCycle = millis();
      dernierEnvoi = 0;
    }
  }
};

// ========================
// Programme principal
// ========================
LoRaMonitor monitor;

void setup() {
  monitor.begin();
}

void loop() {
  monitor.update();
}
