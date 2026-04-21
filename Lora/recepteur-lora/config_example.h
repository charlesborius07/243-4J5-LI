/*
 * config_example.h - Modèle de configuration pour le récepteur LoRa
 * Copiez ce fichier vers config.h et remplissez vos propres valeurs.
 */

#pragma once

// =============================================
// CONFIGURATION WIFI
// =============================================

// Mettre à true pour WPA2 Entreprise, false pour WPA2 Personnel
#define USE_WPA2_ENTERPRISE  false

// --- WPA2 Personnel ---
const char* WIFI_SSID     = "VOTRE_SSID";
const char* WIFI_PASSWORD = "VOTRE_MOT_DE_PASSE";

// --- WPA2 Entreprise (EAP-PEAP) ---
const char* EAP_IDENTITY  = "VOTRE_ID_CÉGEP";
const char* EAP_USERNAME  = "VOTRE_USER_CÉGEP";
const char* EAP_PASSWORD  = "VOTRE_PASS_CÉGEP";

// =============================================
// CONFIGURATION LLM
// =============================================

const char* OPENWEBUI_URL = "https://api.groq.com/openai/v1/chat/completions";
const char* API_KEY       = "VOTRE_CLE_API_GROQ";
const char* MODEL_NAME    = "openai/gpt-oss-20b"; 

// System prompt - modifie ce texte !
const char* SYSTEM_PROMPT =
  "Tu es un contrôleur IoT. Tu reçois la valeur d'un potentiomètre (0-4095). "
  "Renvoie ABSOLUMENT UNIQUEMENT un objet JSON valide contenant deux clés: "
  "'action' ('on' si valeur > 2000, sinon 'off') et "
  "'msg' (un commentaire très court de la valeur, max 20 chars). "
  "Exemple exact: {\"action\":\"on\",\"msg\":\"Niveau OK\"}";

// =============================================
// CONFIGURATION MQTT
// =============================================

const char* MQTT_BROKER = "mqtt.charlesborius07.com";
const int   MQTT_PORT   = 443;
const char* MQTT_PATH   = "/";
const char* MQTT_USER   = "VOTRE_USER_MQTT";
const char* MQTT_PASS   = "VOTRE_PASS_MQTT";
const char* MQTT_CLIENT_ID = "esp32-lora-receiver";

const char* TOPIC_PUB_DECISION = "etudiant/VOTRE_NOM/lora/decision";
