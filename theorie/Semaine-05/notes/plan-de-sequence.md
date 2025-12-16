# Plan de séquence - Semaine 05

## Titre
Projet de mi-session - Shield LilyGO A7670G

## Objectifs de la semaine
- Réaliser un prototype breadboard fonctionnel et validé
- Programmer le code de test pour chaque composant
- Intégrer l'interface avec l'écran tactile du Raspberry Pi
- Documenter le projet avec Markdown sur GitHub

## Contenu théorique

### Communication I2C et accéléromètre
- Protocole I2C : adressage, SDA/SCL, vitesse
- Accéléromètre : axes X/Y/Z, registres, calibration
- Bibliothèques Arduino pour I2C
- Lecture et interprétation des données d'accélération

### Conversion analogique-numérique (ADC)
- Principes de l'ADC sur ESP32
- Résolution et plage de tension
- Lecture des potentiomètres
- Filtrage et lissage des valeurs

### Architecture applicative
- Flux de données : capteurs → LilyGO → MQTT → RPi
- Structure des topics MQTT pour le projet
- Interface utilisateur sur écran tactile (GUI terminal Python)
- Scénarios d'interaction (jeu, dashboard, contrôle)

### Documentation technique avec GitHub
- Structure d'un README.md efficace
- Syntaxe Markdown : titres, listes, code, tableaux, images
- Organisation de la documentation dans un dépôt
- Bonnes pratiques : badges, table des matières, exemples
- Utilisation des Issues et Projects pour le suivi

## Activités

### Théorie (2h)
- Communication I2C et lecture de l'accéléromètre
- ADC et lecture des potentiomètres
- Interface tactile GUI terminal sur RPi
- Documentation Markdown et GitHub
- Présentation des requis et assignations

### Laboratoire (3h)
- Montage breadboard complet
  - LEDs, boutons, potentiomètres, accéléromètre
- Code de test pour chaque composant
- Intégration MQTT avec le broker existant
- Ébauche de l'interface GUI terminal sur RPi
- Création du README.md du projet sur GitHub

## Travaux hors classe
- Finaliser le prototype breadboard
- Développer l'interface GUI terminal sur RPi
- Compléter la documentation GitHub (README, schémas)

## Évaluation
- Suivi de progression (formatif)
- Démonstration du prototype fonctionnel

## Ressources
- Fiche technique de l'accéléromètre
- Documentation ESP32 ADC
- Exemples de code I2C Arduino
- Code de l'interface GUI terminal (Labo 1-2)
- Guide Markdown de GitHub

## Notes pour l'enseignant
- Vérifier les assignations de composants par étudiant
- S'assurer que les accéléromètres I2C sont fonctionnels
- Avoir des exemples de code prêts pour le dépannage
- Accompagner le développement de l'interface RPi
