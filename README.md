OSP Hello World
===============
## Update: 07-July-2024

Various updates have been going on for a while now. Few new ports have been added and more will come soon. The main update here is the support for Keil MDK-ARM's free community version. However, the community licensing does not support the Version 5 compiler (which was probably the best in terms of optimization and tight code generation). It only has the ARMCLANG version 6 (currently at 6.22) and the MP3 project is the current reference for the new MDK-ARM 5.40. Additionally:
 - ST's USB Host Stack updated from latest pack
 - STM32F4xx HAL updated to latest
 - F4's I2C driver implementation updated (in `mondules/bus-drivers/STM32F4-CM4`)
 - Various compiler (ARMCLANG) specific adaptation done in various places
 - Most STM32 projects should be importable into the STM32CubeIDE and compilable with GCC

## Update: 01-Oct-2016

GCC Support is here! Happy to announce that I was successfully able to adapt the Discovery_F4 MP3 project to Atollic TrueSTUDIO ARM v6.0.0 Lite version.
Based on what Google is showing with some preliminary searching, this maybe the first project that provides a reference for CMSIS-RTX with GCC working on an STM32 device.

## Whats New In This Branch

This branch implements CMSIS-RTX adaptation of ASF and corresponding changes in the Discovery_F4 MP3 application.

It is a first step towards using open source/ free toolchain (CMSIS-RTX is compatible with GCC). Work in progress on getting the project to compile on Atollic TrueSTUDIO Lite (free!) IDE.