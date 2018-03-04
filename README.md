# MSD
Arduino SAMD USB MSC device

this is USB Mass Storage Class (MSC) Device for Arduino based on M0 (ARM SAMD21).

The MSC_ class is based on Arduino's PluggableUSBModule to reuse available USB code.  

NOTES:

The device works and partially comply to USB2.0 specs. I am working on full compliance.

It is already workable code. 
However it is not "just plug the library and play" at this time.
1. You should use fresh Arduino USB files from repo https://github.com/arduino/ArduinoCore-samd
2. Fix some issues in these files that are required for Mass Storage Class, if Arduino developers have not fixed them yet.
  I will provide list of patches in patches directory.

It is quite slow because SAMD21 supports only Full Speed USB 2.0.
So, when you plug this into your Windows computer it may take couple of minutes until it will be accepted by Windows as removable device.

The MSC interface can be compiled as composite interface together with CDC interface. It can be helpful if you need to see debug output in console.
Also you can undefine CDC_ENABLED in USB/USBDesc.h. This way your device will be just like a regular USB flash drive.

I am using Adafruit Feather M0 for development and testing so some pins are adjusted to this board.

I do not have SWD Debugger yet, that is why there is my_debug code. Basically it is just String that can be appended by something in interrupt handlers and printed to console later in loop(). This is because you cannot do Serial.print in interrupts.

I develop in Eclipse with Platformio. I found it is more convenient than Ardino IDE. Also it supports debugger device that I plan to use further. Platformio organizes includes its own way, so if an include cannot be resolved in Arduino IDE or other env try to modify it accordingly. 
