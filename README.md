# REACT ASPLOS 2024 Artifact Evaluation
REACT is a dynamic energy management platform for batteryless systems, described in detail in our ASPLOS 2024 paper "Energy-adaptive Buffering for Efficient, Responsive, and Persistent Batteryless Systems".
The primary artifact is a hardware circuit, with implementation details described in the paper; this repository contains the source code for the software component of REACT and the baseline systems running on the MSP430FR5994, application benchmarks, and our recorded RF power traces.

## Software pre-requisites

All source code is provided as Code Composer Studio 11 projects. CCS can be downloaded at https://www.ti.com/tool/CCSTUDIO. By importing each directory as a project into CCS, you can explore the source code and compile binaries to flash to a target system.

## Hardware pre-requisites

Full system evaluation requires building the energy management circuit described in the paper and porting the code (i.e., setting the corresponding GPIO pins) according to the components used and their configuration.
All software was developed for and tested on the Texas Instruments MSP430FR5994 using the MSP-EXP430FR5994 development kit.

## RF Power Traces

Part of our evaluation of REACT uses RF power traces recorded in our workspace, an active office environment.
The traces are based on the Powercast P2110B, a commercial 915 MHz energy harvesting circuit using a dipole antenna which rectifies incoming energy from a 3W, vertically polarized transmitter.
We use the integrated power measurement feature of the harvesting chip to determine received signal strength.
We evaluate three RF traces, with raw data available in the `traces` directory:

- **rf-cart.trace**: The harvester is on a small cart in the plane of the transmitter, repeatedly moving towards, away from, and around it.
- **rf-mobile.trace**: The transmitter is mounted in the upper corner of a room, facing down towards the center of the room. The harvester is carried through, around, and out of the room.
- **rf-obstruction.trace**: The harvester is stationary on a desk approximately 1 meter from and in the plane of the transmitter. Various obstructions (laptops, metal water bottles, etc.) are periodically moved between and around the transmitter and harvester.
