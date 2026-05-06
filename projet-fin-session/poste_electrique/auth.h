// auth.h - Configuration WiFi et MQTT
#ifndef AUTH_H
#define AUTH_H

// ============================================================================
// CONFIGURATION WiFi - CHOISIR UN SEUL TYPE DE SÉCURITÉ
// ============================================================================

// --- Option1: WPA2-Personal (réseau domestique) ---
// Décommentez la ligne suivante pour un réseau WiFi avec mot de passe simple
//#define WIFI_SECURITY_WPA2_PERSONAL

// --- Option2: WPA2-Enterprise (réseau du Cégep, entreprise) ---
// Décommentez la ligne suivante pour un réseau avec authentification par identifiant
 #define WIFI_SECURITY_WPA2_ENTERPRISE

// ============================================================================
// CONFIGURATION WiFi - WPA2-Personal
// ============================================================================
// À utiliser avec WIFI_SECURITY_WPA2_PERSONAL

const char* WIFI_SSID = "climoilou";          // Nom du réseau WiFi
//const char* WIFI_PASSWORD = "votre_mot_de_passe";      // Mot de passe WiFi

// ============================================================================
// CONFIGURATION WiFi - WPA2-Enterprise (EAP-PEAP MSCHAPv2)
// ============================================================================
// À utiliser avec WIFI_SECURITY_WPA2_ENTERPRISE
// Pour les réseaux d'entreprise ou du Cégep

const char* EAP_IDENTITY = "2442832";     // Identité externe
const char* EAP_USERNAME = "2442832";     // Nom d'utilisateur
const char* EAP_PASSWORD = "060313D!@n&ryn";    // Mot de passe

// ============================================================================
// CONFIGURATION MQTT
// ============================================================================

const char* MQTT_BROKER = "mqtt.charlesborius07.com";           // Adresse du broker MQTT
const char* MQTT_USER = "telecom";                 // Utilisateur MQTT
const char* MQTT_PASS = "Teladmin1$";       // Mot de passe MQTT

// Device ID - Généré automatiquement à partir de l'adresse MAC
const char* MQTT_CLIENT_ID = "poste-01";          // Identifiant unique du poste

#endif // AUTH_H
