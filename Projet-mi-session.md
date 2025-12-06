# Projet de mi-session — Shield IoT pour LilyGO A7670G

> 🎯 Objectif : réaliser le projet final dès la mi-session : concevoir et documenter un **shield PCB complet** pour le LilyGO A7670G, du prototype breadboard jusqu'aux fichiers de fabrication.

## Cahier des charges

Concevez un shield qui intègre les blocs matériels suivants :

- **LEDs d'indication** : alimentation (verte), statut réseau (bleue), alerte (rouge), GPS fix (jaune) avec résistances adaptées.
- **Boutons tactiles** : RESET, MODE, USER avec circuits anti-rebond matériels et pull-up/pull-down appropriés.
- **Accéléromètre (MPU6050 ou ADXL345)** : bus I2C stable (résistances de tirage, filtrage), interruptions matérielles si nécessaire.
- **Interface audio** : microphone MEMS (I2S) optionnel, speaker/buzzer avec amplification (PAM8403 ou similaire).
- **Alimentation et gestion d'énergie** : connecteur batterie LiPo, circuit de charge (ex. TP4056), régulateur 3.3V, découplage et monitoring de tension.
- **Connectivité et testabilité** : headers GPIO pour extensions, connecteur I2C externe, UART de debug, pads de test pour signaux critiques.
- **Compatibilité LilyGO** : brochage et empreintes alignés sur le A7670G, contraintes mécaniques respectées (alignement des headers, dégagement antennes).

## Phases et livrables attendus

1. **Prototype breadboard**
   - Câbler tous les composants du futur shield sur plaquette (LEDs, boutons, accéléromètre, audio, alimentation).
   - Fournir un sketch Arduino de test validant chaque bloc (LEDs, boutons, accéléromètre, audio) et une courte vidéo ou capture série montrant les essais.
2. **Schéma (Altium)**
   - Schémas Altium propres, annotés et regroupés par fonction (alimentation, E/S, capteurs, audio).
   - ERC/compilations sans erreurs bloquantes, valeurs et références complètes, BOM initiale exportée.
3. **Layout PCB (Altium)**
   - Routage 2 couches avec plans de masse, largeurs de pistes adaptées, dégagements RF autour des antennes LilyGO.
   - DRC sans erreurs, placements cohérents (boutons accessibles, LEDs visibles, connecteurs alignés).
   - Génération des Gerbers + drill + BOM prêts pour fabrication.
4. **Documentation**
   - README synthétique : description du shield, schémas/rendus 3D, contraintes mécaniques, instructions de fabrication et de test.
   - Journal de tests du prototype breadboard (photos annotées + notes de mesure) et liste des points à surveiller pour l'assemblage.

## Structure recommandée

```bash
~/243-4J5-LI/projet-mi-session/
├── altium/                    # Schéma et PCB
│   ├── shield.PrjPcb
│   ├── shield.SchDoc
│   └── shield.PcbDoc
├── prototype/                 # Code et preuves de test breadboard
│   ├── prototype_shield.ino
│   ├── photos/
│   └── notes-tests.md
├── fabrication/               # Sorties Gerber/Drill/BOM
│   ├── gerbers/
│   ├── bom.csv
│   └── fabrication-readme.md
└── README.md                  # Vue d'ensemble et instructions
```

## Critères d'évaluation

- **Conception matérielle (40%)** : schéma complet, ERC/DRC propres, routage cohérent, choix de composants justifiés.
- **Prototype et validation (30%)** : montage breadboard fonctionnel, sketch de test couvrant chaque bloc, preuves (captures, mesures).
- **Documentation (30%)** : README clair, rendus/schémas inclus, instructions de fabrication/test, risques et mitigations listés.

## Livraison

1. Pousser le projet complet dans `projet-mi-session/` :
   ```bash
   cd ~/243-4J5-LI/projet-mi-session
   git add .
   git commit -m "Projet mi-session : shield LilyGO A7670G"
   git push origin prenom-nom/projet-mi-session
   ```
2. Le README doit inclure :
   - Résumé des fonctionnalités du shield et contraintes mécaniques.
   - Captures du schéma, aperçus 3D/2D du PCB, et liste des composants clés.
   - Procédure de test du prototype breadboard (commande, schémas de câblage, résultats).
   - Détails de fabrication (Gerbers/BOM, options de stackup, notes pour l'assembleur).
   - Points de vigilance connus et correctifs envisagés.

## Ressources utiles
- **Logiciel** : Altium Designer (schéma/PCB), calculateur de largeur de piste, convertisseur STEP/3D si nécessaire.
- **Composants** : MPU6050/ADXL345, INMP441/SPH0645, PAM8403, TP4056, régulateur 3.3V, LEDs/boutons/headers.
- **Fabricants PCB** : JLCPCB, PCBWay, OSH Park (respecter leurs règles de design pour le DRC).
