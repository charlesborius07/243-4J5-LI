## Checklist de validation — Interface

### Affichage
- [ ] Interface s'ouvre en plein écran sans erreur
- [ ] Données des boutons mises à jour en temps réel (< 500 ms)
- [ ] Valeurs des potentiomètres affichées avec unité ou pourcentage
- [ ] Données accéléromètre (X, Y, Z) affichées et actualisées
- [ ] Indicateur de connexion MQTT visible et exact

### Contrôle
- [ ] Chaque bouton tactile LED envoie la commande MQTT correcte
- [ ] L'état de la LED se reflète dans l'interface (feedback visuel)

### Démarrage automatique
- [ ] Service systemd `iot-interface.service` créé et activé
- [ ] Interface se lance automatiquement après un redémarrage (`sudo reboot`)
- [ ] Service redémarre après un kill forcé (`kill -9 <pid>`)

### Qualité
- [ ] Aucune exception Python non gérée lors de l'utilisation normale
- [ ] Interface utilisable au doigt (zones tactiles ≥ 44×44 px)
