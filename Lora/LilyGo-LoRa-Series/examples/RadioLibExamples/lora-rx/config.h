#pragma once

// =============================================
// CONFIGURATION WIFI
// =============================================

#define WIFI_SSID       "MonReseau"
#define WIFI_PASSWORD   "MonMotDePasse"

// =============================================
// CONFIGURATION GROQ API
// =============================================

#define GROQ_API_URL    "https://api.groq.com/openai/v1/chat/completions"
#define GROQ_API_KEY    "YOUR_GROQ_API_KEY"

#define GROQ_MODEL      "llama-3.1-8b-instant"
// Modeles disponibles:
// - llama-3.1-8b-instant (rapide, gratuit)
// - llama-3.2-1b-preview
// - llama-3.2-3b-preview
// - mixtral-8x7b-32768
// - gemma2-9b-it

#define SYSTEM_PROMPT   "Tu es un assistant IoT. Analyse la valeur du potentiometre et decide de l'action a prendre : allumer ou eteindre. Reponds uniquement par 'Allumer' ou 'Eteindre' selon la valeur."

// =============================================
// CONFIGURATION MQTT (Mosquitto Broker)
// =============================================

#define MQTT_BROKER     "192.168.1.100"
#define MQTT_PORT       1883
#define MQTT_TOPIC      "lora/result"
#define MQTT_USER       ""
#define MQTT_PASSWORD   ""

// =============================================
// CONFIGURATION LORA (automatique via LoRaBoards.h)
// =============================================
// Les parametres sont definis automatiquement selon la carte
