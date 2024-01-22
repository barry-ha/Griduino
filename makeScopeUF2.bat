%echo off
echo Batch job to prepare TFT_Touch_Scope pre-compiled binary UF2 file
echo Usage:     makeUF2  release
echo Example:   makeUF2  Barry
echo .

cd %USERPROFILE%
cd Documents\Arduino\Griduino
py uf2conv.py -c -b 0x4000 -o downloads/Scope%1.uf2  build/adafruit.samd.adafruit_feather_m4/Scope.ino.bin

echo Done.