# Griduino
GPS display for vehicle dashboard showing position in the Maidenhead Locator System with distance to nearby grids, designed for ham radio rovers.

## Active Components
| Mfr         | Mfr Part No     | Digi-Key Part No | Reference | Qty   | USD$  | Ext$   | Description  |
| ----------- | --------------- | ---------------- | --------- | :---: | ----- | ------ | ------------ |
| Maxim       | DS1804-050+     | DS1804-050+-ND   | U1        |  1    |  4.30 |   4.30 | IC Digital Pot 50KOHM 100-tap |
| Adafruit    | 3857            | 1528-2648-ND     | U2        |  1    | 22.95 |  22.95 | Feather M4 Express |
| Microchip   | MIC5239-5.0YS   | MIC5239-5.0YS-ND | U3        |  1    |  2.79 |   2.79 | IC Linear Regulator 5v 500mA |
| TI          | LM386N-3/NOPB   | 296-43959-5-ND   | U4        |  1    |  1.24 |   1.24 | Audio Amp, Mono LM386 700MW |
| Adafruit    | 746             | 1528-1153-ND     | U5        |  1    | 39.95 |  39.95 | Ultimate GPS, 66 channel (do not buy part #4279) |
| Adafruit    | 3966            | 1528-2733-ND     | U6        |  1    |  9.95 |   9.95 | BMP-388 Barometric Pressure |
| Adafruit    | 1743            | 1528-2614-ND     | U7        |  1    | 29.95 |  29.95 | TFT Display  |
|             |                 |                  | **total** | **6** |       | **111.13** |     |

## Passive Components
| Mfr         | Mfr Part No     | Digi-Key Part No     | Reference          |  Qty   | USD$  | Ext$   | Description  |
| ----------- | --------------- | -------------------- | ------------------ | :----: | ----- | ------ | ------------ |
| Panasonic   | ECE-A1EKA470I   | P19582CT-ND          | C1, C4, C5, C7, C8 |    5   |  0.25 |   1.25 | CAP Electrolytic 47uF 25v radial |
| KEMET       | C320C104K5R5TA  | 399-4264-ND          | C2, C3, C6, C9     |    4   |  0.24 |   0.96 | CAP Ceramic 0.1uF 50V radial |
| ON Semi     | 1N4001G         | 1N4001GOS-ND         | D1, D2, D3         |    3   |  0.21 |   0.63 | Gen Purpose Diode 50V 1A |
| Vishay      | 1.5KE18CA       | 1.5KE18CA-E3/51GI-ND | D4                 |    1   |  1.33 |   1.33 | Zener Diode 25.2v Clamp |
| Adafruit    | 4445            | 3351-ND              | LS1                |    1   |  3.95 |   3.95 | Speaker, 3W 4ohm |
| Yageo       | FKN2WSJT-73-22R | 22AUCT-ND            | R1                 |    1   |  0.47 |   0.47 | RES 22-ohm 2W 5% axial |
| Yageo       | CFR-25JB-52-82K | 82KQBK-ND            | R2                 |    1   |  0.10 |   0.10 | RES 82K 1/4W 5% axial |
| Yageo       | CFR-25JB-52-10K | CFR-25JB-52-10K      | R3                 |    1   |  0.10 |   0.10 | RES 10K 1/4W 1% axial |
| Yageo       | MFR-25FBF52-10R | 10.0XBK-ND           | R4                 |    1   |  0.10 |   0.10 | RES 10-ohm 1/4W 1% axial |
| Stackpole   | CF14JT100R      | CF14JT100RCT-ND      | R5                 |    1   |  0.10 |   0.10 | RES 100-ohm 1/4W 5% axial |
|             |                 |                      | **total**          | **16** |       | **8.99** |     |

## Hardware
| Mfr         | Mfr Part No     | Digi-Key Part No | Reference     |  Qty  | USD$  | Ext$   | Description  |
| ----------- | --------------- | ---------------- | ------------- | :---: | ----- | ------ | ------------ |
| Panasonic   | CR1220          | P033-ND          | B1            |   1   |  1.00 |   1.00 | Lithium Battery 3-volt Coin Cell |
| Keystone    | 1056            | 36-1056-ND       | BT1           |   1   |  1.50 |   1.50 | Battery Holder (Open) 12.5 mm |
| CUI Devices | PJ-102A         | CP-102A-ND       | J1 jack       |   1   |  0.64 |   0.64 | Connector Power Jack 2X5.5 mm   |
| CUI Devices | PP3-002A        | CP3-1000-ND      | J1 plug       |   1   |  1.29 |   1.29 | Connector Power Plug 2.1X5.5 mm |
| Amphenol    | YO0421500000G   | 609-3920-ND      | J2            |   1   |  1.01 |   1.01 | Connector Screw Terminal 4pin   |
| CUI Devices | SJ1-3545N       | CP1-3545N-ND     | J3 jack       |   1   |  1.31 |   1.31 | Connector 3.5mm Audio Stereo Jack |
| CUI Devices | MP3-3501        | MP3-3501         | J3 plug       |   1   |  1.22 |   1.22 | Connector 3.5mm Audio Mono Plug |
| Sullins     | PPTC201LFBN-RC  | S7018-ND         |               |   1   |  1.23 |   2.46 | Connector 20-pos 0.1 Tin (LCD)  |
| Sullins     | PPTC021LFBN-RC  | S7000-ND         |               |   2   |  0.32 |   0.64 | Connector 2-pos 0.1 Tin (LCD)   |
| Keystone    | 4952            | 36-4952-ND       | TP1-3, TP_GND |   6   |  0.22 |   1.32 | PC Test Point Loop |
| JLC PCB     |                 | n/a              | PCB           |   1   |  6.00 |   6.00 | Printed Circuit Board |
| Polycase    | QS-50MBT        | n/a              |               |   1   | 15.00 |  15.00 | Plastic Case, Custom 4.5 x 3.5 x 1.25 |
|             |                 |                  | **total**     |       |       | **41.36** |     |


## Packaging
| Mfr         | Mfr Part No     | Digi-Key Part |  Qty  | USD$  | Ext$  | Description  |
| ----------- | --------------- | ------------- | :---: | ----- | ----- | ------------ |
| Desco       | 13879           | 16-1344-ND    |   4   |  0.12 |  0.48 | Antistatic bag, 3" x 5" |
| Desco       | 12250           | 16-1231-ND    |   1   |  0.06 |  0.06 | Antistatic foam, 1/4" 1" x 2" |
| Aviditi     | M752            | n/a           |   1   |  0.79 |  0.79 | White easy-fold box, 7 x 5 x 2 |
| Beadaholic  | ZDI-0203        | n/a           |   1   |  0.05 |  0.05 | Bag for screws 2x3" |
| Alin        |                 | n/a           |   1   |  0.19 |  0.19 | Cushion foam pouch 7.5 x 12" |
|             |                 |           | **total** |       | **1.53** |      |

