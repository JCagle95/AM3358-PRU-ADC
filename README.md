# AM3358 Programmable Realtime Unit for SwampSat II 

This repository hosted files related to the development of SwampSat II, which started in 2014 as an undergraduate design project at University of Florida 
and later launched to space on 2019 in NASA's Undergraduate Student Instrument Project (USIP).

The primary on-board computer (OBC) is a custom made device based on Texas Instrument's AM3358 ARM-cortex microprocessor. 
The OBC contains 2 programmable real-time unit (PRU) for developing real-time processes when precise jobs are required. 

In the SwampSat II mission, we utilize the PRU as the primary interface that communicate with a analog-to-digital converter to achieve a 100kHz sampling of Very-Low-Frequency (VLF) wave in ionosphere.

Hopefully this repository provide people with basic understanding of how a PRU can be setup and how the assembly coding works and how to transfer data between PRU and onboard C program for data storage. 
