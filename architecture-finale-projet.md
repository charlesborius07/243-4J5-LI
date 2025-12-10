# Architecture Finale du Projet IoT
**Cours:** 243-4J5-LI – Objets connectés

---

## 🏗️ Architecture du Projet Final (Réaliste)

```mermaid
graph TB
    subgraph Zone_Deployment["🌍 Nœuds IoT Déployés"]
        subgraph Device1["📟 Nœud 1: LilyGO A7670G + PCB"]
            PCB["PCB Assemblé<br/>• Capteurs (temp, humidité)<br/>• LEDs (rouge/verte)<br/>• Boutons poussoirs"]

            A7670G["LilyGO A7670G<br/>• ESP32 + LTE<br/>• GPS intégré"]

            PCB <-->|GPIO/I2C| A7670G
        end

        subgraph Device2["📡 Nœud 2: T-Beam Distant"]
            TBeam["LilyGO T-Beam SUPREME<br/>• ESP32-S3 + LoRa<br/>• GPS intégré<br/>• WiFi activé"]

            SensorLora["Capteurs optionnels<br/>• Température<br/>• Position GPS"]

            TBeam <--> SensorLora
        end
    end

    subgraph Zone_Lab["🏠 Raspberry Pi 5 - Serveur"]
        Mosquitto["Mosquitto Broker<br/>• Port 1883 (local)<br/>• Port 9001 (WSS/TLS)"]

        CloudflareTunnel["Cloudflare Tunnel<br/>• Exposition sécurisée"]

        InterfaceTactile["Interface Tactile Python<br/>• Affichage données<br/>• Contrôle LEDs<br/>• Monitoring"]

        Mosquitto --> InterfaceTactile
    end

    subgraph Zone_Internet["☁️ Internet"]
        Internet["Réseau Public"]

        ClientDistant["Client Web Distant<br/>• Monitoring<br/>• Contrôle"]
    end

    %% Flux de communication
    A7670G -->|"MQTT via LTE<br/>sensors/temp<br/>actuators/led"| Mosquitto

    TBeam -->|"MQTT via WiFi<br/>meshtastic/position<br/>sensors/temp"| Mosquitto

    CloudflareTunnel <-->|Tunnel mTLS| Internet
    Mosquitto -->|WSS| CloudflareTunnel

    Internet <--> ClientDistant

    %% Styles
    classDef device fill:#fef3c7,stroke:#f59e0b,stroke-width:3px,color:#78350f
    classDef lora fill:#ecfeff,stroke:#06b6d4,stroke-width:3px,color:#164e63
    classDef infra fill:#e5e7eb,stroke:#4b5563,stroke-width:2px,color:#1f2937
    classDef cloud fill:#dbeafe,stroke:#3b82f6,stroke-width:2px,color:#1e3a8a

    class PCB,A7670G device
    class TBeam,SensorLora lora
    class Mosquitto,CloudflareTunnel,InterfaceTactile infra
    class Internet,ClientDistant cloud
```

---

## 📊 Flux de Données

### Nœud 1: LilyGO A7670G + PCB → Serveur

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

### Nœud 2: T-Beam Distant → Serveur

```mermaid
sequenceDiagram
    participant TB as T-Beam + GPS
    participant M as Mosquitto (Pi5)
    participant UI as Interface Tactile

    TB->>TB: Acquisition GPS
    Note over TB: Format JSON
    TB->>M: MQTT Publish (WiFi)<br/>meshtastic/position<br/>{"lat":46.8,"lon":-71.2}
    M->>UI: Affichage position
    Note over UI: Carte ou tableau
```

---

## 🔧 Composants du Projet Final

### Infrastructure (déjà en place)
- ✅ **Raspberry Pi 5** configuré
- ✅ **Mosquitto Broker** (local + WSS)
- ✅ **Cloudflare Tunnel** actif
- ✅ **Interface tactile Python** fonctionnelle

### Nœud 1: LilyGO A7670G + PCB
- ✅ **LilyGO A7670G** (Labos 1-2)
- 🔄 **PCB assemblé** (semaine 10)
- 🔄 **Capteurs** branchés sur PCB
- 🔄 **LEDs et boutons** fonctionnels
- 🔄 **Communication MQTT via LTE** opérationnelle

### Nœud 2: T-Beam Distant
- ✅ **T-Beam SUPREME** (semaines 7-9)
- ✅ **WiFi configuré**
- ✅ **MQTT activé**
- ✅ **GPS fonctionnel**
- 🔄 **Données envoyées** au serveur

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

**1. Nœud IoT complet (LilyGO A7670G + PCB):**
- PCB assemblé et soudé
- Capteurs fonctionnels (température, humidité, etc.)
- LEDs et boutons opérationnels
- Communication MQTT via LTE vers le serveur

**2. Infrastructure serveur (déjà en place):**
- Raspberry Pi 5 avec Mosquitto
- Interface tactile Python affichant les données
- Cloudflare Tunnel pour accès distant

**3. Démonstration LoRa (T-Beam):**
- T-Beam configuré et connecté en WiFi
- Envoi de données (position GPS) vers MQTT
- Intégration dans l'architecture globale

**4. Documentation:**
- Schéma du PCB
- Code source (Python, Arduino)
- Guide d'utilisation
- Tests et résultats

---

**Fin du document — Architecture Finale du Projet**
