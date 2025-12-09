# LilyGo T-SIM A7670G - Contrôle MQTT via LTE/Cellulaire

Ce code permet de contrôler les LEDs d'un ESP32 LilyGo T-SIM A7670G via MQTT en utilisant une connexion cellulaire (LTE).

## 📱 Matériel requis

- **LilyGo T-SIM A7670G** (avec modem cellulaire A7670G)
- **Carte SIM** avec forfait de données actif
- Antenne LTE connectée au module

## 🔧 Configuration

### 1. Installation des bibliothèques Arduino

Installez les bibliothèques suivantes via le gestionnaire de bibliothèques Arduino:

```
- TinyGSM (by Volodymyr Shymanskyy) - Version 0.11.5 ou supérieure
- PubSubClient (by Nick O'Leary) - Version 2.8 ou supérieure
```

### 2. Configuration de la carte Arduino

Dans Arduino IDE:
1. **Fichier → Préférences → URLs de gestionnaire de cartes supplémentaires**
2. Ajoutez: `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
3. **Outils → Type de carte → Gestionnaire de cartes**
4. Installez: **esp32 by Espressif Systems**
5. **Outils → Type de carte → ESP32 Arduino**
6. Sélectionnez: **ESP32 Dev Module**

### 3. Configuration de l'APN (auth.h)

Éditez le fichier `auth.h` et configurez l'APN de votre opérateur cellulaire:

```cpp
const char APN[] = "internet.com";      // ⚠️ À remplacer par votre APN
const char APN_USER[] = "";             // Généralement vide
const char APN_PASS[] = "";             // Généralement vide
```

**Exemples d'APN par opérateur au Canada:**
- **Bell**: `"inet.bell.ca"` ou `"pda.bell.ca"`
- **Rogers**: `"internet.com"` ou `"ltemobile.apn"`
- **Telus**: `"sp.telus.com"` ou `"isp.telus.com"`
- **Fido**: `"internet.fido.ca"`
- **Koodo**: `"sp.koodo.com"`
- **Virgin**: `"media.bell.ca"`
- **Videotron**: `"media.videotron"`

### 4. Vérification de la configuration MQTT (auth.h)

Le broker MQTT est préconfiguré pour `mqtt.edxo.ca`:

```cpp
const char MQTT_BROKER[] = "mqtt.edxo.ca";
const int  MQTT_PORT = 1883;  // Port standard MQTT
const char MQTT_USER[] = "esp_user";
const char MQTT_PASS[] = "yxhtfi60";
```

## 📤 Téléversement du code

1. Connectez le LilyGo via USB
2. Sélectionnez le port série: **Outils → Port**
3. Cliquez sur **Téléverser**

## 🚀 Utilisation

### Démarrage

1. Insérez une carte SIM avec forfait de données actif
2. Connectez l'antenne LTE au module A7670G
3. Alimentez le LilyGo (USB ou batterie)

### Séquence de démarrage

Le modem effectue la séquence suivante:

1. **Initialisation du modem** (~3 secondes)
2. **Recherche de réseau cellulaire** (jusqu'à 60 secondes)
3. **Connexion GPRS/LTE** avec l'APN configuré
4. **Connexion au broker MQTT**

### Moniteur série

Ouvrez le moniteur série à **115200 bauds** pour voir les logs:

```
=== LilyGo T-SIM A7670G - MQTT via LTE ===

[MODEM] Démarrage du modem...
[MODEM] Modem allumé
[MODEM] Initialisation...
[MODEM] Fabricant: SIMCOM INCORPORATED
[MODEM] Modèle: SIMCOM_A7670G
[MODEM] IMEI: 123456789012345

[NETWORK] Connexion au réseau cellulaire...
[NETWORK] Opérateur: Rogers
[NETWORK] Signal: -67 dBm

[GPRS] Connexion GPRS...
[GPRS] IP: 10.123.45.67
[GPRS] ✓ Connecté

[MQTT] Device ID: lte-012345
[MQTT] Connexion au broker: mqtt.edxo.ca:1883
[MQTT] ✓ Connecté

=== Système prêt ===
```

### Device ID

Le Device ID est généré automatiquement à partir de l'**IMEI** de la carte SIM:

```
Format: lte-XXXXXX (6 derniers chiffres de l'IMEI)
Exemple: lte-012345
```

Notez ce Device ID pour l'utiliser dans l'interface Python de contrôle.

## 📡 Topics MQTT

Les topics sont générés automatiquement à partir du Device ID:

### Publications (envoi par l'ESP32):
- `{device_id}/button/1/state` → "PRESSED" ou "RELEASED"
- `{device_id}/button/2/state` → "PRESSED" ou "RELEASED"

### Souscriptions (réception par l'ESP32):
- `{device_id}/led/1/set` → "ON" ou "OFF"
- `{device_id}/led/2/set` → "ON" ou "OFF"

**Exemple avec Device ID `lte-012345`:**
- Publier: `lte-012345/button/1/state`
- Recevoir: `lte-012345/led/1/set`

## 🔍 Dépannage

### Le modem ne démarre pas

1. Vérifiez que l'antenne LTE est bien connectée
2. Vérifiez l'alimentation (USB ou batterie chargée)
3. Essayez de redémarrer (débrancher/rebrancher)

### Pas de connexion réseau

1. **Vérifiez la carte SIM**:
   - Forfait de données actif
   - Code PIN désactivé
   - Carte correctement insérée

2. **Vérifiez la couverture réseau**:
   - Signal cellulaire disponible
   - Bande de fréquence compatible

3. **Vérifiez l'APN** dans `auth.h`:
   - APN correct pour votre opérateur
   - Username/password si requis

### La connexion MQTT échoue

1. Vérifiez que le GPRS est connecté (IP assignée)
2. Vérifiez les identifiants MQTT dans `auth.h`
3. Testez la connexion au broker:
   ```bash
   ping mqtt.edxo.ca
   ```

### Connexion instable / déconnexions fréquentes

Le code vérifie automatiquement la connexion GPRS toutes les 30 secondes et reconnecte si nécessaire. Si les déconnexions sont trop fréquentes:

1. Vérifiez la force du signal (devrait être > -100 dBm)
2. Essayez de changer de position/orientation de l'antenne
3. Vérifiez que votre forfait de données n'est pas épuisé

### LED rouge clignote (erreur)

Les clignotements indiquent une erreur:
- **1 clignotement**: Erreur d'initialisation modem
- **2 clignotements**: Erreur de connexion réseau
- **3 clignotements**: Erreur de connexion GPRS
- **4 clignotements**: Erreur de connexion MQTT

## 🔄 Différences avec la version WiFi

| Caractéristique | WiFi (MSCHAPv2) | LTE (A7670G) |
|----------------|-----------------|--------------|
| **Bibliothèque** | WiFi.h + esp_wpa2 | TinyGSM |
| **Connexion** | WiFi WPA2-Enterprise | GPRS/LTE via APN |
| **Device ID** | `esp32-` + MAC | `lte-` + IMEI |
| **Port MQTT** | 443 (WSS) | 1883 (standard) |
| **Démarrage** | ~5 secondes | ~30-60 secondes |
| **Mobilité** | Limitée au WiFi | Mobile (couverture cellulaire) |
| **Consommation** | Faible | Moyenne à élevée |

## 📊 Consommation de données

Estimation pour une journée d'utilisation typique:
- Connexion MQTT keepalive: ~10 KB/jour
- 100 messages LED: ~5 KB/jour
- Total estimé: **< 100 KB/jour**

Un forfait de données de 1 GB suffit amplement pour plusieurs mois d'utilisation.

## 🔒 Sécurité

- **Authentification MQTT**: Username/Password
- **Connexion réseau**: Sécurisée par l'opérateur cellulaire (chiffrement LTE)
- Port standard MQTT (1883) - Pas de TLS sur modem A7670G pour économiser la mémoire

**Note**: Pour une sécurité maximale, utilisez la version WiFi avec WSS (port 443).

## 🛠️ Personnalisation

### Modifier les topics MQTT

Éditez la section MQTT dans `lilygo_lte_mqtt.ino`:

```cpp
snprintf(topic_led1_set, sizeof(topic_led1_set),
         "%s/led/1/set", MQTT_CLIENT_ID);
```

### Changer le comportement des boutons

Modifiez la fonction `checkButtons()`:

```cpp
void checkButtons() {
    bool btn1 = (digitalRead(BUTTON_1) == LOW);
    // Ajoutez votre code ici
}
```

## 📚 Ressources

- [TinyGSM GitHub](https://github.com/vshymanskyy/TinyGSM)
- [LilyGo T-SIM A7670G Documentation](https://github.com/Xinyuan-LilyGO/LilyGO-T-SIM7670G)
- [Datasheet A7670G](https://www.simcom.com/product/A7670X.html)
- [PubSubClient Documentation](https://pubsubclient.knolleary.net/)

## 📝 Interface de contrôle Python

Pour contrôler ce module LilyGo depuis une interface tactile Raspberry Pi, utilisez le code Python dans `/labo2/led-control/`.

**IMPORTANT**: Mettez à jour le `device_id` dans `mqtt_config.py` avec le Device ID LTE:

```python
"device_id": "lte-012345",  # Device ID affiché dans le moniteur série
```
