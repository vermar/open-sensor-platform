STM32 Discovery-F4 MP3 Player
=============================
## Features and Enhancements
This project was inspired by the works of Benjamin Vedder [http://vedder.se/2012/12/stm32f4-discovery-usb-host-and-mp3-player/] and Marco W. [http://www.mikrocontroller.net/topic/252319]. I was able to make the following enhancements on their work:
- Based on [ASF (Application Support Framework)](https://github.com/sensorplatforms/open-sensor-platform/wiki/ASF) from Open Sensor Platform project.
- Multitasking support with Keil-RTX
- ASF provides CPU usage and Stack usage statistics (just hit Enter on serial console screen to see the statistics)
- Updated USB Host stack from ST
- Updated FatFs file system
- MP3 decoder runs as a task instead of running from ISR
- Fixed issues with ID3 tag parsing that caused files to be skipped
- USB Drive events and handling made robust so that insertion and removal of flash drives do not crash or hang the system.
- Helix open source MP3 decoder with support for MPEG layer 3 decoder and optimized for fixed point processors (highly optimized for ARM processors).
  * MPEG1, MPEG2, and MPEG2.5 (low sampling frequency extensions)
  * constant bitrate, variable bitrate, and free bitrate modes
  * mono and all stereo modes (normal stereo, joint stereo, dual-mono) [Note that mono files (1ch) implementation does not work properly on this platform. This is a platform issue and not decoder issue]

## How to use it
- Build and Flash the project on an STM32 Discovery-F4 board.
- Connect a USB flash drive to the USB-OTG (USB micro) connector on the board using a USB-Micro-OTG cable (search for 'usb otg cable' on Amazon)
- Flash drive is expected to be formatted with FAT32 and contain MP3 files in either root folder and/or in a folder called 'Music'
- Connect a FTDI USB-Serial cable (3.3V) on PD8 (RX of FTDI cable), PD9 (TX of FTDI cable) & GND. The serial port setting is 921600-8-N-1.
- Blinking blue LED on the board indicates data being read from the flash drive.
- On the serial console if you press Enter it will display the CPU+Stack usage statistics.
- The Blue user key on Discovery-F4 board will skip the track currently playing.

## Limitations
- Currently only Keil MDK toolchain is supported and due to the size of the project a fully licensed version is needed for building it. It is possible to port it to other toolchains that are free but it would also require having compatible RTX libraries or using CMSIS-RTOS with FreeRTOS - something I am planning to work on in the near future.
