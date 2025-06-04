#include <SoftwareSerial.h>
#define RX 10
#define TX 11
SoftwareSerial loraSerial(RX, TX);

String inputString = "";      // Buffer pour les données reçues
bool stringComplete = false;  // Indique si une commande est complète

// Variables pour l'envoi automatique
const unsigned long intervalle = 10000;  // Intervalle en millisecondes (10000ms = 10 secondes)
unsigned long tempsPrec = 0;
String messageAuto = "2";  // Message à envoyer automatiquement

void setup() {
  Serial.begin(9600);         // Moniteur Série
  loraSerial.begin(9600);     // Dragino LA66
  inputString.reserve(200);   // Allouer de l'espace au buffer
  Serial.println("Système prêt. Tapez des commandes AT dans le Moniteur Série.");
  Serial.println("Envoi automatique activé toutes les " + String(intervalle/1000) + " secondes.");
}

void loop() {
  // Vérifier si c'est le moment d'envoyer un message automatique
  unsigned long tempsCourant = millis();
  if (tempsCourant - tempsPrec >= intervalle) {
    tempsPrec = tempsCourant;
    envoyerMessageAuto();
  }
  
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

void envoyerMessageAuto() {
  // Calculer la longueur du message
  int longueur = messageAuto.length();
  
  // Construire la commande AT complète
  String commandeAT = "AT+SEND=0,2," + String(longueur) + "," + messageAuto + "\r\n";
  
  // Envoyer la commande au module Dragino
  loraSerial.print(commandeAT);
  
  // Afficher dans le moniteur série pour confirmation
  Serial.print("Message auto envoyé: ");
  Serial.println(commandeAT);
}
