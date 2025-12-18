# Plans de séquence hebdomadaires

**Cours:** 243-4J5-LI – Objets connectés
**Session:** H26
**Pondération:** 2-3-2 (2h théorie, 3h labo, 2h travail personnel)

---

## Vue d'ensemble des phases

| Phase | Semaines | Thème principal | Évaluation sommative |
|-------|----------|-----------------|---------------------|
| **1** | 1-3 | Infrastructure et communications | Labo capteurs et Python (15%) |
| **2** | 4-7 | Conception PCB avec KiCad | Projet mi-session Shield PCB (20%) |
| **3** | 7-9 | Réseau mesh LoRa/Meshtastic | TP Intégration LLM (20%) |
| **4** | 10-12 | Assemblage PCB et automatisation LLM | Formatif |
| **5** | 13-15 | Projet final et déploiement | Projet final IdO (30%) |

---

# PHASE 1 — Infrastructure et communications (Semaines 1-3)

---

## Semaine 1 — Introduction à l'IdO et architecture

### Objectifs d'apprentissage
À la fin de cette semaine, l'étudiant sera capable de:
- Décrire l'architecture d'un système IdO complet
- Configurer un Raspberry Pi 5 comme serveur de développement distant
- Installer et configurer un courtier MQTT Mosquitto
- Exposer des services de façon sécurisée via Cloudflare Tunnel
- Prendre en main la carte LilyGO A7670G

### Prérequis
- Connaissances de base en Linux (lignes de commande)
- Notions de programmation (variables, boucles, conditions)
- Compte GitHub actif

### Contenus théoriques (2h)

#### 1.1 Introduction aux objets connectés
- **Définition de l'IdO:** Interconnexion d'appareils physiques via Internet
- **Architecture typique:** Capteurs → Microcontrôleur → Communication → Cloud → Application
- **Cas d'usage:** Domotique, agriculture, santé, transport, industrie 4.0
- **Enjeux:** Sécurité, vie privée, consommation énergétique, interopérabilité

#### 1.2 Architecture du cours
- **Raspberry Pi 5:** Serveur central (broker MQTT, interface utilisateur)
- **LilyGO A7670G:** Objet connecté (ESP32 + LTE + GPS)
- **Cloudflare Tunnel:** Exposition sécurisée sans ouverture de ports
- **Protocoles:** MQTT, HTTP/REST, WebSocket

#### 1.3 Sécurité et exposition des services
- **Problématique:** Exposer un service sur Internet de façon sécurisée
- **Solution Cloudflare Tunnel:** Connexion sortante uniquement, pas de ports ouverts
- **Zero Trust:** Authentification et autorisation à chaque requête

### Activités pratiques (3h) — Labo 1

#### Atelier 1: Installation Ubuntu Server sur Raspberry Pi 5
- Préparation de la carte SD avec Raspberry Pi Imager
- Configuration: hostname, username, password, SSH activé
- Premier démarrage et connexion

#### Atelier 2: Configuration réseau
- Configuration réseau filaire et WiFi
- Test de connectivité Internet
- Mise à jour du système (`apt update && apt upgrade`)

#### Atelier 3: Installation Cloudflare Tunnel
- Création de compte Cloudflare
- Installation de `cloudflared`
- Configuration du tunnel SSH
- Test de connexion à distance

#### Atelier 4: Prise en main LilyGO A7670G
- Installation Arduino CLI sur le Raspberry Pi
- Connexion USB du LilyGO
- Premier programme (blink LED)

### Travail personnel (2h)
- [ ] Finaliser l'installation des outils sur le Raspberry Pi
- [ ] Créer les comptes nécessaires (GitHub, Cloudflare)
- [ ] Lecture: Documentation Cloudflare Tunnel
- [ ] Lecture: Introduction à MQTT (mqtt.org)

### Évaluation formative
- **Diagnostic initial:** Questionnaire sur les prérequis
- **Vérification:** Raspberry Pi accessible via SSH depuis l'extérieur

### Ressources
- **Labo 1:** `labo1/Labo1-environnement de programmation distant sur rPi.md`
- **Liste de pièces:** `liste-pieces-labo1.md`

---

## Semaine 2 — Communication MQTT sans fil (WiFi et LTE)

### Objectifs d'apprentissage
À la fin de cette semaine, l'étudiant sera capable de:
- Expliquer le fonctionnement du protocole MQTT (publish/subscribe)
- Configurer un broker Mosquitto avec WSS/TLS
- Programmer une communication MQTT via WiFi sur le LilyGO A7670G
- Programmer une communication MQTT via LTE sur le LilyGO A7670G
- Créer une interface tactile Python pour contrôler des LEDs

### Prérequis
- Semaine 1 complétée (Raspberry Pi accessible, LilyGO fonctionnel)
- Notions de base en Python

### Contenus théoriques (2h)

#### 2.1 Protocole MQTT
- **Architecture publish/subscribe:** Découplage émetteur/récepteur
- **Broker:** Serveur central qui route les messages
- **Topics:** Hiérarchie de sujets (ex: `maison/salon/temperature`)
- **QoS (Quality of Service):**
  - QoS 0: Au plus une fois (fire and forget)
  - QoS 1: Au moins une fois (accusé de réception)
  - QoS 2: Exactement une fois (handshake complet)

#### 2.2 Sécurisation MQTT
- **TLS/SSL:** Chiffrement des communications
- **WebSocket Secure (WSS):** MQTT sur WebSocket avec TLS
- **Authentification:** Username/password ou certificats
- **Cloudflare Tunnel:** Exposition sécurisée du broker

#### 2.3 Communication sans fil
- **WiFi:** Connexion locale, haute bande passante
- **LTE (Cat-1):** Connexion cellulaire, couverture étendue
- **Comparaison:** Latence, consommation, coût, portée

### Activités pratiques (3h) — Labo 2

#### Atelier 1: Configuration broker MQTT avec WSS
- Installation Mosquitto sur Raspberry Pi 5
- Configuration WebSocket avec TLS
- Exposition via Cloudflare Tunnel
- Test avec client MQTT (mosquitto_pub/sub)

#### Atelier 2: Communication MQTT via WiFi
- Programmation LilyGO A7670G en WiFi
- Connexion au broker via WSS
- Publication de données (température, état)
- Souscription à des commandes (LED on/off)

#### Atelier 3: Communication MQTT via LTE
- Configuration modem LTE du LilyGO A7670G
- Activation de la connexion de données
- Communication MQTT via réseau cellulaire
- Comparaison WiFi vs LTE

#### Atelier 4: Interface tactile Python
- Création d'une interface pygame sur Raspberry Pi
- Connexion MQTT avec paho-mqtt
- Affichage des données reçues
- Boutons de contrôle des LEDs

### Travail personnel (2h)
- [ ] Finaliser la communication MQTT bidirectionnelle
- [ ] Documenter la configuration du broker
- [ ] Tester la robustesse de la connexion LTE
- [ ] Préparer le montage breadboard pour semaine 3

### Évaluation formative
- **Mini-vérifications:** Communication MQTT fonctionnelle
- **Observation:** Qualité du code et de la documentation

### Ressources
- **Labo 2:** `labo2/Labo2-communication-sans-fil-MQTT-LTE.md`
- **Liste de pièces:** `liste-pieces-labo2.md`

---

## Semaine 3 — Intégration complète et montage breadboard

### Objectifs d'apprentissage
À la fin de cette semaine, l'étudiant sera capable de:
- Réaliser un montage breadboard complet avec capteurs et actionneurs
- Programmer la lecture de capteurs analogiques et numériques
- Implémenter une chaîne d'acquisition complète (capteur → serveur)
- Valider la robustesse d'un système IoT

### Prérequis
- Semaines 1-2 complétées
- Broker MQTT fonctionnel
- Communication LilyGO ↔ Raspberry Pi établie

### Contenus théoriques (2h)

#### 3.1 Interfaçage capteurs/actionneurs
- **GPIO (General Purpose Input/Output):** Entrées/sorties numériques
- **ADC (Analog-to-Digital Converter):** Conversion analogique-numérique
- **I2C:** Bus de communication série pour capteurs intelligents
- **Résistances pull-up/pull-down:** Stabilisation des entrées

#### 3.2 Traitement des données
- **Filtrage:** Moyenne mobile, filtre passe-bas
- **Calibration:** Ajustement des valeurs brutes
- **Formats de données:** JSON pour structurer les données
- **Validation:** Vérification de la cohérence des données

#### 3.3 Bonnes pratiques IoT
- **Robustesse:** Reconnexion automatique, gestion des erreurs
- **Efficacité énergétique:** Mode veille, fréquence d'envoi adaptée
- **Sécurité:** Validation des entrées, chiffrement

### Activités pratiques (3h) — Labo 2 (suite)

#### Atelier 1: Montage breadboard complet
- Câblage de 2 LEDs avec résistances
- Câblage de 2 boutons poussoirs
- Vérification des connexions
- Test individuel de chaque composant

#### Atelier 2: Programmation des capteurs
- Lecture des boutons avec debounce
- Publication de l'état des boutons via MQTT
- Contrôle des LEDs via commandes MQTT
- Gestion des événements (pressed/released)

#### Atelier 3: Chaîne d'acquisition complète
- Intégration capteurs + actionneurs + MQTT
- Interface Python mise à jour
- Test de bout en bout
- Documentation technique

#### Atelier 4: Tests de robustesse
- Déconnexion/reconnexion WiFi
- Déconnexion/reconnexion LTE
- Comportement en cas de perte du broker
- Mesure de la latence et fiabilité

### Travail personnel (2h)
- [ ] Finaliser le montage breadboard
- [ ] Compléter la documentation technique
- [ ] Préparer la démonstration pour l'évaluation
- [ ] Réviser les concepts pour l'évaluation sommative

### Évaluation sommative (15%)
**Laboratoire capteurs et Python — Mise en œuvre d'une chaîne d'acquisition sur Raspberry Pi 5/LilyGO**

| Critère | Pondération |
|---------|-------------|
| Montage breadboard fonctionnel | 20% |
| Communication MQTT bidirectionnelle | 30% |
| Interface Python fonctionnelle | 25% |
| Documentation technique | 15% |
| Qualité du code | 10% |

### Ressources
- **Labo 2:** `labo2/Labo2-communication-sans-fil-MQTT-LTE.md`
- **Code référence:** `labo2/code/`

---

# PHASE 2 — Conception PCB avec KiCad (Semaines 4-7)

---

## Semaine 4 — Workflow complet KiCad

### Objectifs d'apprentissage
À la fin de cette semaine, l'étudiant sera capable de:
- Utiliser GitHub Desktop pour la gestion de version
- Créer un projet KiCad et naviguer dans l'interface
- Réaliser une capture schématique avec symboles et connexions
- Exécuter et corriger les erreurs ERC
- Associer les empreintes (footprints) aux symboles

### Prérequis
- Phase 1 complétée
- Notions de base en électronique (schémas, composants)

### Contenus théoriques (2h)

#### 4.1 Introduction à KiCad
- **Présentation:** Suite logicielle open-source de conception électronique
- **Applications:** Eeschema (schéma), PCB Editor, Gerber Viewer
- **Workflow:** Schéma → ERC → Footprints → PCB → DRC → Gerbers

#### 4.2 Capture schématique
- **Symboles:** Représentation graphique des composants
- **Connexions:** Fils, labels, bus
- **Alimentation:** Symboles PWR_FLAG, GND, VCC
- **Hiérarchie:** Feuilles multiples pour circuits complexes

#### 4.3 Vérification électrique (ERC)
- **Erreurs courantes:** Pins non connectées, conflits d'alimentation
- **Règles:** Entrées/sorties, puissance, bidirectionnel
- **Résolution:** Correction des erreurs avant passage au PCB

#### 4.4 Association des empreintes
- **Empreinte (footprint):** Représentation physique du composant
- **Bibliothèques:** Footprints standards vs personnalisés
- **CMS vs traversant:** Surface Mount vs Through-Hole

### Activités pratiques (3h) — Labo 3

#### Atelier 1: Installation et configuration
- Installation de KiCad (version 8.0+)
- Installation de GitHub Desktop
- Clonage du dépôt du cours
- Création de la branche personnelle

#### Atelier 2: Création du projet KiCad
- Nouveau projet dans le dépôt
- Structure de fichiers recommandée
- Configuration des préférences

#### Atelier 3: Capture schématique
- Placement des symboles (résistances, LEDs, connecteurs)
- Tracé des connexions
- Ajout des labels et annotations
- Numérotation des composants

#### Atelier 4: ERC et footprints
- Exécution de l'ERC
- Correction des erreurs
- Association des footprints à chaque symbole
- Vérification des empreintes

### Travail personnel (2h)
- [ ] Réviser le workflow KiCad
- [ ] Préparer la liste des composants pour le projet mi-session
- [ ] Lire la documentation sur le routage PCB
- [ ] Commiter le travail sur GitHub

### Évaluation formative
- **Vérification ciblée:** Schéma ERC propre avec footprints associés

### Ressources
- **Labo 3:** `labo3/Labo3-workflow-KiCad.md`
- **Documentation KiCad:** https://docs.kicad.org/

---

## Semaine 5 — Projet de mi-session: Prototype breadboard et schéma

### Objectifs d'apprentissage
À la fin de cette semaine, l'étudiant sera capable de:
- Concevoir un prototype breadboard fonctionnel
- Capturer un schéma électronique complet dans KiCad
- Intégrer un accéléromètre I2C dans un circuit
- Documenter le processus de conception

### Prérequis
- Semaine 4 complétée (workflow KiCad maîtrisé)
- Montage breadboard de base fonctionnel (semaine 3)

### Contenus théoriques (2h)

#### 5.1 Conception de shield
- **Définition:** Carte d'extension pour microcontrôleur
- **Contraintes:** Compatibilité des broches, encombrement
- **Headers:** Connecteurs mâles/femelles pour empilage

#### 5.2 Communication I2C
- **Principe:** Bus série synchrone à 2 fils (SDA, SCL)
- **Adressage:** Chaque périphérique a une adresse unique
- **Pull-ups:** Résistances de rappel sur les lignes de données
- **Accéléromètre MPU6050/ADXL345:** Mesure d'accélération 3 axes

#### 5.3 Cahier des charges du shield
- 2 LEDs avec résistances de limitation
- 2 boutons poussoirs avec pull-up/pull-down
- 2 potentiomètres (entrées analogiques)
- 1 accéléromètre I2C
- Headers compatibles LilyGO A7670G

### Activités pratiques (3h)

#### Atelier 1: Prototype breadboard complet
- Montage des composants sur breadboard
- Intégration de l'accéléromètre I2C
- Test de tous les capteurs et actionneurs
- Validation du fonctionnement

#### Atelier 2: Capture schématique KiCad
- Création du schéma du shield
- Placement des composants
- Connexions aux headers du LilyGO
- Symboles d'alimentation

#### Atelier 3: Documentation
- Prise de photos du prototype
- Rédaction des spécifications
- Description du fonctionnement
- Début du README du projet

### Travail personnel (2h)
- [ ] Finaliser le prototype breadboard
- [ ] Compléter le schéma KiCad
- [ ] Documenter le prototype avec photos
- [ ] Préparer les questions pour la semaine suivante

### Évaluation formative
- **Suivi de progression:** Prototype fonctionnel, schéma en cours

### Ressources
- **Projet mi-session:** `Projet-mi-session.md`
- **Code LilyGO:** `labo2/code/lilygo_lte_mqtt/`

---

## Semaine 6 — Projet de mi-session: Routage PCB et fabrication

### Objectifs d'apprentissage
À la fin de cette semaine, l'étudiant sera capable de:
- Placer les composants sur un PCB 2 couches
- Router les pistes en respectant les règles de conception
- Configurer et exécuter le DRC
- Générer les fichiers de fabrication (Gerbers, BOM)

### Prérequis
- Schéma KiCad complet avec footprints
- Prototype breadboard validé

### Contenus théoriques (2h)

#### 6.1 Conception PCB
- **Couches:** Top (composants), Bottom (soudure)
- **Plan de masse:** Distribution du GND sur toute la carte
- **Règles de routage:** Largeur de pistes, espacement, vias
- **Contraintes thermiques:** Dissipation, pistes de puissance

#### 6.2 DRC (Design Rules Check)
- **Règles électriques:** Court-circuits, connexions manquantes
- **Règles de fabrication:** Largeur min, espacement min
- **Silkscreen:** Noms des composants, repères

#### 6.3 Fichiers de fabrication
- **Gerbers:** Format standard pour la fabrication
- **Drill files:** Fichiers de perçage
- **BOM (Bill of Materials):** Liste des composants
- **Pick and place:** Positionnement pour assemblage automatique

### Activités pratiques (3h)

#### Atelier 1: Placement des composants
- Import du schéma dans le PCB Editor
- Définition du contour de carte
- Placement stratégique des composants
- Optimisation de l'encombrement

#### Atelier 2: Routage des pistes
- Routage des signaux critiques
- Création du plan de masse
- Ajout de vias si nécessaire
- Optimisation des chemins

#### Atelier 3: Vérification et fabrication
- Exécution du DRC
- Correction des erreurs
- Génération des Gerbers
- Génération de la BOM

#### Atelier 4: Documentation finale
- Rendu 3D du PCB
- Capture d'écran du routage
- Vérification des fichiers Gerbers
- Préparation de la remise

### Travail personnel (2h)
- [ ] Finaliser le routage PCB
- [ ] Corriger toutes les erreurs DRC
- [ ] Générer les fichiers de fabrication
- [ ] Préparer la documentation pour la remise

### Évaluation formative
- **Suivi de progression:** PCB routé, DRC propre

### Ressources
- **Labo 3:** `labo3/Labo3-workflow-KiCad.md`
- **Fabricants PCB:** JLCPCB, PCBWay, OSH Park

---

## Semaine 7 — Remise projet mi-session + Introduction LoRa/Meshtastic

### Objectifs d'apprentissage
À la fin de cette semaine, l'étudiant sera capable de:
- Livrer un projet PCB complet avec documentation
- Comprendre les principes de Meshtastic et du réseau mesh LoRa
- Configurer un T-Beam SUPREME de base
- Effectuer des premiers essais de communication LoRa

### Prérequis
- Projet de mi-session finalisé
- Phases 1-2 complétées

### Contenus théoriques (2h)

#### 7.1 Architecture réseau mesh Meshtastic
- **Topologie mesh vs centralisée:** Avantages de la décentralisation
- **Protocole LoRa:** Longue portée, faible consommation
- **Bandes ISM:** 863-928 MHz selon région
- **Types de nœuds:** Client, Router, Repeater

#### 7.2 Matériel T-Beam SUPREME
- **ESP32-S3:** Microcontrôleur principal
- **Module LoRa SX1262:** Communication longue portée
- **GPS intégré:** Géolocalisation
- **Gestion batterie:** Autonomie avec 18650

### Activités pratiques (3h)

#### Partie 1: Remise projet mi-session (1h)
- Démonstration du prototype breadboard
- Présentation du schéma et PCB
- Vérification des fichiers Gerbers
- Questions/réponses

#### Partie 2: Introduction Meshtastic (2h)
- Flash du firmware Meshtastic sur T-Beam SUPREME
- Configuration de base (région, nom)
- Mise en réseau de 2 nœuds
- Test de portée locale

### Travail personnel (2h)
- [ ] Installation meshtastic-python
- [ ] Lecture documentation Meshtastic
- [ ] Préparation tests de portée
- [ ] Révision SF/BW/CR

### Évaluation sommative (20%)
**Projet de mi-session — Shield PCB pour LilyGO A7670G**

| Critère | Pondération | Capacité |
|---------|-------------|----------|
| Prototype breadboard fonctionnel | 20% | C1 |
| Schéma KiCad (ERC propre) | 25% | C1 |
| PCB routé (DRC propre) | 25% | C1 |
| Fichiers fabrication (Gerbers, BOM) | 15% | C1 |
| Documentation technique | 10% | C2 |
| Qualité du code embarqué | 5% | C2 |

### Ressources
- **Projet mi-session:** `Projet-mi-session.md`
- **LoRa/Meshtastic:** `semaines-lora-meshtastic.md`

---

# PHASE 3 — Réseau mesh LoRa/Meshtastic (Semaines 7-9)

> **Note:** Le contenu détaillé des semaines 7-9 est disponible dans le document `semaines-lora-meshtastic.md`

---

## Semaine 8 — Réseau mesh et configuration avancée

### Objectifs d'apprentissage
À la fin de cette semaine, l'étudiant sera capable de:
- Configurer des relais mesh (rôles Router / Router-Critical)
- Comprendre l'impact des paramètres LoRa (SF, BW, CR)
- Mettre en place un réseau mesh multi-nœuds (4-6 nœuds)
- Installer un relais fixe permanent

### Prérequis
- Semaine 7 complétée (T-Beam flashé et fonctionnel)
- Communication LoRa entre 2 nœuds établie

### Contenus théoriques (2h)

#### 8.1 Paramètres LoRa: SF, BW, CR
- **Spreading Factor (SF7-SF12):** Compromis portée/débit
- **Bandwidth (125-500 kHz):** Largeur de bande
- **Coding Rate (4/5-4/8):** Correction d'erreur
- **Presets Meshtastic:** SHORT_FAST, MEDIUM_FAST, LONG_SLOW

#### 8.2 Rôles des nœuds
- **Client:** Nœud utilisateur standard
- **Router:** Relais optimisé, Bluetooth désactivé
- **Router-Critical:** Toujours actif, jamais en veille
- **Repeater:** Relais pur, consommation minimale

#### 8.3 Mode Beacon GPS
- Partage périodique de position
- Cas d'usage: suivi, SAR
- Configuration broadcast_secs

### Activités pratiques (3h)

#### Atelier 1: Mise en réseau 4-6 nœuds
- Configuration de tous les nœuds sur même canal
- Attribution des rôles (Routers, Clients)
- Test communication multi-sauts
- Mesure du hop count

#### Atelier 2: Tests SF7/SF9/SF12
- Configuration SHORT_FAST, MEDIUM_FAST, LONG_SLOW
- Mesure portée maximale pour chaque SF
- Comparaison latence et consommation
- Tableau comparatif des résultats

#### Atelier 3: Installation relais fixe
- Configuration Router-Critical
- Optimisation alimentation
- Placement en hauteur
- Test de couverture avant/après

### Travail personnel (2h)
- [ ] Compléter les tests SF
- [ ] Documenter les résultats comparatifs
- [ ] Préparer le relais pour installation permanente
- [ ] Lire documentation gateway MQTT

### Évaluation formative
- **Revue de progression:** Réseau mesh fonctionnel, tableau comparatif SF

### Ressources
- **Documentation complète:** `semaines-lora-meshtastic.md`

---

## Semaine 9 — Gateway WiFi + MQTT + Tests terrain

### Objectifs d'apprentissage
À la fin de cette semaine, l'étudiant sera capable de:
- Configurer le T-Beam SUPREME comme gateway WiFi → MQTT autonome
- Connecter le réseau mesh au broker Mosquitto existant
- Effectuer des tests terrain (portée, latence, stabilité)
- Créer une carte de couverture LoRa avec données GPS

### Prérequis
- Réseau mesh multi-nœuds fonctionnel
- Broker Mosquitto configuré (Phase 1)

### Contenus théoriques (2h)

#### 9.1 Gateway LoRa → MQTT
- **Concept:** Pont entre réseau mesh et Internet
- **Architecture:** T-Beam WiFi → Mosquitto → Cloudflare
- **Topics MQTT Meshtastic:** Structure des messages
- **Intégration:** Même broker que le LilyGO A7670G

#### 9.2 Tests de performance
- **Portée maximale:** Distance en ligne de vue
- **Latence:** Temps de transmission
- **Stabilité:** Taux de perte, reconnexions

#### 9.3 Cartographie de couverture
- **Collecte GPS:** Positions avec métadonnées RSSI/SNR
- **Format GPX:** Standard pour traces GPS
- **Visualisation:** QGIS, Google Earth

### Activités pratiques (3h)

#### Atelier 1: Configuration gateway WiFi + MQTT
- Activation WiFi sur T-Beam SUPREME
- Configuration MQTT vers broker local
- Test communication bidirectionnelle LoRa ↔ MQTT
- Intégration avec l'infrastructure existante

#### Atelier 2: Tests terrain
- Test de portée maximale (SF12)
- Mesure de latence à différentes distances
- Effet des obstacles (bâtiments)
- Collecte de données RSSI/SNR/GPS

#### Atelier 3: Cartographie de couverture
- Parcours de test avec nœud mobile
- Conversion des données en GPX
- Visualisation sur carte
- Analyse des zones de couverture

### Travail personnel (2h)
- [ ] Finaliser les tests terrain
- [ ] Créer la carte de couverture
- [ ] Rédiger le rapport de tests
- [ ] Préparer l'évaluation sommative

### Évaluation sommative (20%)
**TP Intégration LLM et automatisation — Flux capteurs → traitement → action**

| Critère | Pondération | Capacité |
|---------|-------------|----------|
| Gateway WiFi→MQTT fonctionnel | 25% | C1 |
| Réseau mesh stable (3-4 nœuds) | 20% | C1 |
| Carte de couverture LoRa | 20% | C1 |
| Tests de performance documentés | 20% | C2 |
| Documentation technique | 15% | C2 |

### Ressources
- **Documentation complète:** `semaines-lora-meshtastic.md`
- **Architecture finale:** `architecture-finale-projet.md`

---

# PHASE 4 — Assemblage PCB et automatisation LLM (Semaines 10-12)

---

## Semaine 10 — Réception et soudure PCB

### Objectifs d'apprentissage
À la fin de cette semaine, l'étudiant sera capable de:
- Inspecter un PCB reçu de fabrication
- Préparer un plan d'assemblage
- Souder des composants CMS et traversants
- Effectuer des tests électriques de validation

### Prérequis
- Projet mi-session complété (Gerbers envoyés)
- Notions de soudure (si possible)

### Contenus théoriques (2h)

#### 10.1 Inspection de PCB
- **Vérification visuelle:** Pistes, vias, silkscreen
- **Correspondance Gerbers:** Comparaison avec les fichiers envoyés
- **Défauts courants:** Courts-circuits, pistes coupées, vias bouchés

#### 10.2 Techniques de soudure
- **Fer à souder:** Température, pointe, flux
- **Composants traversants:** Technique standard
- **Composants CMS:** Étain sur pad, placement, refusion
- **Inspection:** Loupe, multimètre

#### 10.3 Tests électriques
- **Continuité:** Vérification des connexions
- **Isolation:** Absence de courts-circuits
- **Alimentation:** Tensions correctes
- **Fonctionnel:** Test de chaque sous-circuit

### Activités pratiques (3h)

#### Atelier 1: Réception et inspection
- Déballage des PCB
- Inspection visuelle
- Revue par les pairs
- Liste des défauts éventuels

#### Atelier 2: Préparation à l'assemblage
- Tri des composants selon la BOM
- Préparation du poste de soudure
- Plan d'assemblage (ordre des composants)
- Organisation du matériel

#### Atelier 3: Soudure
- Soudure des composants passifs (résistances, condensateurs)
- Soudure des connecteurs (headers)
- Soudure des composants actifs (LEDs, boutons)
- Soudure de l'accéléromètre (si CMS)

#### Atelier 4: Tests électriques
- Test de continuité
- Test d'alimentation
- Test fonctionnel basique
- Documentation des résultats

### Travail personnel (2h)
- [ ] Finaliser l'assemblage si nécessaire
- [ ] Documenter le processus avec photos
- [ ] Préparer le test fonctionnel complet
- [ ] Réviser pour la semaine suivante

### Évaluation formative
- **Contrôle des livrables PCB:** PCB assemblé, tests de base réussis

### Ressources
- **Vidéos soudure:** YouTube - EEVblog, GreatScott!
- **Outils:** Fer à souder, flux, tresse à dessouder, loupe

---

## Semaine 11 — Automatisation LLM: Déclencheurs et pipelines

### Objectifs d'apprentissage
À la fin de cette semaine, l'étudiant sera capable de:
- Comprendre les principes d'automatisation avec LLM
- Créer des déclencheurs basés sur les données MQTT
- Construire des pipelines de traitement de données
- Intégrer un LLM pour l'analyse et la prise de décision

### Prérequis
- PCB assemblé et testé
- Compte LLM actif (Claude, Gemini, ou ChatGPT)

### Contenus théoriques (2h)

#### 11.1 Introduction aux LLM pour l'IoT
- **Cas d'usage:** Analyse de données, génération d'alertes, prise de décision
- **API LLM:** OpenAI, Anthropic, Google
- **Prompts:** Structuration des requêtes
- **Limites:** Latence, coût, hallucinations

#### 11.2 Architecture d'automatisation
- **Sources de données:** Capteurs via MQTT
- **Traitement:** Scripts Python, règles conditionnelles
- **LLM:** Analyse sémantique, décisions complexes
- **Actions:** Notifications, commandes, logs

#### 11.3 Pipelines de données
- **ETL (Extract, Transform, Load):** Flux de données
- **Événements:** Déclencheurs basés sur seuils ou patterns
- **Orchestration:** Séquencement des actions

### Activités pratiques (3h)

#### Atelier 1: Collecte de données MQTT
- Script Python de collecte
- Stockage des données (fichier, base de données)
- Filtrage et agrégation
- Visualisation basique

#### Atelier 2: Création de déclencheurs
- Règles conditionnelles (seuils, patterns)
- Déclencheurs temporels
- Alertes par email ou notification
- Logging des événements

#### Atelier 3: Intégration LLM
- Configuration de l'API LLM
- Création de prompts pour l'analyse
- Traitement des réponses
- Actions automatisées

### Travail personnel (2h)
- [ ] Développer le pipeline de traitement
- [ ] Tester les déclencheurs
- [ ] Documenter l'architecture
- [ ] Préparer les tests de fiabilité

### Évaluation formative
- **Dépôt formatif:** Schéma/PCB et architecture du projet final

### Ressources
- **API Claude:** https://docs.anthropic.com/
- **API OpenAI:** https://platform.openai.com/docs
- **Paho MQTT Python:** https://pypi.org/project/paho-mqtt/

---

## Semaine 12 — Automatisation LLM: Fiabilité et sécurité

### Objectifs d'apprentissage
À la fin de cette semaine, l'étudiant sera capable de:
- Mettre en production des automatisations fiables
- Gérer les erreurs et les cas limites
- Sécuriser les secrets et les accès
- Monitorer les systèmes automatisés

### Prérequis
- Pipeline d'automatisation LLM fonctionnel
- Déclencheurs testés

### Contenus théoriques (2h)

#### 12.1 Fiabilité des systèmes
- **Gestion des erreurs:** Try/except, retry, fallback
- **Timeout:** Limites de temps pour les appels API
- **Idempotence:** Actions qui peuvent être répétées sans effet secondaire
- **Logging:** Journalisation pour le débogage

#### 12.2 Sécurité
- **Secrets:** Variables d'environnement, fichiers .env
- **Authentification:** Tokens, clés API
- **Autorisation:** Contrôle d'accès aux ressources
- **Chiffrement:** TLS, stockage sécurisé

#### 12.3 Observabilité
- **Monitoring:** Surveillance en temps réel
- **Alertes:** Notifications en cas d'anomalie
- **Métriques:** Latence, taux d'erreur, utilisation
- **Dashboards:** Visualisation des indicateurs

### Activités pratiques (3h)

#### Atelier 1: Gestion des erreurs
- Implémentation de try/except
- Stratégie de retry avec backoff
- Fallback en cas d'échec
- Tests de robustesse

#### Atelier 2: Sécurisation
- Gestion des secrets avec .env
- Sécurisation des API keys
- Validation des entrées
- Audit des accès

#### Atelier 3: Monitoring
- Configuration des logs
- Création d'alertes
- Dashboard de surveillance
- Tests de scénarios d'erreur

### Travail personnel (2h)
- [ ] Finaliser l'automatisation
- [ ] Documenter la gestion des erreurs
- [ ] Mettre en place le monitoring
- [ ] Préparer pour le projet final

### Évaluation formative
- **Point d'étape projet:** Automatisation fiable et sécurisée

### Ressources
- **python-dotenv:** https://pypi.org/project/python-dotenv/
- **Logging Python:** https://docs.python.org/3/library/logging.html

---

# PHASE 5 — Projet final et déploiement (Semaines 13-15)

---

## Semaine 13 — Intégration matérielle et logicielle

### Objectifs d'apprentissage
À la fin de cette semaine, l'étudiant sera capable de:
- Assembler un système IoT complet
- Intégrer le PCB, Meshtastic et l'infrastructure
- Tester les performances globales
- Valider l'autonomie énergétique

### Prérequis
- PCB assemblé et testé
- Réseau Meshtastic configuré
- Pipeline d'automatisation fonctionnel

### Contenus théoriques (2h)

#### 13.1 Intégration système
- **Architecture complète:** Capteurs → PCB → LilyGO → MQTT → Pi → LLM
- **Interfaces:** Connexions entre les sous-systèmes
- **Synchronisation:** Timing et coordination
- **Redondance:** Chemins alternatifs (LTE + LoRa)

#### 13.2 Tests de performance
- **Latence bout-en-bout:** Du capteur à l'action
- **Fiabilité:** Taux de réussite des transmissions
- **Autonomie:** Durée de vie sur batterie
- **Stress tests:** Comportement sous charge

#### 13.3 QoS et optimisation
- **Quality of Service:** Niveaux MQTT
- **Priorités:** Messages critiques vs informatifs
- **Compression:** Réduction de la bande passante
- **Caching:** Stockage local en cas de perte de connexion

### Activités pratiques (3h)

#### Atelier 1: Assemblage final
- Intégration du PCB avec le LilyGO
- Connexion au réseau Meshtastic
- Configuration de l'automatisation
- Tests de base

#### Atelier 2: Tests de performance
- Mesure de latence
- Test de fiabilité (100 messages)
- Test d'autonomie
- Stress test

#### Atelier 3: Documentation
- Photos du système assemblé
- Schémas d'architecture
- Résultats des tests
- Liste des problèmes à résoudre

### Travail personnel (2h)
- [ ] Résoudre les problèmes identifiés
- [ ] Compléter la documentation
- [ ] Préparer la démonstration
- [ ] Réviser pour la présentation

### Évaluation formative
- **Dépôt formatif:** Prototype fonctionnel et documentation préliminaire

### Ressources
- **Architecture finale:** `architecture-finale-projet.md`

---

## Semaine 14 — Validation et préparation de la présentation

### Objectifs d'apprentissage
À la fin de cette semaine, l'étudiant sera capable de:
- Effectuer les tests finaux du système
- Corriger les derniers problèmes
- Préparer une présentation professionnelle
- Rédiger la documentation technique finale

### Prérequis
- Système intégré et fonctionnel
- Documentation préliminaire

### Contenus théoriques (2h)

#### 14.1 Documentation technique professionnelle
- **Structure:** Introduction, architecture, installation, utilisation
- **Diagrammes:** Schémas clairs et annotés
- **Code:** Commentaires, README, exemples
- **Maintenance:** Guide de dépannage

#### 14.2 Présentation technique
- **Structure:** Problème → Solution → Démonstration → Résultats
- **Visuels:** Diagrammes, photos, vidéos
- **Timing:** Gestion du temps
- **Questions:** Anticipation des questions

### Activités pratiques (3h)

#### Atelier 1: Tests finaux
- Vérification complète du système
- Tests de cas limites
- Validation de la documentation
- Correction des bugs

#### Atelier 2: Préparation présentation
- Création des diapositives
- Préparation de la démonstration
- Répétition de la présentation
- Timing et ajustements

#### Atelier 3: Finalisation documentation
- Rédaction du rapport final
- Vérification de la complétude
- Relecture et corrections
- Mise en forme

### Travail personnel (2h)
- [ ] Finaliser la présentation
- [ ] Répéter la démonstration
- [ ] Compléter le rapport
- [ ] Préparer les réponses aux questions

### Évaluation formative
- **Répétition évaluée:** Présentation d'entraînement avec feedback

### Ressources
- **Modèle de rapport:** À fournir par l'enseignant

---

## Semaine 15 — Présentation finale et remise

### Objectifs d'apprentissage
À la fin de cette semaine, l'étudiant sera capable de:
- Démontrer un système IoT complet et fonctionnel
- Présenter son travail de façon professionnelle
- Répondre aux questions techniques
- Livrer une documentation complète

### Prérequis
- Projet finalisé
- Présentation préparée
- Documentation complète

### Déroulement de la séance (5h)

#### Partie 1: Présentations (3h)
- Présentation de chaque équipe (10-15 min)
- Démonstration en direct
- Questions/réponses
- Évaluation par les pairs (optionnel)

#### Partie 2: Remise et rétroaction (2h)
- Remise des rapports finaux
- Rétroaction individuelle
- Bilan de la session
- Discussion sur les améliorations possibles

### Évaluation sommative (30%)
**Projet final IdO avec PCB et déploiement**

| Critère | Pondération | Capacité |
|---------|-------------|----------|
| Système fonctionnel complet | 25% | C1 |
| Intégration Meshtastic | 15% | C1 |
| Automatisation LLM | 20% | C2 |
| Présentation et démonstration | 15% | C1/C2 |
| Documentation technique | 15% | C1/C2 |
| Qualité du code | 10% | C1 |

### Critères de réussite globale
- **Seuil de réussite:** 60% sur l'ensemble du cours
- **Capacité 1 (Conception et programmation):** 45%
- **Capacité 2 (Protocoles de communication):** 50%
- **Qualité du français:** 5% (intégré)

### Ressources
- **Structure de remise:** Voir `Projet-mi-session.md`
- **Critères détaillés:** Plan de cours, section 3

---

## Récapitulatif des évaluations sommatives

| Semaine | Évaluation | Pondération | Capacités |
|---------|------------|-------------|-----------|
| 3 | Labo capteurs et Python | 15% | C1: 15% |
| 7 | Projet mi-session Shield PCB | 20% | C1: 15%, C2: 5% |
| 9 | TP Intégration LLM | 20% | C1: 15%, C2: 5% |
| 15 | Projet final IdO | 30% | C1: 10%, C2: 20% |
| - | Qualité du français | 5% | Intégré |
| **Total** | | **90%** | C1: 45%, C2: 50% |

---

## Ressources générales

### Documentation du cours
- `Plan de cours.md` — Plan de cours officiel
- `Projet-mi-session.md` — Spécifications du projet de mi-session
- `architecture-finale-projet.md` — Architecture du projet final
- `semaines-lora-meshtastic.md` — Détail des semaines LoRa/Meshtastic

### Laboratoires
- `labo1/` — Environnement de programmation distant
- `labo2/` — Communication sans fil MQTT/LTE
- `labo3/` — Workflow KiCad

### Outils
- **Raspberry Pi:** Ubuntu Server, Mosquitto, Python
- **LilyGO A7670G:** Arduino CLI, ESP32
- **T-Beam SUPREME:** Meshtastic, LoRa
- **KiCad:** Conception PCB
- **GitHub:** Gestion de version
- **Cloudflare:** Tunnel sécurisé

### Communauté et support
- **GitHub Issues:** Questions et problèmes
- **Discord Meshtastic:** https://discord.gg/meshtastic
- **Documentation Meshtastic:** https://meshtastic.org/docs
