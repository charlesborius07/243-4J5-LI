# Checklist de validation firmware

Cette checklist permet de s'assurer que toutes les exigences du firmware ont été respectées.

## Exigences Fonctionnelles
- [ ] Lecture du bouton assigné avec `digitalRead()`
- [ ] Lecture des 2 potentiomètres avec `analogRead()` (ADC 12 bits, 0–4095)
- [ ] Contrôle des 4 LEDs avec `digitalWrite()`
- [ ] Communication I2C avec le MPU6050 opérationnelle (sans erreurs au démarrage)
- [ ] Lecture des données de l'accéléromètre (orientation, mouvement, gestes)

## Exigences MQTT & Communication
- [ ] Publication des données capteurs (boutons, pots, MPU6050) sur les topics MQTT appropriés
- [ ] Souscription aux topics de commande pour contrôler les LEDs
- [ ] Structure des topics respectée (`etudiant/{prenom-nom}/...`)
- [ ] Communication via LTE établie (A7670G)

## Exigences de Qualité et Architecture
- [ ] Fonctions principales découpées: `initSerial()`, `initGPIO()`, `initI2C()`, `initLTE()`, `connectMQTT()`
- [ ] Boucle `loop()` non bloquante: `maintainMQTT()`, `readSensors()`, `publishData()`
- [ ] Utilisation de variables et fonctions en `camelCase` avec noms descriptifs
- [ ] Constantes en `UPPER_SNAKE_CASE` (ex : `PUBLISH_INTERVAL_MS`)
- [ ] Reconnexion automatique MQTT si déconnexion
- [ ] Logs série structurés (ex: `[INFO]`, `[ERROR]`, `[MQTT]`)
- [ ] Aucun `delay()` bloquant dans la boucle principale — utilisation de timestamps (`millis()`)
