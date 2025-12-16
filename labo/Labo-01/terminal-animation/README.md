# Animations Terminal pour Raspberry Pi

Collection d'animations amusantes utilisant les fonctions graphiques du terminal.

## Animations disponibles

### 1. Matrix Rain (`matrix_animation.py`)
Animation style "Matrix" avec des caractères japonais qui tombent en cascade.
- Caractères verts qui tombent façon code Matrix
- Effet de traînée lumineuse
- **Contrôles**: `q` pour quitter

### 2. Rainbow Rain (`rainbow_rain.py`)
Pluie colorée arc-en-ciel avec des gouttes animées.
- Gouttes de pluie en 6 couleurs différentes
- Effet de traînée pour chaque goutte
- **Contrôles**: `q` pour quitter

### 3. Starfield (`starfield.py`)
Champ d'étoiles 3D simulant un voyage dans l'espace.
- Étoiles en perspective 3D
- Vitesse ajustable
- **Contrôles**:
  - `↑/↓` : Augmenter/diminuer la vitesse
  - `q` : Quitter

## Installation

Aucune installation supplémentaire nécessaire ! Python3 et le module `curses` sont préinstallés sur Raspberry Pi OS.

## Utilisation

Rendre les scripts exécutables (première fois seulement):
```bash
chmod +x *.py
```

Lancer une animation:
```bash
./matrix_animation.py
# ou
python3 matrix_animation.py
```

## Prérequis

- Python 3
- Module `curses` (inclus par défaut)
- Terminal avec support des couleurs

## Conseils

- Agrandir le terminal en plein écran pour une meilleure expérience
- Fonctionne dans SSH avec n'importe quel émulateur de terminal
- Appuyer sur `q` pour quitter n'importe quelle animation

## Notes techniques

Toutes les animations utilisent:
- `curses` pour le contrôle du terminal
- Fréquence de rafraîchissement: ~20 FPS
- Support des couleurs ANSI
- Mode non-bloquant pour les entrées clavier
