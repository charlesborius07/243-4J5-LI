#!/bin/bash
# Script de déploiement pour l'exercice 7.6
# Compile et téléverse le code vers le LilyGO

set -e  # Arrêter en cas d'erreur

echo "=========================================="
echo "Déploiement - Contrôle de LEDs"
echo "=========================================="

# Vérifier que le LilyGO est connecté
echo "1. Vérification de la connexion..."
if ! arduino-cli board list | grep -q "ttyUSB"; then
    echo "❌ Erreur: LilyGO non détecté"
    echo "   Vérifiez que le câble USB est bien branché"
    arduino-cli board list
    exit 1
fi

PORT=$(arduino-cli board list | grep ttyUSB | awk '{print $1}')
echo "✓ LilyGO détecté sur: $PORT"

# Compiler
echo ""
echo "2. Compilation du sketch..."
arduino-cli compile --fqbn esp32:esp32:esp32 led-control.ino

if [ $? -eq 0 ]; then
    echo "✓ Compilation réussie"
else
    echo "❌ Erreur de compilation"
    exit 1
fi

# Téléverser
echo ""
echo "3. Téléversement vers le LilyGO..."
arduino-cli upload -p $PORT --fqbn esp32:esp32:esp32 led-control.ino

if [ $? -eq 0 ]; then
    echo "✓ Téléversement réussi"
else
    echo "❌ Erreur de téléversement"
    echo "   Essayez d'appuyer sur le bouton BOOT pendant le téléversement"
    exit 1
fi

echo ""
echo "=========================================="
echo "✓ Déploiement terminé avec succès!"
echo "=========================================="
echo ""
echo "Pour tester:"
echo "  • Moniteur série:"
echo "    arduino-cli monitor -p $PORT -c baudrate=115200"
echo ""
echo "  • Interface tactile:"
echo "    sudo chvt 1"
echo "    sudo setsid sh -c 'exec </dev/tty1 >/dev/tty1 2>&1 python3 $PWD/touch_ui_led.py'"
echo ""
