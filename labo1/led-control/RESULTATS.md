# Résultats de l'exercice 7.6

## ✅ Déploiement réussi

**Date:** 2025-12-05  
**Port série utilisé:** /dev/ttyACM0

### Compilation

```
Sketch uses 283599 bytes (21%) of program storage space. Maximum is 1310720 bytes.
Global variables use 20816 bytes (6%) of dynamic memory, leaving 306864 bytes for local variables.
```

✓ Compilation réussie

### Téléversement

```
Connected to ESP32 on /dev/ttyACM0:
Chip type:          ESP32-D0WD-V3 (revision v3.1)
Features:           Wi-Fi, BT, Dual Core + LP Core, 240MHz
```

✓ Téléversement réussi

## ✅ Tests de communication série

### Test 1: LED ROUGE
```
Commande envoyée: rouge
Réponse: LED ROUGE allumée, LED VERTE éteinte
```
✓ Fonctionne

### Test 2: LED VERTE
```
Commande envoyée: vert
Réponse: LED VERTE allumée, LED ROUGE éteinte
```
✓ Fonctionne

### Test 3: ÉTEINDRE
```
Commande envoyée: off
Réponse: Toutes les LEDs éteintes
```
✓ Fonctionne

### Test 4: Commande invalide
```
Commande envoyée: bleu
Réponse: Commande non reconnue! Utilisez: rouge, vert, ou off
```
✓ Gestion d'erreur fonctionne

### Test 5: Séquence automatique
```
Séquence: rouge → vert → off (x3)
```
✓ Séquence complétée sans erreur

## 📝 Notes importantes

### Port série détecté
Le LilyGO A7670G est apparu sur `/dev/ttyACM0` au lieu de `/dev/ttyUSB0`.  
Tous les scripts ont été mis à jour pour utiliser le bon port.

### Pins utilisés
- **LED Rouge:** GPIO 25
- **LED Verte:** GPIO 26

### Prochaines étapes

1. **Monter le circuit physique:**
   - Connecter les LEDs avec résistances sur breadboard
   - Vérifier le bon sens des LEDs (anode +, cathode -)

2. **Tester l'interface tactile:**
   ```bash
   sudo chvt 1
   sudo setsid sh -c 'exec </dev/tty1 >/dev/tty1 2>&1 python3 /home/fpoisson/243-4J5-LI/labo1/led-control/touch_ui_led.py'
   ```

3. **Prendre les photos requises:**
   - Circuit sur breadboard
   - Interface tactile
   - LEDs allumées

4. **Commit Git:**
   ```bash
   git add labo1/led-control/
   git commit -m "Exercice 7.6: Contrôle de LEDs via interface tactile et port série"
   git push
   ```
