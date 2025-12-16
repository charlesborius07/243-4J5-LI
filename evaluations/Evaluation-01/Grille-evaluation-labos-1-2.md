# Grille d'évaluation — Laboratoires 1 et 2

**Cours:** 243-4J5-LI – Objets connectés
**Évaluation:** Laboratoire capteurs et Python — Environnement IoT et communication MQTT
**Pondération totale:** 15% (Capacité 1)

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

## Partie 1 : Configuration de l'environnement (25%)

### Critère 1.1 : Configuration Raspberry Pi et réseau (15%)

*Capacité 1 : Concevoir et programmer des objets connectés*

| Niveau | Descripteur |
|:------:|-------------|
| **0** | Aucune configuration ou système non fonctionnel |
| **1** | Ubuntu Server installé mais configuration réseau incorrecte; connexion SSH impossible |
| **2** | Connexion SSH locale fonctionnelle mais pas d'accès distant; configuration WiFi absente ou non fonctionnelle |
| **3** | **Seuil :** Raspberry Pi accessible via SSH en local; configuration IP statique Ethernet fonctionnelle; WiFi configuré (WPA-Personal ou WPA-EAP); système stable |
| **4** | Configuration réseau robuste avec les deux interfaces (Ethernet et WiFi) fonctionnelles; documentation des paramètres réseau |
| **5** | Configuration professionnelle : double connectivité stable, scripts d'automatisation, configuration sécurisée des fichiers Netplan |

### Critère 1.2 : Tunnel Cloudflare et accès distant (10%)

*Capacité 1 : Concevoir et programmer des objets connectés*

| Niveau | Descripteur |
|:------:|-------------|
| **0** | Aucun tunnel configuré |
| **1** | Tunnel créé mais non fonctionnel; erreurs de configuration |
| **2** | Tunnel fonctionnel par moments mais instable; configuration SSH distante incomplète |
| **3** | **Seuil :** Tunnel Cloudflare opérationnel; accès SSH distant via Zero Trust fonctionnel; service cloudflared démarré automatiquement |
| **4** | Configuration Zero Trust avec policies appropriées; accès stable et documenté; configuration SSH côté client optimisée |
| **5** | Configuration exemplaire : tunnel multi-services (SSH + MQTT WSS), logs d'accès activés, documentation complète de l'architecture |

---

## Partie 2 : Interface tactile Python (25%)

### Critère 2.1 : Interface curses et gestion d'événements (15%)

*Capacité 1 : Concevoir et programmer des objets connectés*

| Niveau | Descripteur |
|:------:|-------------|
| **0** | Aucune interface ou code non fonctionnel |
| **1** | Interface curses basique mais ne répond pas aux événements tactiles; erreurs d'exécution |
| **2** | Interface affichée mais interaction tactile partielle; certains boutons ne fonctionnent pas |
| **3** | **Seuil :** Interface tactile fonctionnelle avec curses; boutons détectent les appuis via evdev; affichage correct sur TTY1; fermeture propre de l'application |
| **4** | Interface bien organisée avec feedback visuel; gestion robuste des événements; code structuré et lisible |
| **5** | Interface professionnelle : design soigné, animations fluides, gestion d'erreurs complète, code modulaire et documenté |

### Critère 2.2 : Communication série avec LilyGO (10%)

*Capacité 1 : Concevoir et programmer des objets connectés*

| Niveau | Descripteur |
|:------:|-------------|
| **0** | Aucune communication série implémentée |
| **1** | Code de communication présent mais non fonctionnel; erreurs de port série |
| **2** | Communication unidirectionnelle seulement (envoi OU réception) |
| **3** | **Seuil :** Communication série bidirectionnelle fonctionnelle; commandes envoyées depuis l'interface; réponses du LilyGO traitées correctement |
| **4** | Communication robuste avec gestion des erreurs de connexion; format de messages structuré |
| **5** | Communication optimale : protocole de messages documenté, reconnexion automatique, logs de débogage |

---

## Partie 3 : Programmation LilyGO A7670G (25%)

### Critère 3.1 : Contrôle des LEDs (10%)

*Capacité 1 : Concevoir et programmer des objets connectés*

| Niveau | Descripteur |
|:------:|-------------|
| **0** | Aucun code de contrôle des LEDs |
| **1** | Code Arduino présent mais LEDs ne répondent pas; erreurs de compilation |
| **2** | Une seule LED fonctionne; l'autre ne répond pas ou montage incorrect |
| **3** | **Seuil :** Les deux LEDs (rouge et verte) contrôlables via commandes série; montage correct sur breadboard avec résistances appropriées |
| **4** | Contrôle avancé : patterns lumineux, feedback série de l'état des LEDs, code bien structuré |
| **5** | Contrôle sophistiqué : PWM pour variation d'intensité, animations, gestion de l'état persistant |

### Critère 3.2 : Communication MQTT via WiFi ou LTE (25%)

*Capacité 1 : Concevoir et programmer des objets connectés*

| Niveau | Descripteur |
|:------:|-------------|
| **0** | Aucune communication MQTT implémentée |
| **1** | Tentative de connexion MQTT mais échecs; erreurs de configuration |
| **2** | Connexion au broker établie mais communication unidirectionnelle seulement (publication OU souscription) |
| **3** | **Seuil :** Communication MQTT bidirectionnelle fonctionnelle via WiFi OU LTE; LEDs contrôlables à distance via topics MQTT; souscription aux commandes et publication de l'état |
| **4** | Communication robuste : reconnexion automatique, QoS approprié, structure de topics cohérente, gestion des déconnexions; fonctionne via WiFi ET LTE |
| **5** | Communication optimale : WSS sécurisé fonctionnel, heartbeat régulier, support WiFi ET LTE testé et validé, architecture MQTT bien conçue |

---

## Partie 4 : Montage et intégration (15%)

### Critère 4.1 : Circuit sur breadboard (10%)

*Capacité 1 : Concevoir et programmer des objets connectés*

| Niveau | Descripteur |
|:------:|-------------|
| **0** | Aucun montage réalisé |
| **1** | Montage incomplet ou non fonctionnel; composants mal connectés |
| **2** | Montage partiel : LEDs fonctionnelles mais pas les boutons, ou vice versa |
| **3** | **Seuil :** Montage complet et fonctionnel avec 2 LEDs et 2 boutons; câblage correct; composants identifiables |
| **4** | Montage soigné avec câblage organisé par couleur; étiquetage des connexions; facilement reproductible |
| **5** | Montage exemplaire : organisation professionnelle, schéma de câblage annoté fourni, documentation photographique |

### Critère 4.2 : Boutons physiques avec publication MQTT (5%)

*Capacité 1 : Concevoir et programmer des objets connectés*

| Niveau | Descripteur |
|:------:|-------------|
| **0** | Boutons non implémentés ou non fonctionnels |
| **1** | Boutons détectés mais rebonds excessifs ou logique incorrecte |
| **2** | Un seul bouton fonctionne correctement |
| **3** | **Seuil :** Les deux boutons toggle les LEDs localement ET publient l'état sur MQTT; debounce fonctionnel |
| **4** | Implémentation robuste avec gestion d'interruptions; feedback visuel et série |
| **5** | Implémentation professionnelle : code optimisé, documentation du comportement, tests de robustesse documentés |

---

## Tableau récapitulatif

| Partie | Critère | Pondération | Note /5 | Points |
|--------|---------|:-----------:|:-------:|:------:|
| **1. Environnement** | 1.1 Configuration RPi et réseau | 15% | /5 | |
| | 1.2 Tunnel Cloudflare | 10% | /5 | |
| **2. Interface Python** | 2.1 Interface curses et événements | 15% | /5 | |
| | 2.2 Communication série | 10% | /5 | |
| **3. LilyGO** | 3.1 Contrôle des LEDs | 10% | /5 | |
| | 3.2 Communication MQTT | 25% | /5 | |
| **4. Intégration** | 4.1 Circuit breadboard | 10% | /5 | |
| | 4.2 Boutons physiques MQTT | 5% | /5 | |
| | **TOTAL** | **100%** | | **/100** |

---

## Calcul de la note finale

Pour chaque critère :
$$\text{Points} = \text{Note sur 5} \times \frac{\text{Pondération}}{5}$$

**Exemple :** Critère 3.2 Communication MQTT (25%), note 4/5
$$\text{Points} = 4 \times \frac{25}{5} = 4 \times 5 = 20 \text{ points}$$

**Note finale du laboratoire :** Somme des points sur 100, puis ×0.15 pour la pondération dans le cours.

---

## Correspondance avec les capacités du cours

| Capacité | Critères associés | Pondération |
|----------|-------------------|:-----------:|
| **Capacité 1** : Concevoir et programmer des objets connectés | 1.1, 1.2, 2.1, 2.2, 3.1, 3.2, 4.1, 4.2 | 100% |

---

## Livrables attendus

### Labo 1
- [ ] Raspberry Pi accessible en SSH local et distant (Cloudflare Tunnel)
- [ ] Interface tactile Python fonctionnelle sur TTY1
- [ ] Contrôle de LEDs via port série depuis l'interface tactile
- [ ] Code Arduino de base pour le LilyGO
- [ ] Photos du montage et de l'interface

### Labo 2
- [ ] Broker Mosquitto configuré avec WSS via Cloudflare
- [ ] Communication MQTT fonctionnelle (WiFi ou LTE)
- [ ] Interface tactile avec contrôle MQTT des LEDs
- [ ] Boutons physiques avec publication MQTT
- [ ] Montage complet sur breadboard (2 LEDs + 2 boutons)

---

**Date de remise :** Semaine 3
**Mode de remise :** Git (branche `prenom-nom/labo1` et `prenom-nom/labo2`)
