/*
 * config.example.h - Modele de configuration
 * Copie ce fichier vers config.h et remplis tes valeurs.
 */

#pragma once

// =============================================
// CONFIGURATION WIFI
// =============================================

// Mettre a true pour WPA2 Entreprise, false pour WPA2 Personnel
//#define USE_WPA2_ENTERPRISE  true
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

//const char* OPENWEBUI_URL = "https://chat.ve2fpd.com/api/chat/completions";
const char* OPENWEBUI_URL = "https://api.groq.com/openai/v1/chat/completions";
//const char* API_KEY       = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpZCI6ImUwOWZhNjRhLTdhMzctNDRhNi05NWU4LTAxMzY0MWFjNDhkNiIsImV4cCI6MTc3Njk5NDg5MSwianRpIjoiODJhNTE1MWQtOWM3ZC00N2E4LWJmZDEtYzNjNzA0MWU5YzlhIn0.ig3_rGHIos4znA2_M27_x0Jf1KgeAKJoWL4k6XjfmuA";
const char* API_KEY       = "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";
//const char* MODEL_NAME    = "assistant-iot-v2";
const char* MODEL_NAME    = "openai/gpt-oss-20b";

// System prompt - modifie ce texte !
const char* SYSTEM_PROMPT =
  "Tu es un Chef cuisinier. "
  "Pot à 0 :Plat raté, conseils."
  "Pot à 4095 :Chef étoilé, compliments."
  "Limite toi a 50 caracteres";
