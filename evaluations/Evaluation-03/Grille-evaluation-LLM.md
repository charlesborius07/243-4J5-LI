# Grille d'évaluation — Pipeline LLM pour IoT

**Cours:** 243-4J5-LI – Objets connectés
**Évaluation:** Intégration d'un LLM dans un pipeline IoT
**Pondération totale:** 20% (Capacité 1 : 10%, Capacité 2 : 10%)

---

## Échelle de notation

| Niveau | Description | Équivalence |
|:------:|-------------|:-----------:|
| **0** | Aucun travail remis ou travail non fonctionnel | 0% |
| **1** | Travail incomplet avec lacunes majeures | 40% |
| **2** | Travail partiel, en dessous du seuil | 50% |
| **3** | **Seuil de réussite** — Exigences minimales atteintes | 60% |
| **4** | Travail de bonne qualité, au-delà des attentes | 80% |
| **5** | Travail excellent, niveau optimal atteint | 100% |

---

## Partie 1 : Configuration et API LLM (25%)

### Critère 1.1 : Configuration de l'environnement (10%)

*Capacité 1 : Concevoir et programmer des objets connectés*
*Savoir-faire évalué : Configurer et sécuriser un environnement de développement*

| Niveau | Descripteur |
|:------:|-------------|
| **0** | Aucune configuration ou clé API exposée dans le code source |
| **1** | Configuration partielle; clé API en dur dans le code ou fichier non ignoré par git |
| **2** | Fichier .env présent mais incomplet ou mal structuré; .gitignore insuffisant |
| **3** | **Seuil :** Configuration fonctionnelle avec variables d'environnement; fichier .env utilisé; .gitignore inclut les fichiers sensibles; script de démarrage fourni |
| **4** | Configuration robuste avec validation des variables requises; fichier .env.example documenté; gestion des erreurs si clé manquante |
| **5** | Configuration professionnelle : support de multiples environnements (dev/prod), documentation complète des variables, script de vérification automatique |

### Critère 1.2 : Appels API LLM (15%)

*Capacité 1 : Concevoir et programmer des objets connectés*
*Savoir-faire évalué : Intégrer des API externes dans une application*

| Niveau | Descripteur |
|:------:|-------------|
| **0** | Aucun appel API fonctionnel |
| **1** | Appels API échouent (erreurs d'authentification, format incorrect) |
| **2** | Appels fonctionnels mais sans gestion d'erreurs; timeout non géré |
| **3** | **Seuil :** Appels API fonctionnels avec gestion des erreurs de base (try/except); réponse correctement parsée; modèle approprié utilisé (gpt-3.5-turbo ou claude-3-haiku) |
| **4** | Gestion avancée : retry sur erreur temporaire, timeout configurable, logging des appels, gestion du rate limiting |
| **5** | Intégration optimale : circuit breaker implémenté, métriques de latence, fallback sur modèle alternatif, cache des réponses similaires |

---

## Partie 2 : Prompt Engineering (25%)

### Critère 2.1 : Prompt système (15%)

*Capacité 1 : Concevoir et programmer des objets connectés*
*Savoir-faire évalué : Concevoir des interactions homme-machine efficaces*

| Niveau | Descripteur |
|:------:|-------------|
| **0** | Aucun prompt système défini |
| **1** | Prompt système vague ou non pertinent pour l'IoT; réponses incohérentes |
| **2** | Prompt système basique; réponses parfois utiles mais format imprévisible |
| **3** | **Seuil :** Prompt système clair définissant le rôle (assistant IoT), les responsabilités (analyser, détecter, suggérer), et le format de réponse (JSON structuré avec status/analysis/action) |
| **4** | Prompt bien structuré avec exemples de réponses attendues; gestion des cas limites; instructions de sécurité |
| **5** | Prompt professionnel : few-shot examples, gestion du contexte historique, adaptation dynamique selon le type de capteur, documentation des choix |

### Critère 2.2 : Formatage des données (10%)

*Capacité 2 : Maîtriser les protocoles de communication IdO*
*Savoir-faire évalué : Structurer les données pour l'échange entre systèmes*

| Niveau | Descripteur |
|:------:|-------------|
| **0** | Données envoyées au LLM non structurées ou illisibles |
| **1** | Format incohérent; informations importantes manquantes |
| **2** | Format JSON basique mais sans contexte suffisant pour l'analyse |
| **3** | **Seuil :** Données formatées en JSON lisible; inclusion des valeurs capteurs, timestamp, et seuils de référence; contexte suffisant pour l'analyse |
| **4** | Enrichissement des données avec historique récent, calculs dérivés (heat index, tendances), et métadonnées du device |
| **5** | Formatage optimal : contexte adaptatif selon la situation, compression intelligente pour réduire les tokens, inclusion de patterns temporels |

---

## Partie 3 : Pipeline de traitement (30%)

### Critère 3.1 : Réception et validation (10%)

*Capacité 2 : Maîtriser les protocoles de communication IdO*
*Savoir-faire évalué : Implémenter une communication MQTT robuste*

| Niveau | Descripteur |
|:------:|-------------|
| **0** | Aucune réception MQTT fonctionnelle |
| **1** | Réception instable; déconnexions fréquentes non gérées |
| **2** | Réception fonctionnelle mais sans validation des données; messages malformés causent des crashes |
| **3** | **Seuil :** Réception MQTT stable; validation des données entrantes (format JSON, valeurs dans les limites); rejet propre des messages invalides avec logging |
| **4** | Validation avancée avec Pydantic ou équivalent; reconnexion automatique; gestion des topics multiples |
| **5** | Pipeline robuste : file d'attente pour pics de charge, déduplication des messages, métriques de réception, healthcheck |

### Critère 3.2 : Analyse LLM intégrée (10%)

*Capacité 1 : Concevoir et programmer des objets connectés*
*Savoir-faire évalué : Intégrer l'intelligence artificielle dans un système embarqué*

| Niveau | Descripteur |
|:------:|-------------|
| **0** | Analyse LLM non intégrée au pipeline |
| **1** | Intégration partielle; analyse bloquante causant des délais excessifs |
| **2** | Analyse fonctionnelle mais non optimisée; appel LLM pour chaque message sans discernement |
| **3** | **Seuil :** Analyse LLM intégrée dans le flux MQTT; décision intelligente sur quand appeler le LLM (seuils dépassés, anomalies détectées); temps de réponse acceptable (< 5s) |
| **4** | Analyse asynchrone; batching des messages similaires; cache des analyses récentes; règles de pré-filtrage |
| **5** | Architecture optimale : analyse en temps réel avec fallback sur règles simples, adaptation du niveau de détail selon l'urgence, apprentissage des patterns |

### Critère 3.3 : Exécution des actions (10%)

*Capacité 1 : Concevoir et programmer des objets connectés*
*Savoir-faire évalué : Automatiser des réponses basées sur l'analyse de données*

| Niveau | Descripteur |
|:------:|-------------|
| **0** | Aucune action exécutée suite à l'analyse |
| **1** | Actions codées en dur; ne correspondent pas à l'analyse LLM |
| **2** | Actions basiques (ON/OFF) sans gradation ni intelligence |
| **3** | **Seuil :** Actions exécutées selon le statut de l'analyse (normal/warning/critical); publication MQTT des commandes aux actionneurs; notifications pour alertes |
| **4** | Actions graduées et contextuelles; confirmation des actions; logging complet; possibilité de désactiver les actions automatiques |
| **5** | Système d'actions intelligent : chaînes d'actions, rollback si échec, audit trail complet, intégration avec système de notification externe |

---

## Partie 4 : Documentation et qualité (20%)

### Critère 4.1 : Documentation technique (10%)

*Capacité 1 : Concevoir et programmer des objets connectés*
*Savoir-faire évalué : Documenter un système technique*

| Niveau | Descripteur |
|:------:|-------------|
| **0** | Aucune documentation |
| **1** | README minimal sans instructions utiles |
| **2** | Documentation partielle; manque les instructions d'installation ou d'utilisation |
| **3** | **Seuil :** README complet avec description du projet, instructions d'installation, configuration requise, exemples d'utilisation, architecture du pipeline documentée |
| **4** | Documentation détaillée : diagrammes de flux, exemples de prompts, cas d'utilisation, troubleshooting |
| **5** | Documentation professionnelle : guide complet, exemples annotés, vidéo de démonstration, documentation API, changelog |

### Critère 4.2 : Qualité du code (6%)

*Capacité 1 : Concevoir et programmer des objets connectés*
*Savoir-faire évalué : Produire du code maintenable et lisible*

| Niveau | Descripteur |
|:------:|-------------|
| **0** | Code illisible ou non fonctionnel |
| **1** | Code fonctionnel mais très difficile à comprendre; aucune structure |
| **2** | Structure basique; nommage incohérent; code dupliqué |
| **3** | **Seuil :** Code structuré en fonctions logiques; nommage clair des variables et fonctions; commentaires aux endroits importants; séparation configuration/logique |
| **4** | Code bien organisé en modules; typage Python (type hints); docstrings; respect PEP8 |
| **5** | Code exemplaire : architecture modulaire, tests unitaires, gestion des erreurs exhaustive, patterns de conception appropriés |

### Critère 4.3 : Qualité du français (4%)

*Expression et communication en français*

| Niveau | Descripteur |
|:------:|-------------|
| **0** | Texte illisible ou en langue étrangère uniquement |
| **1** | Nombreuses fautes nuisant à la compréhension; structure incohérente |
| **2** | Fautes fréquentes mais texte compréhensible; organisation déficiente |
| **3** | **Seuil :** Français acceptable avec quelques fautes mineures; texte clair et structuré; termes techniques appropriés |
| **4** | Français de bonne qualité; texte bien rédigé et organisé; vocabulaire technique précis |
| **5** | Français excellent; rédaction professionnelle; aucune faute; style clair et concis |

---

## Tableau récapitulatif

| Partie | Critère | Pondération | Note /5 | Points |
|--------|---------|:-----------:|:-------:|:------:|
| **1. Configuration API** | 1.1 Configuration environnement | 10% | /5 | |
| | 1.2 Appels API LLM | 15% | /5 | |
| **2. Prompt Engineering** | 2.1 Prompt système | 15% | /5 | |
| | 2.2 Formatage des données | 10% | /5 | |
| **3. Pipeline** | 3.1 Réception et validation | 10% | /5 | |
| | 3.2 Analyse LLM intégrée | 10% | /5 | |
| | 3.3 Exécution des actions | 10% | /5 | |
| **4. Documentation** | 4.1 Documentation technique | 10% | /5 | |
| | 4.2 Qualité du code | 6% | /5 | |
| | 4.3 Qualité du français | 4% | /5 | |
| | **TOTAL** | **100%** | | **/100** |

---

## Calcul de la note finale

Pour chaque critère :
$$\text{Points} = \text{Note sur 5} \times \frac{\text{Pondération}}{5}$$

**Exemple :** Critère 1.2 Appels API LLM (15%), note 4/5
$$\text{Points} = 4 \times \frac{15}{5} = 4 \times 3 = 12 \text{ points}$$

**Note finale du projet :** Somme des points sur 100, puis ×0.20 pour la pondération dans le cours.

---

## Correspondance avec les capacités du cours

| Capacité | Critères associés | Pondération |
|----------|-------------------|:-----------:|
| **Capacité 1** : Concevoir et programmer des objets connectés | 1.1, 1.2, 2.1, 3.2, 3.3, 4.1, 4.2 | ~76% |
| **Capacité 2** : Maîtriser les protocoles de communication IdO | 2.2, 3.1 | ~20% |
| **Expression française** | 4.3 | ~4% |

---

## Livrables attendus

### Structure du dépôt

```
projet-llm-iot/
├── README.md                 # Documentation principale
├── .env.example             # Template des variables d'environnement
├── .gitignore               # Doit inclure .env, __pycache__, etc.
├── requirements.txt         # Dépendances Python
├── src/
│   ├── config.py            # Configuration et chargement des variables
│   ├── pipeline.py          # Pipeline principal
│   ├── llm_client.py        # Client API LLM
│   ├── mqtt_handler.py      # Gestion MQTT
│   ├── validator.py         # Validation des données
│   └── actions.py           # Exécution des actions
├── prompts/
│   └── system_prompt.txt    # Prompt système documenté
├── docs/
│   ├── architecture.md      # Description de l'architecture
│   └── examples.md          # Exemples d'utilisation
└── tests/                   # Tests (bonus)
    └── test_pipeline.py
```

### Checklist de remise

- [ ] Aucune clé API dans le code source
- [ ] Fichier .env.example avec toutes les variables requises
- [ ] .gitignore inclut : `.env`, `*.pyc`, `__pycache__/`
- [ ] README avec instructions d'installation et d'exécution
- [ ] Pipeline fonctionnel démontrable
- [ ] Prompt système documenté et justifié
- [ ] Architecture du pipeline expliquée

---

## Critères de sécurité (éliminatoires)

**Attention :** Les situations suivantes entraînent une pénalité automatique :

| Infraction | Pénalité |
|------------|----------|
| Clé API exposée dans le code source | -20 points |
| Clé API dans l'historique git | -15 points |
| Fichier .env commité | -10 points |
| Aucun .gitignore | -5 points |

---

**Date de remise :** Semaine 12
**Mode de remise :** Git (branche `prenom-nom/projet-llm`)
