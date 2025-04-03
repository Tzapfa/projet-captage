#include <SoftwareSerial.h>
#define RX 10
#define TX 11
SoftwareSerial loraSerial(RX, TX);

String inputString = "";      // Buffer pour les données reçues
bool stringComplete = false;  // Indique si une commande est complète

void setup() {
  Serial.begin(9600);         // Moniteur Série
  loraSerial.begin(9600);     // Dragino LA66
  inputString.reserve(200);   // Allouer de l'espace au buffer
  Serial.println("Système prêt. Tapez des commandes AT dans le Moniteur Série.");
}

void loop() {
  // Lecture des données du Dragino
  while (loraSerial.available()) {
    char inChar = (char) loraSerial.read();
    inputString += inChar;
    if (inChar == '\n' || inChar == '\r') {
      stringComplete = true;
    }
    
  }

  // Lecture des données du Moniteur Série
  while (Serial.available()) {
   // char inChar = (char) Serial.read();
    char inChar = (char) Serial.read(); 
    inputString += inChar;
    if (inChar == '\n' || inChar == '\r') {
      loraSerial.print(inputString); // Envoyer au Dragino
      inputString = "";              // Réinitialiser le buffer
    }
  }

  // Affichage des réponses complètes dans le Moniteur Série
  if (stringComplete) {
    Serial.print(inputString);
    inputString = "";              // Réinitialiser le buffer
    stringComplete = false;
  }
}
