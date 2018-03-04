AH=/home/mark/eclipse-workspace-pio/MSD/Arduino/ArduinoCore-samd-master/cores/arduino/USB
MY=/home/mark/.platformio/packages/framework-arduinosam/cores/adafruit/USB
diff -Naur $AH/USBAPI.h $MY/USBAPI.h >USBAPI.h.patch
diff -Naur $AH/USBCore.h $MY/USBCore.h >USBCore.h.patch
diff -Naur $AH/USBCore.cpp $MY/USBCore.cpp >USBCore.cpp.patch

