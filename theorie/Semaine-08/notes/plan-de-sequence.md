# Plan de séquence - Semaine 08

## Titre
LilyGO T-Beam SUPREME - Configuration avancée et réseau mesh

## Objectifs de la semaine
- Maîtriser les paramètres radio LoRa (SF/BW/CR)
- Configurer les rôles mesh (Router, Router-Critical)
- Mettre en place un réseau multi-noeuds (4-6 noeuds)
- Optimiser le compromis portée/consommation

## Contenu théorique

### Paramètres radio LoRa
- Spreading Factor (SF7 à SF12)
- Bandwidth (BW): 125kHz, 250kHz, 500kHz
- Coding Rate (CR): 4/5, 4/6, 4/7, 4/8
- Impact sur portée, débit et consommation

### Architecture mesh Meshtastic
- Types de noeuds : Client, Router, Router-Critical
- Algorithme de routage
- Gestion des messages et accusés de réception
- Optimisation de la topologie du réseau

### Tests et mesures
- Mesure de RSSI et SNR
- Outils de diagnostic Meshtastic
- Cartographie de couverture
- Analyse des performances

## Activités

### Théorie (2h)
- Approfondissement des paramètres LoRa
- Configuration des rôles mesh
- Stratégies d'optimisation du réseau

### Laboratoire (3h)
- Configuration d'un réseau de 4-6 noeuds
- Tests comparatifs SF7/SF9/SF12
- Configuration des rôles (Router, Client)
- Mise en place d'un relais fixe
- Documentation des performances

## Travaux hors classe
- Itérations sur les tests de Spreading Factor
- Documentation comparative des résultats
- Planification de l'installation du relais permanent

## Évaluation
- Revue de progression (formatif)
- Vérification du réseau mesh fonctionnel

## Ressources
- Documentation Meshtastic - Configuration radio
- Calculateur de portée LoRa
- Guide d'optimisation des réseaux mesh

## Notes pour l'enseignant
- Préparer un environnement de test contrôlé
- Avoir des batteries chargées pour les tests mobiles
- Documenter les configurations de référence
