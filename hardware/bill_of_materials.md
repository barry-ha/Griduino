# Griduino
GPS display for vehicle dashboard showing position in the Maidenhead Locator System with distance to nearby grids, designed for ham radio rovers.

## Active Components
| Mfr         | Mfr Part No     | Digi-Key Part No | Reference | Qty   | USD$  | Ext$   | Description  |
| ----------- | --------------- | ---------------- | --------- | :---: | ----- | ------ | ------------ |
| Maxim       | DS1804-050+     | DS1804-050+-ND   | U1        |  1    |  2.94 |   2.94 | IC Digital Pot 50KOHM 100-tap |
| Adafruit    | 3857            | 1528-2648-ND     | U2        |  1    | 22.95 |  22.95 | Feather M4 Express |
| Microchip   | MIC5239-5.0YS   | MIC5239-5.0YS-ND | U3        |  1    |  2.66 |   2.66 | IC Linear Regulator 5v 500mA |
| TI          | LM386N-3/NOPB   | 296-43959-5-ND   | U4        |  1    |  1.17 |   1.17 | Audio Amp, Mono LM386 700MW |
| Adafruit    | 746             | 1528-4279-ND     | U5        |  1    | 39.95 |  39.95 | Ultimate GPS, 66 channel |
| Adafruit    | 3966            | 1528-2733-ND     | U6        |  1    |  9.95 |   9.95 | BMP-388 Barometric Pressure |
| Adafruit    | 1743            | 1528-2614-ND     | Display   |  1    | 29.95 |  29.95 | TFT Display  |
|             |                 |                  | **total** | **6** |       | **109.57** |     |

## Passive Components
| Mfr         | Mfr Part No     | Digi-Key Part No     | Reference          |  Qty   | USD$  | Ext$   | Description  |
| ----------- | --------------- | -------------------- | ------------------ | :----: | ----- | ------ | ------------ |
| Adafruit    | 1898            | n/a                  | LS1                |    1   |  1.85 |   1.85 | 8-ohm Mini Speaker, PCB mount, 0.2W |
| AVX Corp    |                 |                      | C1, C2, C3, C6, C9 |    5   |  2.67 |  13.35 | CAP Electrolytic 47UF 10% 25v radial |
| KEMET       | C320C104K5R5TA  | 399-4264-ND          | C2, C3, C6, C9     |    4   |  0.22 |   0.88 | CAP Ceramic 0.1UF 50V X7R radial |
| ON Semi     | 1N4001G         | 1N4001GOS-ND         | D1, D2, D3         |    3   |  0.21 |   0.63 | Gen Purpose Diode 50V 1A |
| Yageo       | FKN2WSJT-73-22R | 22AUCT-ND            | R1                 |    1   |  0.39 |   0.39 | RES 22-ohm 2W 5% axial |
| Yageo       | HHV-25JR-52-82K? | HHV-25JR-52-82K-ND  | R2                 |    1   |  0.34 |   0.34 | RES 82K 1/4W 5% axial |
| Yageo       |                 |                      | R3                 |    1   |  0.10 |   0.20 | RES 10K 1/4W 1% axial |
| Yageo       | MFR-25FBF52-10R | 10.0XBK-ND           | R4                 |    1   |  0.10 |   0.10 | RES 10-ohm 1/4W 1% axial |
| Yageo       |                 |                      | R5                 |    1   |  0.10 |   0.20 | RES 100-ohm 1/4W 1% axial |
| Vishay      | 1.5KE18CA       | 1.5KE18CA-E3/51GI-ND | CR1             |    1   |  1.21 |   1.21 | Zener Diode 25.2v Clamp |
|             |                 |                      | **total**          | **16** |       | **19.15** |     |

## Hardware
| Mfr         | Mfr Part No     | Digi-Key Part No | Reference       |  Qty  | USD$  | Ext$   | Description  |
| ----------- | --------------- | ---------------- | --------------- | :---: | ----- | ------ | ------------ |
| CUI Devices | PJ-102A         | CP-102A-ND       | J1              |   1   |  0.64 |   0.64 | Connector Power Jack 2X5.5 mm  |
| Sullins     | PPTC201LFBN-RC  | S7018-ND         | J2              |   1   |  1.23 |   1.23 | Connector 20-pos 0.1 Tin (LCD) |
| CUI Devices | SJ1-3545N       | CP1-3545N-ND     | J3              |   1   |  1.31 |   1.31 | Connector 3.5mm Audio Jack     |
| Keystone    | 4952            | 36-4952-ND       | TP1-3, TP_GND   |   4   |  0.21 |   0.84 | PC Test Point Loop |
| JLC PCB     |                 |                  | PCB             |   1   |  5.00 |   5.00 | Printed Circuit Board |
| Polycase    | QS-50MBT        | n/a              |                 |   1   |  4.98 |   4.98 | Plastic Case |
|             |                 |                  | **total**       |       |       | **14.00** |     |
