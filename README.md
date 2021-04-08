# Modules
This repo is for native modules in MicroPython for board Ophyra by Intesc

## How build modules:

  Edit in ports/stm32/boards/OPHYRA the file mpconfigboard.h
  
  Add this macros:
``` 
  #define MODULE_OPHYRA_MPU60_ENABLED (1)
  #define MODULE_OPHYRA_EEPROM_ENABLED (1)
  #define MODULE_OPHYRA_BOTONES_ENABLED   (1)
  #define MODULE_OPHYRA_HCSR04_ENABLED    (1)
  #define MODULE_OPHYRA_TFTDISP_ENABLED   (1)
```
Remember the folder modules and micropython should be in the same directory.
In bash terminal execute the following command:

```
  make BOARD=OPHYRA USER_C_MODULES=../../../modules -j8 
```
