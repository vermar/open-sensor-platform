OSP Hello World
===============

Open Sensor Platform (OSP) is open source software built to enable
systems with sensor hubs. This repository is a fork of the open-sensor-platform on GitHub. The intent is to use the OSP framework for something other than what it was meant for since it is as good a framework as any to start building your fun projects!

I too am using the Keil-MDK toolchain for now and hope that someone will take the initiative to convert the projects to GCC or other ST supported Tool chains.

The project uses the Keil RTX Real Time Operating system and libraries with and without CPU profiling addition is provided. I am working on getting CMSIS-RTOS API port for the ASF and hopefully that will also help anyone using GCC or other toolchains/IDEs to compile it without dependency on Keil.

### Additional Questions

Refer to the detailed [FAQ](https://github.com/sensorplatforms/open-sensor-platform/wiki) on the wiki section of the original project.


### Project Structure
  * embedded - various project code 
  * external - Source code, library, documentation, algorithms, etc. from external sources available under open-source license
