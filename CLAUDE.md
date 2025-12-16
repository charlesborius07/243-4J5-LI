Guide du projet - Cours techniques
Ce projet contient du matériel pédagogique pour des cours techniques : présentations théoriques (Slidev) et documents de laboratoire.

Structure du projet
/
├── theorie/
│   └── Semaine-XX/
│       ├── notes/           # Plans de séquence, notes de cours
│       ├── diaporama/       # Présentation Slidev
│       │   ├── slides.md
│       │   ├── components/  # Composants Vue interactifs
│       │   └── public/      # Images et ressources
│       └── exercices/       # Exercices et problèmes
│
├── labo/
│   └── Labo-XX/
│       └── Laboratoire XX - *.md
│
├── Plan de cours - *.md     # Plan de cours global
└── CLAUDE.md
Présentations Slidev (diaporama/)
Gestionnaire de paquets
Utiliser **pnpm** pour toutes les opérations liées aux diaporamas Slidev :
```bash
# Installation des dépendances
pnpm install

# Lancement du serveur de développement
pnpm dev

# Build pour production
pnpm build
```

Composants Vue interactifs
Les composants interactifs doivent être optimisés pour l'affichage en présentation (résolution 1920x1080, ratio 16:9).

Dimensions et mise en page
Conteneur principal : Utiliser flexbox avec gap: 8-10px
Graphiques SVG : Dimensions recommandées 300-450px, jamais plus de 500px de hauteur
Panneaux de contrôle : Largeur fixe 140-200px pour éviter les débordements
Padding global : 4-8px pour maximiser l'espace utile
Tailles de police dans les SVG
Les tailles doivent être proportionnelles au viewBox, pas à l'affichage CSS :

Élément	Taille recommandée
Titres principaux	4-5
Labels d'axes	5-6
Valeurs numériques	3.5-5
Annotations	3-4
Éléments SVG
Traits d'axes : stroke-width="1"
Vecteurs/lignes importantes : stroke-width="1.5-2"
Grilles : stroke-width="0.5", couleur #e8e8e8
Points interactifs : rayon r="4-6"
Marqueurs de flèches : markerWidth="6" markerHeight="4"
Contrôles interactifs
/* Sliders */
input[type="range"] {
  width: 100%;
  height: 4px;
}

/* Boutons */
button {
  padding: 4-6px 8px;
  font-size: 10px;
  border-radius: 4px;
}

/* Labels */
label {
  font-size: 9-10px;
}
Structure CSS type
<style scoped>
.demo-container {
  font-family: 'Segoe UI', system-ui, sans-serif;
  padding: 4px;
  background: #fafafa;
  border-radius: 8px;
  font-size: 10px;
}

.main-container {
  display: flex;
  gap: 8-10px;
}

.graph-area {
  flex: 1;
  min-width: 300px;
}

.controls {
  width: 150px;
  flex-shrink: 0;
}

.section {
  background: white;
  padding: 5px;
  border-radius: 6px;
  box-shadow: 0 1px 2px rgba(0,0,0,0.1);
  margin-bottom: 4px;
}

.section h4 {
  margin: 0 0 3px 0;
  font-size: 10px;
  border-bottom: 1px solid #eee;
  padding-bottom: 2px;
}
</style>
Palette de couleurs
Usage	Couleur	Hex
Principal/Vecteurs	Bleu	#2196F3
Positif/Succès	Vert	#4CAF50
Négatif/Erreur	Rouge	#F44336
Avertissement	Orange	#FF9800
Accent/Angle	Violet	#9C27B0
Texte principal	Gris foncé	#333
Texte secondaire	Gris	#666
Bordures	Gris clair	#ddd
Fond sections	Blanc	#fff
Fond général	Gris très clair	#fafafa
Bonnes pratiques Slidev
Utiliser <style scoped> pour isoler les styles
Préfixer les IDs SVG avec le nom du composant pour éviter les conflits
Utiliser Vue 3 Composition API (<script setup>)
Limiter les animations CSS pour la performance
Tester sur la résolution cible (1920x1080)
Documents de laboratoire (labo/)
Format Markdown
Les laboratoires utilisent le format Markdown avec :

En-tête : titre, objectifs, matériel
Sections numérotées : Introduction, Matériel, Procédure, Questions
Formules : Notation LaTeX entre $...$ ou $$...$$
Tableaux : Format Markdown standard pour les mesures
Images : Stockées dans le même dossier ou sous-dossier images/
Structure type d'un laboratoire
# Laboratoire XX - Titre

## Objectifs
- Objectif 1
- Objectif 2

## Matériel requis
- Équipement 1
- Équipement 2

## Théorie
Rappels théoriques avec formules.

## Procédure

### Partie 1 : Titre
1. Étape 1
2. Étape 2

### Partie 2 : Titre
...

## Questions d'analyse
1. Question 1
2. Question 2

## Annexes (si nécessaire)
Conventions pour les formules
Utiliser j (pas i) pour l'unité imaginaire en électronique
Angles en degrés avec symbole °
Unités SI avec espaces : 50 Ω, 155 MHz
Notation ingénieur pour les grands/petits nombres
Notes de cours (notes/)
Les plans de séquence et notes de cours suivent ce format :

# Plan de séquence - Semaine XX

## Objectifs de la semaine
- Objectif 1
- Objectif 2

## Contenu théorique
### Thème 1
...

### Thème 2
...

## Activités
- Activité 1
- Activité 2

## Évaluation
...
Conventions générales
Langue
Contenu en français
Termes techniques anglais acceptés entre parenthèses si nécessaire
Pas d'emojis sauf si explicitement demandé
Nommage des fichiers
Semaines : Semaine-XX/ (avec zéro de remplissage)
Laboratoires : Labo-XX/Laboratoire XX - Titre descriptif.md
Composants Vue : PascalCase.vue (ex: SmithChartDemo.vue)
Images : kebab-case.png
Git
Commits en français
Messages descriptifs du contenu modifié
Un commit par fonctionnalité/correction logique
