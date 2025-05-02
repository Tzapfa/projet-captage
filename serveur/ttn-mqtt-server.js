// server.js
// Serveur Node.js pour récupérer les données TTI via MQTT et les exposer en REST
const express = require('express');
const mqtt = require('mqtt');
const cors = require('cors');
const https = require('https');
const fs = require('fs');
const { HttpsProxyAgent } = require('https-proxy-agent');
const app = express();
const PORT = process.env.PORT || 3000;

// Configuration du proxy
const PROXY_CONFIG = {
  host: '10.129.254.254',
  port: 3128
};

// Configuration TTI
const TTI_CONFIG = {
  region: 'eu2',  // Région TTI (eu1, eu2, nam1, au1, etc.)
  applicationId: 'appli-captage',
  apiKey: 'NNSXS.FWBI4YL6EISYPDI4J4HLUH6LRHOZ666COCVDS5I.AFUABQI76OFUMN5QX3YNOTEENAJYB3DO3VKQLWF4BVJU5NOEL2WQ' // Clé d'API avec droits de lecture
};

// Stockage des données reçues
let sensorData = [];
const MAX_DATA_POINTS = 100; // Nombre maximum de points de données à conserver

// Configuration MQTT avec proxy
const mqttOptions = {
  username: TTI_CONFIG.applicationId,
  password: TTI_CONFIG.apiKey,
  rejectUnauthorized: false,
  agent: new HttpsProxyAgent(`http://${PROXY_CONFIG.host}:${PROXY_CONFIG.port}`)
};

// Connexion au broker MQTT de TTI
const mqttUrl = `mqtts://${TTI_CONFIG.region}.cloud.thethings.industries:8883`;
const mqttClient = mqtt.connect(mqttUrl, mqttOptions);

// Gestion des événements MQTT
mqttClient.on('connect', () => {
  console.log('Connecté au broker MQTT TTI');
  
  // S'abonner à tous les appareils de l'application
  const topic = `v3/${TTI_CONFIG.applicationId}/devices/+/up`;
  mqttClient.subscribe(topic, (err) => {
    if (err) {
      console.error('Erreur lors de l\'abonnement:', err);
    } else {
      console.log(`Abonné au topic: ${topic}`);
    }
  });
});

mqttClient.on('error', (error) => {
  console.error('Erreur MQTT:', error);
});

mqttClient.on('message', (topic, message) => {
  try {
    const data = JSON.parse(message.toString());
    
    // Extraction des données utiles (à adapter selon votre format de données)
    const timestamp = new Date(data.received_at).getTime();
    
    // Supposons que nous avons un capteur qui envoie une valeur numérique
    // Adaptez cette partie en fonction de la structure de vos données
    let value = null;
    if (data.uplink_message && data.uplink_message.decoded_payload) {
      // Exemple: récupérer la température
      value = data.uplink_message.decoded_payload.temperature;
    }
    
    if (value !== null) {
      const dataPoint = {
        timestamp,
        value,
        deviceId: data.end_device_ids.device_id
      };
      
      // Ajouter les données et limiter la taille
      sensorData.push(dataPoint);
      if (sensorData.length > MAX_DATA_POINTS) {
        sensorData.shift(); // Supprimer le plus ancien
      }
      
      console.log(`Données reçues de ${dataPoint.deviceId}: ${dataPoint.value}`);
    }
  } catch (error) {
    console.error('Erreur lors du traitement du message:', error);
  }
});

// Configuration Express
app.use(cors()); // Activer CORS pour permettre les requêtes depuis le frontend
app.use(express.json());

// Endpoint pour récupérer les données
app.get('/api/data', (req, res) => {
  res.json(sensorData);
});

// Endpoint pour récupérer des statistiques sur les données
app.get('/api/stats', (req, res) => {
  if (sensorData.length === 0) {
    return res.json({
      count: 0,
      min: null,
      max: null,
      avg: null
    });
  }
  
  const values = sensorData.map(d => d.value);
  const stats = {
    count: sensorData.length,
    min: Math.min(...values),
    max: Math.max(...values),
    avg: values.reduce((sum, val) => sum + val, 0) / values.length
  };
  
  res.json(stats);
});

// Servir les fichiers statiques (frontend)
app.use(express.static('public'));

// Démarrer le serveur
app.listen(PORT, () => {
  console.log(`Serveur démarré sur http://localhost:${PORT}`);
});