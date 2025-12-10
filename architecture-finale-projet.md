# Architecture Finale du Projet IoT
**Cours:** 243-4J5-LI – Objets connectés

---

## 🏗️ Architecture du Projet Final (Réaliste)

```mermaid
graph TB
    subgraph Device_LTE["📟 Nœud 1: LilyGO A7670G + PCB"]
        PCB["PCB Assemblé<br/>• Capteurs (temp, humidité)<br/>• LEDs (rouge/verte)<br/>• Boutons poussoirs"]

        A7670G["LilyGO A7670G<br/>• ESP32 + LTE Cat-1<br/>• GPS intégré"]

        PCB <-->|GPIO/I2C| A7670G
    end

    subgraph Device_LoRa["📡 Nœud 2: T-Beam Distant"]
        TBeam_Distant["T-Beam SUPREME<br/>• ESP32-S3 + LoRa<br/>• GPS intégré<br/>• Batterie/Mobile"]
    end

    TBeam_Local["🔄 T-Beam Local (Gateway)<br/>• ESP32-S3 + LoRa<br/>• WiFi (réseau local)<br/>• Pont LoRa → MQTT"]

    subgraph RaspberryPi["🍓 Raspberry Pi 5"]
        Mosquitto["Mosquitto Broker<br/>• Port 1883 (local)<br/>• Port 9001 (WSS/TLS)"]

        InterfaceTactile["Interface Tactile Python<br/>• Affichage données<br/>• Contrôle LEDs"]

        Mosquitto --> InterfaceTactile
        Mosquitto --> CloudflareTunnel
    end

    CloudflareTunnel(["☁️ Cloudflare Tunnel<br/>• Exposition sécurisée<br/>• WSS port 443<br/>• domaine.example.com"])

    Internet(["☁️ Internet / LTE<br/>Réseau cellulaire"])

    ClientDistant["💻 Client Web Distant<br/>• Dashboard<br/>• Monitoring"]

    %% Flux de communication

    %% Nœud 1 passe par Internet/Cloudflare
    A7670G -->|"MQTT via LTE"| Internet
    Internet -->|"WSS:443<br/>TLS/mTLS"| CloudflareTunnel
    CloudflareTunnel -->|"MQTT<br/>sensors/*<br/>actuators/*"| Mosquitto

    %% Nœud 2 via LoRa mesh
    TBeam_Distant <-->|"LoRa mesh<br/>Longue portée"| TBeam_Local

    %% Gateway local
    TBeam_Local -->|"MQTT via WiFi local<br/>meshtastic/position"| Mosquitto

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

    class PCB,A7670G lte
    class TBeam_Distant lora_remote
    class TBeam_Local lora_local
    class Mosquitto,InterfaceTactile infra
    class CloudflareTunnel,Internet cloud
    class ClientDistant client
```

---

## 📊 Flux de Données

### Flux 1: LilyGO A7670G + PCB → Serveur (via LTE)

```mermaid
sequenceDiagram
    participant PCB as Capteurs PCB
    participant A7670G as LilyGO A7670G
    participant M as Mosquitto (Pi5)
    participant UI as Interface Tactile

    PCB->>A7670G: Lecture GPIO<br/>(température, boutons)
    Note over A7670G: Format JSON
    A7670G->>M: MQTT Publish (LTE)<br/>sensors/temp<br/>{"value":22.5}
    M->>UI: Affichage temps réel
    Note over UI: Mise à jour écran tactile
```

### Flux 2: T-Beam Distant → T-Beam Local → Serveur (via LoRa mesh + WiFi)

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

### Nœud 1: LilyGO A7670G + PCB (Communication LTE)
- ✅ **LilyGO A7670G** (Labos 1-2)
- 🔄 **PCB assemblé et soudé** (semaine 10)
- 🔄 **Capteurs** branchés sur PCB (température, humidité)
- 🔄 **LEDs et boutons** fonctionnels
- 🔄 **Communication MQTT via LTE** opérationnelle

### Nœud 2: T-Beam Local (Gateway LoRa → WiFi)
- ✅ **T-Beam SUPREME #1** (semaines 7-9)
- ✅ **WiFi configuré** (réseau local du labo)
- ✅ **LoRa activé** (réception mesh)
- ✅ **MQTT activé** (envoi vers Mosquitto)
- 🔄 **Rôle gateway** LoRa → MQTT fonctionnel

### Nœud 3: T-Beam Distant (Mobile LoRa)
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
├── sensors/               # Nœud A7670G + PCB
│   ├── temperature        # {"value": 22.5, "unit": "C"}
│   ├── humidity           # {"value": 65, "unit": "%"}
│   └── gps                # {"lat": 46.8, "lon": -71.2}
│
├── actuators/             # Contrôle des LEDs
│   ├── led/red            # {"state": "on" | "off"}
│   └── led/green          # {"state": "on" | "off"}
│
└── meshtastic/            # Nœud T-Beam distant
    └── position           # {"lat": 46.8, "lon": -71.2, "alt": 100}
```

---


## ✅ Résumé du Projet Final

### Ce que chaque étudiant doit livrer:

**1. Infrastructure serveur (déjà en place depuis Labos 1-2):**
- Raspberry Pi 5 avec Mosquitto Broker
- Interface tactile Python affichant les données
- Cloudflare Tunnel pour accès distant sécurisé

**2. Nœud IoT LTE (LilyGO A7670G + PCB):**
- PCB assemblé et soudé (semaine 10)
- Capteurs fonctionnels branchés au PCB
- LEDs et boutons opérationnels
- Communication MQTT via LTE vers le serveur

**3. Système LoRa mesh (2 T-Beam):**
- **T-Beam local:** Gateway LoRa → MQTT (WiFi réseau local)
- **T-Beam distant:** Nœud mobile avec GPS (communication LoRa)
- Communication mesh LoRa fonctionnelle entre les 2 nœuds
- Données GPS du nœud distant acheminées au serveur

**4. Documentation complète:**
- Schéma du PCB (Altium)
- Code source (Python, Arduino/ESP32)
- Cartographie de couverture LoRa (GPX)
- Guide d'utilisation
- Résultats de tests (RSSI, SNR, portée)

---

**Fin du document — Architecture Finale du Projet**
