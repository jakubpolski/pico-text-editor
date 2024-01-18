## A simple text editor I made for a school project

__As of the setup:__
- It's meant to be compiled with VSCode shipped with Pico SDK
- Keyboard is connected to the main microUSB port, through an OTG adapter
- It uses an external 5V power supply connected to VBUS
- [The display](https://wiki.seeedstudio.com/Grove-16x2_LCD_Series/) is connected to GP21,GP20 pins, and is powered by the same power supply
- After compiling pico can be programmed by running __copy_to_pico.ps1__ with PowerShell, when pico is in flash mode or by flashing the __main.uf2__ file by yourself (located in /build/src)

__As of the editor itself:__
- It stores all files in pico's internal memory
- It supports most of the keys except F1-F12 and shortcuts
