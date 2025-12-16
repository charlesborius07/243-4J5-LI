# Plan de séquence - Semaine 03

## Titre
Intégration complète et montage breadboard

## Objectifs de la semaine
- Réaliser une chaîne d'acquisition complète (capteurs → serveur)
- Intégrer des boutons et LEDs dans le système
- Finaliser le montage breadboard pour l'interface capteurs/actionneurs
- Effectuer des tests de robustesse du système
- Documenter le processus technique

## Contenu théorique

### Chaîne d'acquisition IdO
- Architecture complète: capteurs → microcontrôleur → serveur
- Flux de données bidirectionnel
- Synchronisation et timing des données
- Gestion des erreurs et reconnexions

### Interfaçage matériel
- GPIO et entrées/sorties numériques
- Boutons: debouncing et gestion des événements
- LEDs: contrôle et indicateurs d'état
- Bonnes pratiques de câblage sur breadboard

### Robustesse et fiabilité
- Tests de stress et de charge
- Gestion des déconnexions réseau
- Reprise après erreur
- Journalisation et débogage

## Activités

### Théorie (2h)
- Révision de l'architecture complète du système
- Techniques d'interfaçage matériel
- Stratégies de test et de validation

### Laboratoire (3h)
- Montage final de la chaîne d'acquisition
- Intégration des boutons et LEDs sur breadboard
- Communication bidirectionnelle Pi 5 ↔ LilyGO A7670G via MQTT
- Tests de robustesse (déconnexion/reconnexion)
- Documentation du montage et du code

## Travaux hors classe
- Finaliser le montage breadboard
- Compléter la documentation technique de base
- Préparer la démonstration pour l'évaluation

## Évaluation
- **Évaluation sommative:** Laboratoire capteurs et Python (15%)
  - Mise en œuvre d'une chaîne d'acquisition sur Raspberry Pi 5/LilyGO
  - Critères: fonctionnalité, code, documentation

## Ressources
- Documentation GPIO Raspberry Pi
- Documentation GPIO ESP32
- Guides de câblage électronique
- Exemples de code d'intégration

## Notes pour l'enseignant
- Prévoir du temps pour le dépannage individuel
- Avoir des composants de rechange (boutons, LEDs, résistances)
- Grille d'évaluation détaillée pour l'évaluation sommative
- S'assurer que les critères d'évaluation sont clairs pour les étudiants
