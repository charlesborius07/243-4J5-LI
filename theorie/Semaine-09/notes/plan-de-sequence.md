# Plan de séquence - Semaine 09

## Titre
LilyGO T-Beam SUPREME - Gateway WiFi autonome + Tests terrain

## Objectifs de la semaine
- Configurer le T-Beam comme gateway WiFi vers MQTT
- Réaliser des tests de portée sur le terrain
- Cartographier la couverture avec les données GPS
- Intégrer l'architecture complète (LoRa + LTE + MQTT)

## Contenu théorique

### Gateway WiFi/MQTT
- Configuration du T-Beam comme pont LoRa → WiFi → MQTT
- Connexion au broker Mosquitto existant
- Flux bidirectionnels LoRa ↔ MQTT
- Gestion des messages et topics

### Tests terrain
- Méthodologie de test de portée
- Utilisation des données GPS
- Cartographie de couverture
- Facteurs influençant la portée (terrain, obstacles)

### Architecture unifiée
- Intégration LoRa + LTE + MQTT
- Redondance et basculement
- Scénarios d'utilisation hybrides
- Dashboard de monitoring (Node-RED optionnel)

## Activités

### Théorie (2h)
- Configuration de la gateway WiFi
- Méthodologie des tests terrain
- Intégration de l'architecture complète

### Laboratoire (3h)
- Configuration du T-Beam comme gateway
- Tests bidirectionnels LoRa ↔ MQTT
- Sortie terrain pour tests de portée
- Collecte des données GPS
- Création d'une carte de couverture

## Travaux hors classe
- Finalisation de la gateway WiFi
- Analyse des données de terrain
- Documentation des tests et résultats

## Évaluation
- **Évaluation sommative:** TP Intégration LLM et automatisation (20%)
  - Flux capteurs → traitement → action (15% Capacité 1)
  - Configuration et communication (5% Capacité 2)

## Ressources
- Documentation Meshtastic - Gateway
- Outils de cartographie (QGIS, Google Earth)
- Guide d'intégration MQTT

## Notes pour l'enseignant
- Planifier la sortie terrain (météo, lieu)
- Préparer les équipements de test
- Avoir un plan B en cas de mauvais temps
