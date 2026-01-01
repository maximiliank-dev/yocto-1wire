# Readme

# Installation

## Yocto Build instructions
1. Install the required Yocto [Dependencies](https://docs.yoctoproject.org/5.0.14/ref-manual/system-requirements.html)
2. source poky/oe-init-build-env
3. bitbake core-image-minimal
4. flash the core-image-minimal-raspberrypi3.rootfs.wic.bz2 using bmaptool on a sdcard and boot the sdcard in the raspberry pi


## Client software 
Recommended Python version: Python 3.12
1. pip install PySide6
2. cd client-software
3. python main.py
4. set the ip address in the address field and click connect -> if successful the GUI should change from yellow to green
5. enable the CRC-check checkbox
6. click the Read temperature button -> after some time the plot should update and add a new temperature point


# Architecture
The Project is a control unit for a temperature sensor (DS18B20). The sensor send the temperature data to a Raspberry Pi, which processes it and sends the data to an external device. The system uses a Linux kernel module to interface with the sensor. Then a daemon is used to trigger reading from the sensor, processes the data and sends the data to the host.
For the external device there is a Python software written in QT to visualize the temperature received temperature. The GUI communicates with the raspberry pi via TCP.

The software has the following **architecture**:


<img src="./doc/architecture2.drawio.svg">


