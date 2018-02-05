# MSD
Arduino SAMD USB MSC device

this is USB Mass Storage Class (MSC) Device for Arduino based on M0 (ARM SAMD21).

The MSC_ class is based on Arduino's PluggableUSBModule to reuse available USB code.  

NOTES:

IT IS NOT WORKABLE CODE YET! it is in development...

I am using Adafruit Feather M0 for development and testing so some pins are adjusted to this board.

I do not have SWD Debugger yet, that is why there is my_debug code. Basically it is just String that can be appended by something in interrupt handlers and printed to console later in loop(). This is because you cannot do Serial.print in interrupts.

I develop in Eclipse with Platformio. I found it is more convenient than Ardino IDE. Also it supports debugger device that I plan to use further. Platformio organizes includes its own way, so if an include cannot be resolved in Arduino IDE or other env try to modify it accordingly. 

Currently SD functionality is not used, until all issues on USB/MSC/SCSI levels resolved. 
Writes are not implemented. 
Reads return 'ABCDE...' array.
