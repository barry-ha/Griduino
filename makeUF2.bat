%echo off
echo Batch job to prepare Griduino pre-compiled binary UF2 file
echo Usage:     makeUF2  release
echo Example:   makeUF2  1.13.1
echo .

cd %USERPROFILE%
cd Documents\Arduino\Griduino
py uf2conv.py -c -b 0x4000 -o downloads/griduino_v%1.uf2  build/adafruit.samd.adafruit_feather_m4/Griduino.ino.bin

echo Done.