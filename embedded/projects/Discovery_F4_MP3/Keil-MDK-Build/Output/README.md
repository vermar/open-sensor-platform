ASF Based MP3 Player
====================

## How to use it:
- Flash the HEX file using STM32 ST-Link Utility from st.com
- Connect a USB flash drive to the USB-OTG (USB micro) connector on the board using a USB-Micro-OTG cable (search for 'usb otg cable' on Amazon)
- Flash drive is expected to be formatted with FAT32 and contain MP3 files in either root folder and/or in a folder called 'Music'
- Connect a FTDI USB-Serial cable (3.3V) on PD8 (RX of FTDI cable), PD9 (TX of FTDI cable) & GND. The serial port setting is 921600-8-N-1.
- Blinking blue LED on the board indicates data being read from the flash drive.
- On the serial console if you press Enter it will display the CPU+Stack usage statistics.
- NOTE: Headphone volume is set at 80% so it might be loud for an headset.
- Sources coming soon... with due credits to persons/sites that got me started on this!

## Known Limitations
- Single channel files do not play properly
