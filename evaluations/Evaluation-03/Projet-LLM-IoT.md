# Projet — Pipeline LLM pour IoT

**Cours:** 243-4J5-LI – Objets connectés
**Pondération:** 20%
**Date de remise:** Semaine 12

---

## Objectifs

Ce projet vise à intégrer un modèle de langage (LLM) dans votre système IoT afin d'ajouter une couche d'intelligence artificielle capable d'analyser les données de vos capteurs et de prendre des décisions automatisées.

À la fin de ce projet, vous serez capable de :

1. Configurer et sécuriser une API LLM (OpenAI ou Anthropic)
2. Concevoir des prompts efficaces pour l'analyse de données IoT
3. Intégrer un LLM dans un pipeline de traitement MQTT
4. Automatiser des actions basées sur l'analyse intelligente des données

---

## Contexte

Votre système IoT actuel collecte des données de capteurs (température, humidité, accéléromètre, boutons) et les transmet via MQTT. Jusqu'à présent, la logique de décision était basée sur des seuils fixes (ex: si température > 30°C, allumer le ventilateur).

L'intégration d'un LLM permet une analyse plus nuancée :
- Détection de patterns complexes
- Prise en compte du contexte historique
- Génération d'alertes intelligentes
- Suggestions d'actions adaptatives

---

## Spécifications techniques

### 1. Configuration de l'API LLM

Vous devez utiliser l'une des API suivantes :

| Fournisseur | Modèle recommandé | Documentation |
|-------------|-------------------|---------------|
| OpenAI | `gpt-3.5-turbo` | https://platform.openai.com/docs |
| Anthropic | `claude-3-haiku-20240307` | https://docs.anthropic.com |

**Exigences de sécurité :**
- La clé API doit être stockée dans un fichier `.env`
- Le fichier `.env` doit être ignoré par git (`.gitignore`)
- Un fichier `.env.example` doit documenter les variables requises

```bash
# .env.example
MQTT_BROKER=localhost
MQTT_PORT=1883
OPENAI_API_KEY=sk-votre-cle-ici
# OU
ANTHROPIC_API_KEY=sk-ant-votre-cle-ici
```

### 2. Prompt système

Votre prompt système doit :
- Définir le rôle de l'assistant (analyste IoT)
- Spécifier les responsabilités (analyser, détecter anomalies, suggérer actions)
- Définir le format de réponse attendu (JSON structuré)

**Format de réponse obligatoire :**

```json
{
  "status": "normal | warning | critical",
  "analysis": "Description courte de l'analyse",
  "action": "Action suggérée ou null",
  "confidence": 0.0 à 1.0
}
```

### 3. Pipeline de traitement

Le pipeline doit suivre les étapes suivantes :

```
┌─────────────┐    ┌─────────────┐    ┌─────────────┐    ┌─────────────┐
│  Réception  │───▶│ Validation  │───▶│   Analyse   │───▶│   Actions   │
│    MQTT     │    │   Données   │    │     LLM     │    │ Automatiques│
└─────────────┘    └─────────────┘    └─────────────┘    └─────────────┘
```

**Réception MQTT :**
- Souscription aux topics de vos capteurs
- Parsing JSON des messages

**Validation :**
- Vérification du format des données
- Rejet des valeurs hors limites avec logging

**Analyse LLM :**
- Appel à l'API LLM avec les données formatées
- Parsing de la réponse JSON

**Actions :**
- Exécution des actions selon le statut (normal/warning/critical)
- Publication MQTT pour contrôler les actionneurs
- Envoi de notifications si nécessaire

### 4. Critères de décision pour l'appel LLM

L'appel au LLM ne doit pas être systématique. Définissez des critères intelligents :

- Seuil dépassé (température > X, humidité < Y)
- Changement d'état significatif
- Intervalle de temps (analyse périodique)
- Demande explicite de l'utilisateur

---

## Livrables

### Structure du dépôt

```
projet-llm-iot/
├── README.md
├── .env.example
├── .gitignore
├── requirements.txt
├── src/
│   ├── config.py
│   ├── pipeline.py
│   ├── llm_client.py
│   ├── mqtt_handler.py
│   ├── validator.py
│   └── actions.py
├── prompts/
│   └── system_prompt.txt
└── docs/
    └── architecture.md
```

### README.md

Votre README doit contenir :

1. **Description du projet** : Objectif et fonctionnalités
2. **Prérequis** : Python, compte API, broker MQTT
3. **Installation** :
   ```bash
   pip install -r requirements.txt
   cp .env.example .env
   # Éditer .env avec vos clés
   ```
4. **Configuration** : Variables d'environnement requises
5. **Utilisation** : Comment démarrer le pipeline
6. **Architecture** : Schéma du flux de données
7. **Exemples** : Cas d'utilisation typiques

### Documentation de l'architecture

Le fichier `docs/architecture.md` doit expliquer :

- Le flux de données complet
- Les décisions de conception
- Le format des messages MQTT
- La logique de décision pour les appels LLM
- Les actions possibles et leurs déclencheurs

---

## Exemples de code

### Configuration sécurisée

```python
# config.py
import os
from dotenv import load_dotenv

load_dotenv()

class Config:
    MQTT_BROKER = os.getenv("MQTT_BROKER", "localhost")
    MQTT_PORT = int(os.getenv("MQTT_PORT", 1883))
    OPENAI_API_KEY = os.getenv("OPENAI_API_KEY")
    ANTHROPIC_API_KEY = os.getenv("ANTHROPIC_API_KEY")

    @classmethod
    def validate(cls):
        if not cls.OPENAI_API_KEY and not cls.ANTHROPIC_API_KEY:
            raise ValueError("Aucune clé API LLM configurée!")
```

### Client LLM avec gestion d'erreurs

```python
# llm_client.py
import json
from openai import OpenAI
from config import Config

client = OpenAI(api_key=Config.OPENAI_API_KEY)

SYSTEM_PROMPT = """Tu es un assistant d'analyse IoT..."""

def analyze_data(data: dict) -> dict:
    try:
        response = client.chat.completions.create(
            model="gpt-3.5-turbo",
            messages=[
                {"role": "system", "content": SYSTEM_PROMPT},
                {"role": "user", "content": json.dumps(data)}
            ],
            response_format={"type": "json_object"}
        )
        return json.loads(response.choices[0].message.content)
    except Exception as e:
        logging.error(f"Erreur LLM: {e}")
        return {"status": "error", "analysis": str(e), "action": None}
```

---

## Évaluation

Votre projet sera évalué selon la grille fournie, couvrant :

| Partie | Pondération |
|--------|:-----------:|
| Configuration et API LLM | 25% |
| Prompt Engineering | 25% |
| Pipeline de traitement | 30% |
| Documentation et qualité | 20% |

**Rappel important :** La sécurité des clés API est critique. Toute clé exposée dans le code source ou l'historique git entraîne des pénalités.

---

## Ressources

- [Documentation OpenAI](https://platform.openai.com/docs)
- [Documentation Anthropic](https://docs.anthropic.com)
- [python-dotenv](https://pypi.org/project/python-dotenv/)
- [Pydantic](https://docs.pydantic.dev/) (validation de données)
- [Paho MQTT](https://pypi.org/project/paho-mqtt/)

---

## Questions fréquentes

**Q: Puis-je utiliser un autre fournisseur LLM?**
R: Oui, avec l'accord préalable de l'enseignant. Ollama en local est acceptable.

**Q: Le pipeline doit-il tourner en continu?**
R: Oui, le script doit rester actif et traiter les messages au fur et à mesure.

**Q: Combien d'appels LLM maximum?**
R: Optimisez vos appels. Une analyse toutes les 30 secondes ou sur événement est raisonnable.

**Q: Comment tester sans capteurs physiques?**
R: Publiez des messages MQTT de test manuellement ou avec un script.

---

**Date de remise :** Semaine 12
**Mode de remise :** Git (branche `prenom-nom/projet-llm`)
