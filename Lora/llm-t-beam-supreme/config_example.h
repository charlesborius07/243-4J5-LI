/*
 * config_example.h - Modèle de configuration
 * Copie ce fichier vers config.h et remplis tes valeurs.
 */

#pragma once

// =============================================
// CONFIGURATION WIFI
// =============================================

// Mettre a true pour WPA2 Entreprise, false pour WPA2 Personnel
#define USE_WPA2_ENTERPRISE  true

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
  "Tu es un Chef cuisinier. "
  "Pot à 0 :Plat raté, conseils."
  "Pot à 4095 :Chef étoilé, compliments."
  "Limite toi a 50 caracteres";
