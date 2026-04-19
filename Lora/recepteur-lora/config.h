/*
 * config.h - Configuration du récepteur LoRa
 */

#pragma once

// =============================================
// CONFIGURATION WIFI
// =============================================

// Mettre a true pour WPA2 Entreprise, false pour WPA2 Personnel
#define USE_WPA2_ENTERPRISE  false

// --- WPA2 Personnel ---
const char* WIFI_SSID     = "1975 grandville";
const char* WIFI_PASSWORD = "grandville1975";

// --- WPA2 Entreprise (EAP-PEAP) ---
const char* EAP_IDENTITY  = "2442832";
const char* EAP_USERNAME  = "2442832";
const char* EAP_PASSWORD  = "060313D!@n&ryn";

// =============================================
// CONFIGURATION LLM
// =============================================

const char* OPENWEBUI_URL = "https://api.groq.com/openai/v1/chat/completions";
const char* API_KEY       = "REMOVED";
const char* MODEL_NAME    = "openai/gpt-oss-20b"; 

// System prompt - modifie ce texte !
const char* SYSTEM_PROMPT =
  "Tu es un contrôleur IoT. Tu reçois la valeur d'un potentiomètre (0-4095). "
  "Renvoie ABSOLUMENT UNIQUEMENT un objet JSON valide contenant deux clés: "
  "'action' ('on' si valeur > 2000, sinon 'off') et "
  "'msg' (un commentaire très court de la valeur, max 20 chars). "
  "Exemple exact: {\"action\":\"on\",\"msg\":\"Niveau OK\"}";