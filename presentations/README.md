# Présentations Slidev - 243-4J5-LI

Présentations interactives pour le cours Objets connectés.

## Installation

```bash
cd presentations
npm install
```

## Utilisation

### Mode développement (avec rechargement automatique)

```bash
# Semaine 1 - Introduction et plan de cours
npm run semaine1

# Semaine 2 - MQTT et communication sans fil
npm run semaine2

# Semaine 3 - LTE et sécurité
npm run semaine3
```

### Export PDF

```bash
npm run export:semaine1
npm run export:semaine2
npm run export:semaine3
```

### Build pour déploiement web

```bash
npm run build:all
```

Les fichiers seront générés dans `../docs/semaineX/`.

## Structure des présentations

### Semaine 1 - Introduction et mise en contexte
- Qu'est-ce que l'IoT?
- Présentation du plan de cours
- Compétences et évaluations
- Introduction au Laboratoire 1
- Architecture du système de développement distant

### Semaine 2 - Protocole MQTT
- Introduction à MQTT (Publish/Subscribe)
- Topics et wildcards
- QoS (Quality of Service)
- Broker Mosquitto
- WiFi Enterprise (WPA-EAP)
- Introduction au Laboratoire 2

### Semaine 3 - Communication LTE et sécurité
- Communication cellulaire LTE
- Commandes AT et configuration modem
- Sécurité TLS/SSL
- Gestion des secrets
- Gestion des erreurs et reconnexion
- Préparation à l'évaluation

## Raccourcis clavier Slidev

| Touche | Action |
|--------|--------|
| `Espace` / `→` | Diapositive suivante |
| `←` | Diapositive précédente |
| `o` | Vue d'ensemble |
| `d` | Mode sombre |
| `f` | Plein écran |
| `g` | Aller à une diapositive |

## Personnalisation

Les présentations utilisent le thème `seriph`. Pour modifier l'apparence, éditez le frontmatter de chaque fichier `slides.md`.

```yaml
---
theme: seriph
background: url-image
---
```
