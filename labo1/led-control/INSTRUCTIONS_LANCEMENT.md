# Instructions pour lancer l'interface tactile

## Problème rencontré

L'interface tactile nécessite des privilèges `sudo` pour accéder à `/dev/tty1` (l'écran physique du Raspberry Pi). Dans une session SSH non-interactive, `sudo` ne peut pas demander le mot de passe.

## Solution : Lancement manuel

Vous devez exécuter la commande manuellement dans votre session SSH.

### Option 1 : Commande directe (recommandée)

```bash
cd ~/243-4J5-LI/labo1/led-control
sudo chvt 1
sudo setsid sh -c 'exec </dev/tty1 >/dev/tty1 2>&1 python3 /home/fpoisson/243-4J5-LI/labo1/led-control/touch_ui_led.py'
```

### Option 2 : Utiliser le script de lancement

```bash
cd ~/243-4J5-LI/labo1/led-control
sudo ./launch_ui.sh
```

## Comment arrêter l'interface

L'interface peut être arrêtée de deux façons:

1. **Sur l'écran tactile :** Appuyez sur le bouton **QUIT**
2. **Depuis SSH :** Tapez `q` (si vous voyez l'interface) ou tuez le processus:
   ```bash
   sudo pkill -f touch_ui_led.py
   ```

## Vérification que l'interface est lancée

```bash
# Vérifier les processus Python en cours
ps aux | grep touch_ui_led

# Vérifier les processus sur TTY1
ps aux | grep tty1
```

## Alternative : Configuration sudo sans mot de passe (optionnel)

Si vous voulez automatiser le lancement, vous pouvez configurer sudo pour ne pas demander de mot de passe pour cette commande spécifique:

```bash
sudo visudo -f /etc/sudoers.d/touch-ui
```

Ajoutez cette ligne:
```
fpoisson ALL=(ALL) NOPASSWD: /usr/bin/python3 /home/fpoisson/243-4J5-LI/labo1/led-control/touch_ui_led.py
```

⚠️ **Attention:** Cette approche réduit la sécurité. À utiliser uniquement en environnement de développement.

## Test rapide sans écran physique

Si vous voulez tester la communication série sans l'interface tactile:

```bash
cd ~/243-4J5-LI/labo1/led-control
python3 test_serial.py -i
```

Cela lance le mode interactif où vous pouvez taper `rouge`, `vert`, ou `off` directement.
