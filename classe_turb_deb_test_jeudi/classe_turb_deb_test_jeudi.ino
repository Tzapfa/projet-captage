class WaterQualitySensor {
private:
  uint8_t turbidityPin;     // pin analogique pour turbidité
  uint8_t flowInterruptPin; // pin interruption pour débit

  volatile unsigned int pulseCount;  // compteur d'impulsions pour débit
  unsigned long lastFlowMillis;      // temps dernière mesure débit
  double totalLiters;                // total litres cumulés

  static WaterQualitySensor* instance;  // instance statique pour ISR

public:
  WaterQualitySensor(uint8_t turbidityAnalogPin, uint8_t flowIntPin)
    : turbidityPin(turbidityAnalogPin), flowInterruptPin(flowIntPin),
      pulseCount(0), lastFlowMillis(0), totalLiters(0.0) {}

  void begin() {
    pulseCount = 0;
    totalLiters = 0;
    lastFlowMillis = millis();
    instance = this;
    attachInterrupt(digitalPinToInterrupt(flowInterruptPin), WaterQualitySensor::pulseISR, RISING);
  }

  // Lire la tension brute capteur turbidité
  float readVoltage() {
    int sensorValue = analogRead(turbidityPin);
    return sensorValue * (5.0 / 1024.0);
  }

  // Calculer NTU à partir de la tension (formule calibrée)
  float readNTU() {
    float voltage = readVoltage();
    float ntu = 895.8 * voltage * voltage - 3667.86 * voltage + 3753.15;
    return constrain(ntu, 0, 1200);
  }

  // Appelé dans loop pour calculer le débit instantané en L/s toutes les 1s
  // Retourne -1 si pas encore 1s écoulée
  float getFlowRate() {
    unsigned long currentMillis = millis();
    if (currentMillis - lastFlowMillis >= 1000) {
      noInterrupts();
      unsigned int count = pulseCount;
      pulseCount = 0;
      interrupts();

      lastFlowMillis = currentMillis;

      double flow = (double)count / 150.0; // 150 pulses = 1L
      totalLiters += flow;
      return flow;
    }
    return -1;
  }

  // Retourne la quantité totale d'eau mesurée (L)
  double getTotalLiters() {
    return totalLiters;
  }

private:
  // ISR statique appelée sur interruption
  static void pulseISR() {
    if (instance != nullptr) {
      instance->pulseCount++;
    }
  }
};

WaterQualitySensor* WaterQualitySensor::instance = nullptr;


WaterQualitySensor sensor(A0, 3);

void setup() {
  Serial.begin(9600);
  sensor.begin();
}

void loop() {
  float ntu = sensor.readNTU();
  float voltage = sensor.readVoltage();
  float flow = sensor.getFlowRate();  // débit instantané L/s, ou -1 si pas prêt

  Serial.print("Turbidité: ");
  Serial.print(ntu);
  Serial.print(" NTU, Voltage: ");
  Serial.print(voltage);
  Serial.print(" V\t");
  Serial.print("Débit: ");
  Serial.print(flow);
  Serial.print(" L/s  \n");
  


  delay(500);
}

