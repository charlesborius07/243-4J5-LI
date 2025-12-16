# Architecture Finale du Projet IoT
**Cours:** 243-4J5-LI – Objets connectés

---

## 🏗️ Architecture du Projet Final (Réaliste)

```mermaid
graph TB
    subgraph Device_LTE["📟 LilyGO A7670G + PCB"]
        PCB["PCB Assemblé<br/>• LEDs (1-4)<br/>• Boutons (1-3)<br/>• Accéléromètre"]

        A7670G["LilyGO A7670G<br/>• ESP32 + LTE Cat-1<br/>• GPS intégré<br/>• Config: WSS:443"]

        PCB <-->|GPIO/I2C| A7670G
    end

    TBeam_Distant["📡 T-Beam Distant<br/>• ESP32-S3 + LoRa<br/>• GPS intégré<br/>• Batterie/Mobile"]

    subgraph Reseau_Local["🏠 Réseau Local du Laboratoire"]
        TBeam_Local["🔄 T-Beam Local (Gateway)<br/>• ESP32-S3 + LoRa<br/>• WiFi (réseau local)<br/>• Pont LoRa → MQTT"]

        subgraph RaspberryPi["🍓 Raspberry Pi 5"]
            Mosquitto["Mosquitto Broker<br/>• Port 1883 (local)<br/>• Port 9001 (WSS/TLS)"]

            InterfaceTactile["Interface Tactile Python<br/>• Affichage données<br/>• Contrôle LEDs"]

            Mosquitto --> InterfaceTactile
        end
    end

    CloudflareTunnel(["☁️ Cloudflare Tunnel<br/>• Exposition sécurisée<br/>• WSS port 443<br/>• domaine.example.com"])

    Internet(["☁️ Internet / LTE<br/>Réseau cellulaire<br/>+ Cloudflare CDN"])

    ClientDistant["💻 Client Web Distant<br/>• Dashboard<br/>• Monitoring"]

    %% Flux de communication

    %% A7670G se connecte en WSS via Cloudflare sur Internet
    A7670G -->|"MQTT over WSS<br/>Port 443<br/>wss://domain.example.com"| Internet
    Internet -->|"Via Cloudflare CDN"| CloudflareTunnel
    CloudflareTunnel -->|"MQTT local<br/>sensors/*<br/>actuators/*"| Mosquitto

    %% LoRa mesh
    TBeam_Distant <-->|"LoRa mesh<br/>Longue portée"| TBeam_Local

    %% Gateway local et exposition via tunnel
    TBeam_Local -->|"MQTT via WiFi local<br/>meshtastic/position"| Mosquitto
    Mosquitto -->|"Port 9001 WSS/TLS"| CloudflareTunnel

    %% Accès client distant
    ClientDistant -->|"HTTPS/WSS"| Internet
    Internet -->|"WSS:443"| CloudflareTunnel

    %% Styles
    classDef lte fill:#fef3c7,stroke:#f59e0b,stroke-width:3px,color:#78350f
    classDef lora_remote fill:#ecfeff,stroke:#06b6d4,stroke-width:3px,color:#164e63
    classDef lora_local fill:#fae8ff,stroke:#a855f7,stroke-width:3px,color:#581c87
    classDef infra fill:#e5e7eb,stroke:#4b5563,stroke-width:2px,color:#1f2937
    classDef cloud fill:#e6f3ff,stroke:#3b82f6,stroke-width:2px,stroke-dasharray:5 5,color:#1e3a8a
    classDef client fill:#f3e8ff,stroke:#9333ea,stroke-width:2px,color:#4c1d95
    classDef local_net fill:#f0fdf4,stroke:#22c55e,stroke-width:2px,color:#14532d

    class PCB,A7670G lte
    class TBeam_Distant lora_remote
    class TBeam_Local lora_local
    class Mosquitto,InterfaceTactile infra
    class CloudflareTunnel,Internet cloud
    class ClientDistant client
    class Reseau_Local local_net
```

---

## 📊 Flux de Données

### Flux LTE: LilyGO A7670G + PCB → Serveur

```mermaid
sequenceDiagram
    participant PCB as Capteurs PCB
    participant A7670G as LilyGO A7670G
    participant LTE as Internet/LTE
    participant CF as Cloudflare<br/>(CDN + Tunnel)
    participant M as Mosquitto (Pi5)
    participant UI as Interface Tactile

    PCB->>A7670G: Lecture GPIO/I2C<br/>(boutons, accéléromètre)
    Note over A7670G: Format JSON
    A7670G->>LTE: MQTT over WSS:443<br/>wss://domain.example.com
    LTE->>CF: Via Cloudflare CDN
    CF->>M: Tunnel → Port 9001<br/>sensors/accel {"x":0.1,"y":0.2}
    M->>UI: Affichage temps réel
    Note over UI: Mise à jour écran tactile
```

### Flux LoRa: T-Beam Distant → T-Beam Local → Serveur

```mermaid
sequenceDiagram
    participant TBD as T-Beam Distant<br/>(LoRa)
    participant TBL as T-Beam Local<br/>(Gateway WiFi)
    participant M as Mosquitto (Pi5)
    participant UI as Interface Tactile

    TBD->>TBD: Acquisition GPS
    Note over TBD: Format Meshtastic
    TBD->>TBL: Message LoRa<br/>Position GPS
    Note over TBL: Conversion<br/>LoRa → MQTT
    TBL->>M: MQTT Publish (WiFi)<br/>meshtastic/position<br/>{"lat":46.8,"lon":-71.2}
    M->>UI: Affichage position
    Note over UI: Carte ou liste
```

---

## 🔧 Composants du Projet Final

### Infrastructure (déjà en place)
- ✅ **Raspberry Pi 5** configuré (Labos 1-2)
- ✅ **Mosquitto Broker** (local + WSS)
- ✅ **Cloudflare Tunnel** actif
- ✅ **Interface tactile Python** fonctionnelle

### LilyGO A7670G + PCB (Communication LTE)
- ✅ **LilyGO A7670G** (Labos 1-2)
- 🔄 **PCB assemblé et soudé** (semaine 10)
- 🔄 **LEDs** fonctionnelles (selon assignation: 1-4)
- 🔄 **Boutons** fonctionnels (selon assignation: 1-3)
- 🔄 **Accéléromètre** (MPU6050/ADXL345) via I2C
- 🔄 **Communication MQTT via LTE** opérationnelle

### T-Beam Local (Gateway LoRa → WiFi)
- ✅ **T-Beam SUPREME #1** (semaines 7-9)
- ✅ **WiFi configuré** (réseau local du labo)
- ✅ **LoRa activé** (réception mesh)
- ✅ **MQTT activé** (envoi vers Mosquitto)
- 🔄 **Rôle gateway** LoRa → MQTT fonctionnel

### T-Beam Distant (Mobile LoRa)
- ✅ **T-Beam SUPREME #2** (semaines 7-9)
- ✅ **LoRa configuré** (transmission mesh)
- ✅ **GPS fonctionnel**
- 🔄 **Envoi position GPS** via LoRa vers T-Beam local
- 🔄 **Tests terrain** complétés

---

## 📡 Topics MQTT

### Structure simple des topics:

```
mqtt://
├── etudiant/{prenom-nom}/    # Nœud A7670G + PCB
│   ├── sensors/
│   │   ├── buttons           # {"btn1": true, "btn2": false, ...}
│   │   └── accel             # {"x": 0.12, "y": -0.05, "z": 9.81}
│   ├── actuators/
│   │   ├── led1              # {"state": "on" | "off"}
│   │   └── led2              # {"state": "on" | "off"} ...
│   └── status                # {"uptime": 3600, "rssi": -65}
│
└── meshtastic/               # Nœud T-Beam distant
    └── position              # {"lat": 46.8, "lon": -71.2, "alt": 100}
```

---


## ✅ Résumé du Projet Final

### Ce que chaque étudiant doit livrer:

**1. Infrastructure serveur (déjà en place depuis Labos 1-2):**
- Raspberry Pi 5 avec Mosquitto Broker
- Interface tactile Python affichant les données
- Cloudflare Tunnel pour accès distant sécurisé

**2. Module IoT LTE (LilyGO A7670G + PCB):**
- PCB assemblé et soudé (semaine 10)
- LEDs opérationnelles (selon assignation: 1-4)
- Boutons opérationnels (selon assignation: 1-3)
- Accéléromètre (MPU6050/ADXL345) fonctionnel
- Communication MQTT via LTE vers le serveur

**3. Système LoRa mesh (2 T-Beam SUPREME):**
- **T-Beam local:** Gateway LoRa → MQTT (WiFi réseau local)
- **T-Beam distant:** Module mobile avec GPS (communication LoRa)
- Communication mesh LoRa fonctionnelle entre les deux T-Beam
- Données GPS du T-Beam distant acheminées au serveur

**4. Documentation complète:**
- Schéma du PCB (KiCad)
- Code source (Python, Arduino/ESP32)
- Cartographie de couverture LoRa (GPX)
- Guide d'utilisation
- Résultats de tests (RSSI, SNR, portée)

---

**Fin du document — Architecture Finale du Projet**
