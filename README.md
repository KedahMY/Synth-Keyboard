# Synth-Keyboard

A self-contained Compact Synth Keyboard powered by two STM32F103VET6 microcontrollers. It features 24 tactile keys with real-time audio synthesis, a 4-track looper with SD card persistence, a 240×320 LCD UI, and a 64×32 RGB LED matrix that displays falling notes synchronized to the music.

## Project Structure

The repository is split into two distinct STM32CubeIDE projects, one for each microcontroller:

- **`Synced_Keyboard/` (Board 1)**: Audio synthesis, 4x8 keypad scanning, 4-track looper, SD card persistence, and LCD UI.
- **`Board2_Display/` (Board 2)**: 64×32 HUB75 RGB LED matrix showing falling notes synchronized to the music played on Board 1.

## Hardware & Architecture
P
- **MCUs**: 2x STM32F103VET6 (ARM Cortex-M3, 72 MHz, 512 KB Flash, 64 KB SRAM)
- **Audio synthesis**: Wavetable oscillator with selectable sample rates (22–96 kHz) via DAC1, utilizing double-buffered DMA.
- **Main UI Display**: 240x320 LCD connected via FSMC 16-bit parallel bus.
- **Visualizer Matrix**: HUB75 64×32 RGB LED matrix, driven by GPIO bit-bang and TIM2 ISR (1920 Hz).
- **Inter-Board Communication**: The boards communicate over a unidirectional UART connection (USART1, 115200 baud). Board 1 sends 9-byte `NoteEvent` packets, and Board 2 receives them via DMA with idle-line detection.

## Build Instructions

Both boards use **STM32CubeIDE** (Eclipse CDT + GNU MCU toolchain, arm-none-eabi-gcc).

1. Open STM32CubeIDE and import `Synced_Keyboard/` and `Board2_Display/` as existing projects.
2. Build each project using `Project → Build Project` (or `Ctrl+B`).
3. Flash and debug using the included `.launch` files (`Synced_Keyboard Debug.launch` and `Board2_Display Debug.launch`) with an ST-Link probe.

*Note: Peripheral configuration is managed via `.ioc` files using STM32CubeMX. If you regenerate the code, ensure custom logic remains safely inside the `/* USER CODE BEGIN/END */` guards.*

