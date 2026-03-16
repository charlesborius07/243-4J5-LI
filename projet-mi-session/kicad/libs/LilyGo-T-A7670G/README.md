# LilyGo T-A7670G V1.1 — Bibliothèque KiCad 9.0

## Contenu

- `LilyGo-T-A7670G.kicad_sym` → Symbole schématique (32 pins)
- `LilyGo-T-A7670G.pretty/LilyGo-T-A7670G-V1.1.kicad_mod` → Empreinte PCB (footprint)

## Comment importer dans KiCad 9.0

### Importer le symbole

1. Ouvre **KiCad 9.0** → **Éditeur de symboles**
2. Menu **Fichier → Ajouter une bibliothèque...**
3. Choisis **"Bibliothèque de projet"** ou **"Bibliothèque globale"**
4. Navigue vers le fichier `LilyGo-T-A7670G.kicad_sym` et sélectionne-le
5. Le symbole `LilyGo-T-A7670G-V1.1` est maintenant disponible

### Importer le footprint

1. Ouvre **KiCad 9.0** → **Éditeur d'empreintes**
2. Menu **Fichier → Ajouter une bibliothèque...**
3. Choisis **"Bibliothèque de projet"** ou **"Bibliothèque globale"**
4. Navigue vers le **dossier** `LilyGo-T-A7670G.pretty` et sélectionne-le
5. L'empreinte `LilyGo-T-A7670G-V1.1` est maintenant disponible

## Pinout (32 pins — 2 rangées de 16)

### Header gauche (Pins 1–16)

| Pin | Nom          | Type              |
|-----|--------------|-------------------|
| 1   | VIN          | Alimentation IN   |
| 2   | GND          | Masse             |
| 3   | IO12         | GPIO              |
| 4   | IO13         | GPIO              |
| 5   | IO14         | GPIO              |
| 6   | IO15         | GPIO              |
| 7   | IO25         | GPIO              |
| 8   | IO26         | GPIO              |
| 9   | IO27         | GPIO              |
| 10  | IO32         | GPIO              |
| 11  | IO33         | GPIO              |
| 12  | IO34         | Entrée seulement  |
| 13  | IO35/BAT_ADC | Entrée seulement  |
| 14  | SOLAR_IN     | Alimentation IN   |
| 15  | VBAT         | Alimentation IN   |
| 16  | GND          | Masse             |

### Header droit (Pins 17–32)

| Pin | Nom           | Type              |
|-----|---------------|-------------------|
| 17  | 3V3           | Alimentation OUT  |
| 18  | GND           | Masse             |
| 19  | IO0           | GPIO (Boot)       |
| 20  | IO2/SD_CS     | GPIO / SD Card CS |
| 21  | IO4           | GPIO              |
| 22  | IO5           | GPIO              |
| 23  | IO18/SD_CLK   | GPIO / SD Card CLK|
| 24  | IO19/SD_MISO  | GPIO / SD MISO    |
| 25  | IO21          | GPIO (I2C SDA)    |
| 26  | IO22          | GPIO (I2C SCL)    |
| 27  | IO23/SD_MOSI  | GPIO / SD MOSI    |
| 28  | RST           | Reset             |
| 29  | RXD           | UART RX           |
| 30  | TXD           | UART TX           |
| 31  | IO36/SVP      | Entrée seulement  |
| 32  | IO39/SVN      | Entrée seulement  |

## Dimensions du footprint

- Espacement entre pins : **2.54mm** (100 mil)
- Espacement entre les rangées : **22.86mm** (900 mil)
- Pads : **1.7mm** diamètre, perçage **1.0mm**
- Pin 1 = pad rectangulaire (marqueur)

## ⚠️ IMPORTANT

**Mesure ta carte avec un pied à coulisse** avant de fabriquer un PCB !
L'espacement entre les deux rangées de pins (22.86mm) est une valeur
standard mais peut varier légèrement selon ta version exacte.

## Source

Pinout basé sur :
- GitHub officiel : https://github.com/Xinyuan-LilyGO/LilyGo-Modem-Series
- Schéma V1.1 : schematic/esp32/T-Call-A7670-V1.1.pdf
