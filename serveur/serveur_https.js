// server-http.js
// Serveur Node.js pour récupérer les données TTI via HTTP API et les exposer en REST
const express = require('express');
const cors = require('cors');
const axios = require('axios');
const https = require('https');
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
  region: 'eu2',
  applicationId: 'appli-captage',
  apiKey: 'NNSXS.FWBI4YL6EISYPDI4J4HLUH6LRHOZ666COCVDS5I.AFUABQI76OFUMN5QX3YNOTEENAJYB3DO3VKQLWF4BVJU5NOEL2WQ'
};

// Stockage des données reçues
let sensorData = [];
const MAX_DATA_POINTS = 100;

// Créer un agent HTTPS avec le proxy
const httpsAgent = new HttpsProxyAgent(`http://${PROXY_CONFIG.host}:${PROXY_CONFIG.port}`);

// Client HTTP avec configuration proxy
const httpClient = axios.create({
  baseURL: `https://${TTI_CONFIG.region}.cloud.thethings.industries/api/v3`,
  headers: {
    'Authorization': `Bearer ${TTI_CONFIG.apiKey}`,
    'Accept': 'application/json'
  },
  httpsAgent,
  proxy: false // Désactiver le proxy global d'axios car nous utilisons HttpsProxyAgent
});

// Fonction pour récupérer les données récentes
async function fetchRecentData() {
  try {
    console.log('Récupération des données récentes...');
    const response = await httpClient.get(
      `/as/applications/${TTI_CONFIG.applicationId}/packages/storage/uplink_message`
    );

    if (response.data && Array.isArray(response.data.result)) {
      console.log(`${response.data.result.length} messages récupérés`);
      
      response.data.result.forEach(message => {
        // Adaptez cette partie en fonction du format de vos données
        if (message.decoded_payload && message.decoded_payload.temperature !== undefined) {
          const dataPoint = {
            timestamp: new Date(message.received_at).getTime(),
            value: message.decoded_payload.temperature,
            deviceId: message.end_device_ids.device_id
          };
          
          // Ajouter uniquement si pas déjà dans sensorData (vérifier par timestamp)
          if (!sensorData.some(d => d.timestamp === dataPoint.timestamp)) {
            sensorData.push(dataPoint);
            console.log(`Données récupérées de ${dataPoint.deviceId}: ${dataPoint.value}`);
          }
        }
      });
      
      // Trier par timestamp et limiter la taille
      sensorData.sort((a, b) => b.timestamp - a.timestamp);
      if (sensorData.length > MAX_DATA_POINTS) {
        sensorData = sensorData.slice(0, MAX_DATA_POINTS);
      }
    }
  } catch (error) {
    console.error('Erreur lors de la récupération des données:', error.message);
    if (error.response) {
      console.error('Réponse d\'erreur:', error.response.status, error.response.data);
    }
  }
}

// Configuration Express
app.use(cors());
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

// Endpoint pour déclencher manuellement une récupération des données
app.get('/api/refresh', async (req, res) => {
  try {
    await fetchRecentData();
	console.log('Réponse TTI brute :', JSON.stringify(response.data, null, 2));

    res.json({ success: true, count: sensorData.length });
  } catch (error) {
    res.status(500).json({ success: false, error: error.message });
  }
});

// Servir les fichiers statiques (frontend)
app.use(express.static('public'));

// Récupérer les données initialement et configurer une récupération périodique
fetchRecentData().then(() => {
  console.log('Données initiales récupérées');
  
  // Récupérer de nouvelles données toutes les minutes
  setInterval(fetchRecentData, 60000);
  
  // Trier par timestamp et limiter la taille
sensorData.sort((a, b) => b.timestamp - a.timestamp);
if (sensorData.length > MAX_DATA_POINTS) {
  sensorData = sensorData.slice(0, MAX_DATA_POINTS);
}

// ✅ Affichage dans le terminal
console.log('--- Données actuelles des capteurs ---');
console.table(sensorData);


});

// Démarrer le serveur
app.listen(PORT, () => {
  console.log(`Serveur démarré sur http://localhost:${PORT}`);
  console.log(`Endpoints disponibles:`);
  console.log(`- http://localhost:${PORT}/api/data - Données des capteurs`);
  console.log(`- http://localhost:${PORT}/api/stats - Statistiques des données`);
  console.log(`- http://localhost:${PORT}/api/refresh - Rafraîchir les données`);
});