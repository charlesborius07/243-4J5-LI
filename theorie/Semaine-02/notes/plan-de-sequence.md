# Plan de séquence - Semaine 02

## Titre
Communication MQTT sans fil (WiFi et LTE)

## Objectifs de la semaine
- Configurer une communication MQTT sécurisée avec WSS/TLS
- Établir une communication MQTT via WiFi avec le LilyGO A7670G
- Établir une communication MQTT via LTE avec le LilyGO A7670G
- Développer une interface tactile Python pour le contrôle de LEDs
- Appliquer les bonnes pratiques de validation des données

## Contenu théorique

### Protocole MQTT approfondi
- Architecture publish/subscribe
- Topics et hiérarchie des sujets
- Niveaux de qualité de service (QoS 0, 1, 2)
- Messages retenus (retained messages)
- Last Will and Testament (LWT)

### Sécurisation des communications MQTT
- WebSocket Secure (WSS) et TLS
- Certificats SSL/TLS
- Authentification par utilisateur/mot de passe
- Bonnes pratiques de sécurité

### Communication sans fil
- Comparaison WiFi vs LTE pour l'IdO
- Avantages et inconvénients de chaque technologie
- Gestion de la connectivité et des déconnexions

## Activités

### Théorie (2h)
- Approfondissement du protocole MQTT
- Sécurisation des communications (WSS/TLS)
- Stratégies de communication WiFi et LTE

### Laboratoire (3h)
- Configuration MQTT avec WSS/TLS sur Mosquitto
- Communication MQTT via WiFi avec le LilyGO A7670G
- Communication MQTT via LTE avec le LilyGO A7670G
- Développement d'une interface tactile Python
- Tests d'envoi/réception de messages bidirectionnels
- Implémentation des bonnes pratiques de validation

## Travaux hors classe
- Finaliser la communication MQTT bidirectionnelle
- Mise à jour du cahier de laboratoire
- Documentation technique initiale

## Évaluation
- Mini-vérifications en laboratoire (formatif)
- Validation du fonctionnement MQTT bidirectionnel

## Ressources
- Spécification MQTT 5.0
- Documentation Paho MQTT (Python)
- Documentation PubSubClient (Arduino)
- Guide de configuration TLS pour Mosquitto

## Notes pour l'enseignant
- Préparer les certificats TLS à l'avance si nécessaire
- Vérifier la disponibilité des cartes SIM LTE
- Prévoir des scénarios de dépannage réseau
