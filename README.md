# STM32 "Black Pill" Blinky

This project simply blinks the LED of an STM32F401CEU6 variant of the "Black Pill" board using mostly common, open source tooling in linux.

The easiest way to get started with the Black Pill is install a copy of ST's CubeMX and use that to set up the project. This
will give you a good starting point with the proper config, oscillator and divider settings and HAL imports out of the gate.

We are using an ST Link V2 serial wire debug (SWD) adapter for flashing and debugging the code together with OpenOCD, GCC and
GDB.

## Prerequisites

You'll need GCC, OpenOCD and GDB including gcc-arm-none-eabi gdb-multiarch for your distro.


## Key facts about the Black Pill (STM32F401CEU6)

- Flash: 512 KB
- SRAM: 96 KB
- I/O: 36
- Max clock frequency: 84 MHz
- Flash base address: 0x08000000

A typical board uses an external 25 MHz crystal.


## Wiring (ST-Link → Black Pill via SWD)

| ST-Link | Black Pill |
| ------- | ---------- |
| SWDIO   | PA13       |
| SWCLK   | PA14       |
| GND     | GND        |
| 3.3V    | 3.3V       |
| NRST    | NRST       |

Make sure the board is powered either via the ST-Link or USB and BOOT0 is connected to GND (normal flash boot).


## Compiling the code

Run `make build` to compile the code


## Flashing the MCU

After a successful compilation:

1. press and hold the `BOOT0` button on the black pill.
2. While continuing to hold down `BOOT0`, press the `NRST` button once, then run `make flash` to flash the binary onto the MCU.
3. Once that is finished, release the `BOOT0` button.
4. Finally press the `NRST` button once more and your new binary should run on the MCU.
   
