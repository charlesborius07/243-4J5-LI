# Résumés hebdomadaires pour Moodle

## Semaine 1

<div style="font-family: 'Segoe UI', system-ui, sans-serif; line-height: 1.6;">
  <p>Introduction à l'<strong>Internet des objets (IoT)</strong> : réseaux d'appareils connectés qui collectent et échangent des données. Défis : alimentation, connectivité, sécurité.</p>
  <p><strong>Objectif du cours :</strong> concevoir, programmer et déployer des objets connectés en utilisant MQTT, LoRa, KiCad (PCB) et des outils professionnels.</p>
  <p><strong>Environnement de développement distant :</strong></p>
  <ul style="margin: 10px 0;">
    <li><strong>Raspberry Pi</strong> — station relais sur le site distant</li>
    <li><strong>SSH + Cloudflare Tunnel</strong> — accès sécurisé sans IP publique</li>
    <li><strong>Git + GitHub</strong> — versionnage et synchronisation du code</li>
    <li><strong>Arduino CLI</strong> — compiler et flasher sans interface graphique</li>
    <li><strong>Assistants IA CLI</strong> — coder efficacement en terminal</li>
  </ul>
  <p><strong>Labo 1 :</strong> installer Ubuntu Server sur le RPi, configurer le réseau et SSH, mettre en place Cloudflare Tunnel, et tester la compilation sur le LilyGO A7670G.</p>
  <p><strong>À faire pour la semaine prochaine :</strong></p>
  <ul style="margin: 10px 0;">
    <li>Finaliser la configuration du Raspberry Pi et Cloudflare Tunnel</li>
    <li>Lire la documentation sur le protocole MQTT</li>
    <li>S'assurer que le LilyGO A7670G compile et flashe correctement</li>
  </ul>
</div>

## Semaine 2

<div style="font-family: 'Segoe UI', system-ui, sans-serif; line-height: 1.6;">
  <p>Introduction au protocole <strong>MQTT</strong> (Message Queuing Telemetry Transport), le standard de communication pour l'IoT. Architecture <strong>publish/subscribe</strong> avec découplage spatial, temporel et de synchronisation.</p>
  <p><strong>Concepts clés :</strong></p>
  <ul style="margin: 10px 0;">
    <li><strong>Topics hiérarchiques</strong> — structure avec séparateur / (ex: maison/salon/temperature)</li>
    <li><strong>Wildcards</strong> — + (un niveau) et # (tous les niveaux)</li>
    <li><strong>QoS</strong> — trois niveaux de fiabilité (0, 1, 2)</li>
    <li><strong>Retained messages</strong> — dernière valeur conservée pour nouveaux subscribers</li>
    <li><strong>Last Will Testament</strong> — message automatique si déconnexion anormale</li>
  </ul>
  <p><strong>Broker Mosquitto :</strong> configuration avec authentification et WebSocket (WSS) pour traverser Cloudflare Tunnel.</p>
  <p><strong>WiFi Enterprise (WPA-EAP) :</strong> connexion au réseau du Cégep avec identifiants personnels via PEAP-MSCHAPv2.</p>
  <p><strong>Labo 2 :</strong> configurer Mosquitto, créer le tunnel MQTT via Cloudflare, et établir une communication bidirectionnelle LilyGO ↔ RPi pour contrôler des LEDs.</p>
  <p><strong>À faire pour la semaine prochaine :</strong></p>
  <ul style="margin: 10px 0;">
    <li>Finaliser la communication MQTT bidirectionnelle</li>
    <li>Préparer le montage breadboard (LEDs, boutons)</li>
    <li>Se préparer pour l'évaluation sommative de la semaine 3 (15%)</li>
  </ul>
</div>

## Semaine 3

<div style="font-family: 'Segoe UI', system-ui, sans-serif; line-height: 1.6;">
  <p>Communication cellulaire <strong>LTE</strong> pour l'IoT mobile : couverture étendue sans infrastructure WiFi. Le LilyGO A7670G utilise un modem <strong>Cat-1</strong> avec commandes <strong>AT</strong> pour la configuration réseau.</p>
  <p><strong>Communication LTE :</strong></p>
  <ul style="margin: 10px 0;">
    <li><strong>Commandes AT</strong> — protocole Hayes pour contrôler le modem (AT+CSQ, AT+CGDCONT)</li>
    <li><strong>APN</strong> — point d'accès réseau de l'opérateur (ex: hologram, ltemobile.apn)</li>
    <li><strong>Force du signal (CSQ)</strong> — indicateur de qualité de réception</li>
  </ul>
  <p><strong>Sécurité IoT :</strong></p>
  <ul style="margin: 10px 0;">
    <li><strong>TLS/SSL</strong> — chiffrement des communications avec certificats X.509</li>
    <li><strong>Gestion des secrets</strong> — fichier auth.h dans .gitignore, jamais de mots de passe dans le code</li>
    <li><strong>Reconnexion automatique</strong> — backoff exponentiel et watchdog timer</li>
  </ul>
  <p><strong>Labo 3 :</strong> configurer la communication LTE, implémenter la sécurité TLS, et préparer la vérification des laboratoires 1 et 2.</p>
  <p><strong>À faire pour la semaine prochaine :</strong></p>
  <ul style="margin: 10px 0;">
    <li>Installer KiCad sur votre ordinateur</li>
    <li>Consulter les tutoriels KiCad de base</li>
    <li>Réviser les concepts de schémas électriques</li>
  </ul>
</div>

## Semaine 4

<div style="font-family: 'Segoe UI', system-ui, sans-serif; line-height: 1.6;">
  <p>Introduction à la conception de <strong>circuits imprimés (PCB)</strong> avec <strong>KiCad</strong>, logiciel libre et professionnel. Transformer un prototype breadboard en produit fiable et reproductible.</p>
  <p><strong>Concepts PCB :</strong></p>
  <ul style="margin: 10px 0;">
    <li><strong>Schéma électrique</strong> — représentation logique des connexions</li>
    <li><strong>Empreintes (footprints)</strong> — dimensions physiques des composants</li>
    <li><strong>Pistes et pastilles</strong> — chemins de cuivre pour les connexions</li>
    <li><strong>ERC</strong> — vérification des règles électriques du schéma</li>
  </ul>
  <p><strong>Workflow KiCad :</strong></p>
  <ul style="margin: 10px 0;">
    <li><strong>Saisie du schéma</strong> — placement des symboles et connexions</li>
    <li><strong>Assignation des empreintes</strong> — THT (traversant) ou SMD (CMS)</li>
    <li><strong>Routage</strong> — tracer les pistes sur le PCB</li>
  </ul>
  <p><strong>Labo 4 :</strong> créer un premier projet KiCad, saisir un schéma simple (LED + résistance), et générer les fichiers de fabrication.</p>
  <p><strong>À faire pour la semaine prochaine :</strong></p>
  <ul style="margin: 10px 0;">
    <li>Rassembler les composants pour le prototype breadboard du projet</li>
    <li>Planifier le schéma du shield LilyGO (2 LEDs, 2 boutons, 2 potentiomètres, accéléromètre)</li>
    <li>Créer le dépôt GitHub pour le projet de mi-session</li>
  </ul>
</div>

## Semaine 5

<div style="font-family: 'Segoe UI', system-ui, sans-serif; line-height: 1.6;">
  <p>Début du projet de mi-session : conception d'un <strong>shield PCB pour le LilyGO A7670G</strong>. Communication <strong>I2C</strong> avec accéléromètre et lecture <strong>ADC</strong> des potentiomètres.</p>
  <p><strong>Protocole I2C :</strong></p>
  <ul style="margin: 10px 0;">
    <li><strong>Bus série 2 fils</strong> — SDA (données) et SCL (horloge)</li>
    <li><strong>Adressage</strong> — chaque périphérique a une adresse unique (ex: 0x68)</li>
    <li><strong>Résistances pull-up</strong> — maintiennent les lignes à l'état haut</li>
  </ul>
  <p><strong>Conversion ADC :</strong></p>
  <ul style="margin: 10px 0;">
    <li><strong>Résolution 12 bits</strong> — valeurs de 0 à 4095 sur l'ESP32</li>
    <li><strong>ADC1 uniquement</strong> — ADC2 incompatible avec WiFi actif (GPIO 32-39)</li>
    <li><strong>Filtrage</strong> — moyenne mobile ou filtre passe-bas (EMA) pour stabiliser</li>
  </ul>
  <p><strong>Labo 5 :</strong> monter le prototype complet sur breadboard, tester tous les capteurs/actionneurs, et documenter le projet sur GitHub.</p>
  <p><strong>À faire pour la semaine prochaine :</strong></p>
  <ul style="margin: 10px 0;">
    <li>Finaliser le prototype breadboard fonctionnel</li>
    <li>Compléter le schéma KiCad avec tous les composants</li>
    <li>Passer l'ERC sans erreurs</li>
  </ul>
</div>

## Semaine 6

<div style="font-family: 'Segoe UI', system-ui, sans-serif; line-height: 1.6;">
  <p>Finalisation du <strong>routage PCB</strong> et génération des <strong>fichiers Gerber</strong> pour la fabrication. Vérification DRC et préparation à la commande chez un fabricant.</p>
  <p><strong>Routage avancé :</strong></p>
  <ul style="margin: 10px 0;">
    <li><strong>Plan de masse</strong> — couche Bottom remplie de GND pour réduire le bruit</li>
    <li><strong>Vias</strong> — trous métallisés pour connecter les couches Top et Bottom</li>
    <li><strong>Largeur des pistes</strong> — 0.25mm pour signaux, 0.5mm pour alimentation</li>
    <li><strong>DRC</strong> — vérification des règles de conception (clearance, drill)</li>
  </ul>
  <p><strong>Fichiers de fabrication :</strong></p>
  <ul style="margin: 10px 0;">
    <li><strong>Gerber</strong> — format standard pour chaque couche (.GTL, .GBL, .GTS, etc.)</li>
    <li><strong>Fichier de perçage</strong> — .DRL au format Excellon</li>
    <li><strong>BOM</strong> — liste des composants à acheter</li>
  </ul>
  <p><strong>Labo 6 :</strong> finaliser le routage, passer le DRC sans erreur, générer les Gerbers et les vérifier dans un visualiseur.</p>
  <p><strong>À faire pour la semaine prochaine :</strong></p>
  <ul style="margin: 10px 0;">
    <li>Finaliser tous les livrables du projet de mi-session</li>
    <li>Vérifier les Gerbers dans un visualiseur en ligne</li>
    <li>Compléter la documentation technique</li>
    <li>Préparer la remise du projet (30%)</li>
  </ul>
</div>

## Semaine 7

<div style="font-family: 'Segoe UI', system-ui, sans-serif; line-height: 1.6;">
  <p><strong>Remise du projet de mi-session</strong> (30%) et introduction à la technologie <strong>LoRa</strong> (Long Range) pour la communication longue portée sans infrastructure.</p>
  <p><strong>Technologie LoRa :</strong></p>
  <ul style="margin: 10px 0;">
    <li><strong>Modulation CSS</strong> — Chirp Spread Spectrum, résistante aux interférences</li>
    <li><strong>Portée</strong> — 2 à 15 km selon les paramètres et l'environnement</li>
    <li><strong>Bande 915 MHz</strong> — ISM sans licence en Amérique du Nord</li>
    <li><strong>Compromis</strong> — longue portée mais faible débit (0.3 à 50 kbps)</li>
  </ul>
  <p><strong>Meshtastic :</strong></p>
  <ul style="margin: 10px 0;">
    <li><strong>Firmware open source</strong> — transforme des radios LoRa en réseau mesh</li>
    <li><strong>Réseau décentralisé</strong> — aucune infrastructure requise</li>
    <li><strong>Chiffrement AES-256</strong> — communication sécurisée</li>
    <li><strong>T-Beam SUPREME</strong> — ESP32-S3 + LoRa SX1262 + GPS</li>
  </ul>
  <p><strong>Labo 7 :</strong> remettre le projet PCB, flasher le firmware Meshtastic sur T-Beam, et effectuer un premier test de communication.</p>
  <p><strong>À faire pour la semaine prochaine :</strong></p>
  <ul style="margin: 10px 0;">
    <li>Se familiariser avec l'application Meshtastic (Android/iOS)</li>
    <li>Lire la documentation sur les paramètres radio LoRa (SF, BW, CR)</li>
    <li>Préparer des questions sur la configuration mesh</li>
  </ul>
</div>

## Semaine 8

<div style="font-family: 'Segoe UI', system-ui, sans-serif; line-height: 1.6;">
  <p>Configuration avancée des <strong>paramètres radio LoRa</strong> et mise en place d'un <strong>réseau mesh</strong> multi-noeuds avec Meshtastic.</p>
  <p><strong>Paramètres radio :</strong></p>
  <ul style="margin: 10px 0;">
    <li><strong>Spreading Factor (SF7-SF12)</strong> — plus élevé = plus de portée, moins de débit</li>
    <li><strong>Bandwidth (125-500 kHz)</strong> — plus large = plus de débit, moins de portée</li>
    <li><strong>Coding Rate (4/5 à 4/8)</strong> — redondance pour corriger les erreurs</li>
    <li><strong>Presets Meshtastic</strong> — SHORT_FAST, MEDIUM_FAST, LONG_SLOW, etc.</li>
  </ul>
  <p><strong>Architecture mesh :</strong></p>
  <ul style="margin: 10px 0;">
    <li><strong>Rôles des noeuds</strong> — Client, Router, Router_Client</li>
    <li><strong>Multi-hop</strong> — les messages sont relayés automatiquement</li>
    <li><strong>Métriques</strong> — RSSI (force du signal) et SNR (rapport signal/bruit)</li>
  </ul>
  <p><strong>Labo 8 :</strong> configurer un réseau mesh en équipe, tester différents presets, mesurer les performances et documenter les résultats.</p>
  <p><strong>À faire pour la semaine prochaine :</strong></p>
  <ul style="margin: 10px 0;">
    <li>Documenter les résultats des tests de portée</li>
    <li>Lire la documentation sur la configuration gateway MQTT</li>
    <li>Préparer l'équipement pour les tests terrain</li>
  </ul>
</div>

## Semaine 9

<div style="font-family: 'Segoe UI', system-ui, sans-serif; line-height: 1.6;">
  <p>Configuration d'une <strong>gateway WiFi/MQTT</strong> pour connecter le réseau LoRa mesh à Internet, et <strong>tests terrain</strong> pour évaluer la couverture.</p>
  <p><strong>Gateway Meshtastic :</strong></p>
  <ul style="margin: 10px 0;">
    <li><strong>Pont LoRa ↔ MQTT</strong> — le T-Beam connecté au WiFi publie les messages sur le broker</li>
    <li><strong>Topics standardisés</strong> — msh/[region]/[channel]/json pour les messages</li>
    <li><strong>Bidirectionnel</strong> — envoyer des commandes depuis Python vers le mesh</li>
  </ul>
  <p><strong>Architecture unifiée :</strong></p>
  <ul style="margin: 10px 0;">
    <li><strong>LTE + LoRa</strong> — redondance des communications</li>
    <li><strong>Topics MQTT unifiés</strong> — iot/[projet]/[source]/[type]/[data]</li>
  </ul>
  <p><strong>Tests terrain :</strong></p>
  <ul style="margin: 10px 0;">
    <li><strong>Méthodologie</strong> — point fixe de référence, points mobiles de test</li>
    <li><strong>Données collectées</strong> — distance, RSSI, SNR, taux de succès</li>
    <li><strong>Cartographie</strong> — visualisation de la couverture du réseau</li>
  </ul>
  <p><strong>Labo 9 :</strong> configurer la gateway WiFi/MQTT, effectuer une sortie terrain et créer une carte de couverture.</p>
  <p><strong>À faire pour la semaine prochaine :</strong></p>
  <ul style="margin: 10px 0;">
    <li>Finaliser la carte de couverture</li>
    <li>Préparer le poste de soudure (fer, étain, flux)</li>
    <li>Revoir les techniques de soudure THT</li>
  </ul>
</div>

## Semaine 10

<div style="font-family: 'Segoe UI', system-ui, sans-serif; line-height: 1.6;">
  <p><strong>Réception des PCB</strong> fabriqués et atelier de <strong>soudure</strong> pour assembler le shield LilyGO. Tests électriques pour valider le fonctionnement.</p>
  <p><strong>Inspection des PCB :</strong></p>
  <ul style="margin: 10px 0;">
    <li><strong>Qualité visuelle</strong> — masque uniforme, pistes continues, sérigraphie lisible</li>
    <li><strong>Défauts courants</strong> — piste coupée, court-circuit, trou décentré</li>
    <li><strong>Test de continuité</strong> — vérifier GND et VCC avant d'alimenter</li>
  </ul>
  <p><strong>Techniques de soudure :</strong></p>
  <ul style="margin: 10px 0;">
    <li><strong>Ordre d'assemblage</strong> — composants bas d'abord (résistances), puis hauts (headers)</li>
    <li><strong>Polarité</strong> — vérifier LEDs (anode/cathode) et condensateurs</li>
    <li><strong>Temps de contact</strong> — 2-3 secondes par soudure, éviter la surchauffe</li>
  </ul>
  <p><strong>Tests fonctionnels :</strong></p>
  <ul style="margin: 10px 0;">
    <li><strong>Alimentation</strong> — vérifier les tensions 3.3V</li>
    <li><strong>Périphériques</strong> — LEDs, boutons, potentiomètres, accéléromètre (scan I2C)</li>
  </ul>
  <p><strong>Labo 10 :</strong> inspecter le PCB, souder tous les composants et valider le fonctionnement avec des tests progressifs.</p>
  <p><strong>À faire pour la semaine prochaine :</strong></p>
  <ul style="margin: 10px 0;">
    <li>Finaliser la soudure et les tests du PCB</li>
    <li>Créer un compte API chez un fournisseur LLM (OpenAI, Anthropic ou Google)</li>
    <li>Installer les bibliothèques Python nécessaires (openai, anthropic, pydantic)</li>
  </ul>
</div>

## Semaine 11

<div style="font-family: 'Segoe UI', system-ui, sans-serif; line-height: 1.6;">
  <p>Intégration des <strong>API LLM</strong> (Large Language Models) dans les systèmes IoT pour l'analyse intelligente des données et l'automatisation des décisions.</p>
  <p><strong>LLM pour l'IoT :</strong></p>
  <ul style="margin: 10px 0;">
    <li><strong>Analyse intelligente</strong> — interpréter les données capteurs, détecter les anomalies</li>
    <li><strong>Fournisseurs</strong> — OpenAI (GPT), Anthropic (Claude), Google (Gemini)</li>
    <li><strong>Prompt engineering</strong> — structurer les requêtes pour des réponses exploitables</li>
  </ul>
  <p><strong>Pipeline de données :</strong></p>
  <ul style="margin: 10px 0;">
    <li><strong>Ingestion</strong> — réception des messages MQTT</li>
    <li><strong>Validation</strong> — vérifier le format et les plages de valeurs (Pydantic)</li>
    <li><strong>Transformation</strong> — enrichir avec contexte et historique</li>
    <li><strong>Analyse LLM</strong> — classification, détection d'anomalies, génération de rapports</li>
    <li><strong>Actions</strong> — alertes, commandes aux actionneurs, logging</li>
  </ul>
  <p><strong>Labo 11 :</strong> créer un compte API, configurer le pipeline Python et intégrer l'analyse LLM aux données du shield.</p>
  <p><strong>À faire pour la semaine prochaine :</strong></p>
  <ul style="margin: 10px 0;">
    <li>Tester le pipeline LLM avec des données réelles</li>
    <li>Documenter les prompts utilisés</li>
    <li>Identifier les cas d'erreur potentiels</li>
  </ul>
</div>

## Semaine 12

<div style="font-family: 'Segoe UI', system-ui, sans-serif; line-height: 1.6;">
  <p><strong>Fiabilité</strong> et <strong>sécurité</strong> des automatisations IoT : passer d'un prototype à un système de production robuste.</p>
  <p><strong>Gestion des erreurs :</strong></p>
  <ul style="margin: 10px 0;">
    <li><strong>Types d'erreurs</strong> — réseau, API, données invalides, matériel</li>
    <li><strong>Retry avec backoff</strong> — délai exponentiel entre les tentatives</li>
    <li><strong>Circuit Breaker</strong> — protection contre les cascades d'erreurs</li>
    <li><strong>Fallback</strong> — analyse basée sur règles si le LLM est indisponible</li>
  </ul>
  <p><strong>Observabilité :</strong></p>
  <ul style="margin: 10px 0;">
    <li><strong>Logging structuré</strong> — format JSON pour analyse automatisée</li>
    <li><strong>Métriques</strong> — messages/min, latence, taux d'erreur</li>
    <li><strong>Alertes proactives</strong> — notification avant que l'utilisateur ne remarque</li>
  </ul>
  <p><strong>Sécurité :</strong></p>
  <ul style="margin: 10px 0;">
    <li><strong>Gestion des secrets</strong> — fichier .env, variables d'environnement</li>
    <li><strong>Validation des entrées</strong> — ne jamais faire confiance aux données externes</li>
    <li><strong>Principe du moindre privilège</strong> — permissions minimales pour chaque composant</li>
  </ul>
  <p><strong>Labo 12 :</strong> ajouter la gestion d'erreurs, migrer les secrets vers .env et implémenter le logging structuré.</p>
  <p><strong>À faire pour la semaine prochaine :</strong></p>
  <ul style="margin: 10px 0;">
    <li>Finaliser la gestion d'erreurs et le logging</li>
    <li>Préparer tous les composants pour l'intégration finale</li>
    <li>Vérifier que chaque sous-système fonctionne individuellement</li>
  </ul>
</div>

## Semaine 13

<div style="font-family: 'Segoe UI', system-ui, sans-serif; line-height: 1.6;">
  <p><strong>Intégration finale</strong> du projet : assembler tous les composants (PCB, LoRa, MQTT, LLM) en un système IoT complet et fonctionnel.</p>
  <p><strong>Composants à intégrer :</strong></p>
  <ul style="margin: 10px 0;">
    <li><strong>Matériel</strong> — shield PCB soudé, LilyGO A7670G, T-Beam SUPREME</li>
    <li><strong>Communication</strong> — MQTT broker, gateway LoRa, WiFi/LTE</li>
    <li><strong>Logiciel</strong> — firmware Arduino, pipeline Python, intégration LLM</li>
  </ul>
  <p><strong>Tests de performance :</strong></p>
  <ul style="margin: 10px 0;">
    <li><strong>Latence bout-en-bout</strong> — cible inférieure à 2 secondes</li>
    <li><strong>Taux de livraison</strong> — cible supérieure à 99%</li>
    <li><strong>Tests de charge</strong> — simuler des conditions réelles</li>
  </ul>
  <p><strong>Autonomie énergétique :</strong></p>
  <ul style="margin: 10px 0;">
    <li><strong>Profil de consommation</strong> — mesurer chaque composant (ESP32, LTE, LoRa)</li>
    <li><strong>Deep Sleep</strong> — mode veille à 10 µA entre les transmissions</li>
    <li><strong>Calcul d'autonomie</strong> — capacité batterie / consommation moyenne</li>
  </ul>
  <p><strong>Labo 13 :</strong> assembler le système complet, effectuer les tests de performance et mesurer l'autonomie.</p>
  <p><strong>À faire pour la semaine prochaine :</strong></p>
  <ul style="margin: 10px 0;">
    <li>Corriger les problèmes identifiés lors des tests</li>
    <li>Documenter les résultats de performance</li>
    <li>Commencer la préparation des diapositives de présentation</li>
  </ul>
</div>

## Semaine 14

<div style="font-family: 'Segoe UI', system-ui, sans-serif; line-height: 1.6;">
  <p><strong>Validation finale</strong> du projet et <strong>préparation de la présentation</strong>. Derniers ajustements et documentation complète.</p>
  <p><strong>Tests d'acceptation :</strong></p>
  <ul style="margin: 10px 0;">
    <li><strong>Checklist système</strong> — démarrage, capteurs, actionneurs, MQTT, LoRa, LLM</li>
    <li><strong>Récupération après panne</strong> — tester les scénarios d'erreur</li>
    <li><strong>Documentation des limitations</strong> — être honnête sur les limites du système</li>
  </ul>
  <p><strong>Préparation de la présentation :</strong></p>
  <ul style="margin: 10px 0;">
    <li><strong>Structure (15 min)</strong> — introduction, architecture, démonstration, résultats, questions</li>
    <li><strong>Plan B</strong> — vidéo de backup, screenshots, mode dégradé</li>
    <li><strong>Réponses aux questions</strong> — techniques de gestion et phrases utiles</li>
  </ul>
  <p><strong>Rétrospective :</strong></p>
  <ul style="margin: 10px 0;">
    <li><strong>Points forts</strong> — valoriser les réussites avec des métriques</li>
    <li><strong>Leçons apprises</strong> — défis rencontrés et solutions trouvées</li>
    <li><strong>Portfolio professionnel</strong> — valoriser le projet pour CV et entretiens</li>
  </ul>
  <p><strong>Labo 14 :</strong> finaliser les corrections, compléter la documentation, préparer les diapositives et répéter la présentation.</p>
  <p><strong>À faire pour la semaine prochaine :</strong></p>
  <ul style="margin: 10px 0;">
    <li>Finaliser les diapositives et la vidéo de démonstration</li>
    <li>Répéter la présentation (10-12 min + questions)</li>
    <li>Préparer tous les livrables pour la remise finale</li>
    <li>S'assurer que le système fonctionne pour la démonstration live</li>
  </ul>
</div>

## Semaine 15

<div style="font-family: 'Segoe UI', system-ui, sans-serif; line-height: 1.6;">
  <p><strong>Présentations finales</strong> et remise du projet IoT complet. Évaluation sommative couvrant toutes les compétences développées au cours de la session.</p>
  <p><strong>Évaluation du projet final (30%) :</strong></p>
  <ul style="margin: 10px 0;">
    <li><strong>Capacité 1 (10%)</strong> — PCB fonctionnel, code Arduino/Python, interface utilisateur</li>
    <li><strong>Capacité 2 (20%)</strong> — configuration Meshtastic, intégration MQTT, tests de performance, qualité de présentation</li>
  </ul>
  <p><strong>Déroulement des présentations :</strong></p>
  <ul style="margin: 10px 0;">
    <li><strong>Format</strong> — 15 minutes par étudiant (10-12 min présentation + 3-5 min questions)</li>
    <li><strong>Contenu</strong> — introduction, architecture, démonstration live, résultats, défis et solutions</li>
    <li><strong>Livrables</strong> — code sur GitHub, documentation, fichiers KiCad, vidéo de démonstration</li>
  </ul>
  <p><strong>Compétences acquises :</strong></p>
  <ul style="margin: 10px 0;">
    <li><strong>Matériel</strong> — conception PCB (KiCad), soudure, microcontrôleurs ESP32</li>
    <li><strong>Communication</strong> — MQTT, LoRa, WiFi, LTE, réseau mesh</li>
    <li><strong>Logiciel</strong> — Arduino, Python, intégration LLM, Git/GitHub</li>
  </ul>
  <p><strong>Félicitations!</strong> Vous êtes maintenant des développeurs IoT!</p>
</div>
