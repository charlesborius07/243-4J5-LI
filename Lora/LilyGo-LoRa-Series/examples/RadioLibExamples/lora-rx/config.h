#pragma once

// =============================================
// CONFIGURATION WIFI
// =============================================

#define WIFI_SSID       "1975 grandville"
#define WIFI_PASSWORD   "grandville1975"

// =============================================
// CONFIGURATION GROQ API
// =============================================

#define GROQ_API_URL    "https://api.groq.com/openai/v1/chat/completions"
#define GROQ_API_KEY    "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"

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

#define MQTT_BROKER     "mqtt.charlesborius07.com"
#define MQTT_PORT       443
#define MQTT_TOPIC      "lora/result"
#define MQTT_USER       "telecom"
#define MQTT_PASSWORD   "Teladmin1$"

// =============================================
// CONFIGURATION LORA (automatique via LoRaBoards.h)
// =============================================
// Les parametres sont definis automatiquement selon la carte
