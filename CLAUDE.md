# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

A dual-MCU synthesizer keyboard using two STM32F103VET6 (Cortex-M3, 72 MHz, 512 KB Flash, 64 KB SRAM) boards that communicate over UART:

- **Board 1 (`Synced_Keyboard/`)** — Audio synthesis, 4x8 keypad scanning, 4-track looper, SD card persistence, 240×320 LCD UI
- **Board 2 (`Board2_Display/`)** — 64×32 HUB75 RGB LED matrix showing falling notes synchronized to music played on Board 1

## Build System

Both boards use **STM32CubeIDE** (Eclipse CDT + GNU MCU toolchain, arm-none-eabi-gcc). There are no standalone Makefiles — the build is managed by the IDE's managed build system driven by `.cproject`.

- Open `Synced_Keyboard/` or `Board2_Display/` as a project in STM32CubeIDE
- Build: `Project → Build Project` (or Ctrl+B)
- Debug: use the included `.launch` files (`Synced_Keyboard Debug.launch`, `Board2_Display Debug.launch`) with an ST-Link probe
- Peripheral/pin configuration is in the `.ioc` files (STM32CubeMX); regenerating from CubeMX will overwrite `main.c` user code sections — keep custom code inside `/* USER CODE BEGIN/END */` guards

## Architecture

### Inter-Board Protocol

Defined in [Synced_Keyboard/Core/Inc/note_event.h](Synced_Keyboard/Core/Inc/note_event.h) and mirrored in [Board2_Display/Core/Inc/note_event.h](Board2_Display/Core/Inc/note_event.h). Both copies must stay in sync.

A `NoteEvent` is a 9-byte packet: `0xAA` header, note name (0–6), accidental, octave (2–7), velocity (0=off, 100=on), duration (ms), track ID (0=live, 1–3=looper), active flag, and a checksum. Sent at 115200 baud; Board 2 receives via DMA with idle-line detection.

### Board 1 — Audio Engine

| File | Role |
|------|------|
| [Core/Src/audio.c](Synced_Keyboard/Core/Src/audio.c) | Wavetable oscillator; phase accumulator; selectable sample rates (22–96 kHz) via DAC+DMA double-buffer (512 samples/half) |
| [Core/Src/wavetable.c](Synced_Keyboard/Core/Src/wavetable.c) | 256-sample, 12-bit sine/square/triangle lookup tables |
| [Core/Src/keydef.c](Synced_Keyboard/Core/Src/keydef.c) | 4×8 column-scan keypad with 3-tick (~30 ms) software debounce |
| [Core/Src/song_player.c](Synced_Keyboard/Core/Src/song_player.c) | Pre-programmed riff sequencer |
| [Core/Src/sd_player.c](Synced_Keyboard/Core/Src/sd_player.c) | SD card `.bin` file playback via FATFS |
| [Core/Src/lcd.c](Synced_Keyboard/Core/Src/lcd.c) | 240×320 LCD via FSMC 16-bit parallel bus |
| [Core/Src/uart_tx.c](Synced_Keyboard/Core/Src/uart_tx.c) | Blocking NoteEvent TX to Board 2 |

Key hardware: DAC1 (PA4) for audio, TIM8 as DAC DMA trigger, USART1 (PA9/PA10), SDIO (1-bit), FSMC Bank 1 for LCD.

### Board 2 — Display Engine

| File | Role |
|------|------|
| [Core/Src/hub75.c](Board2_Display/Core/Src/hub75.c) | HUB75 64×32 driver; GPIO bit-bang; double-buffered 4 KB framebuffer; TIM2 ISR at 1920 Hz (32 rows × 60 fps) |
| [Core/Src/animation.c](Board2_Display/Core/Src/animation.c) | Falling-note animation; up to 32 concurrent notes; notes spawn at bottom and scroll up |
| [Core/Src/uart_rx.c](Board2_Display/Core/Src/uart_rx.c) | DMA + idle-line detection for NoteEvent RX |
| [Core/Src/sim_events.c](Board2_Display/Core/Src/sim_events.c) | Compile-time test mode that generates NoteEvents without Board 1 |

Key hardware: TIM2 (1920 Hz row-scan ISR), USART1 RX with DMA. HUB75 color/row/control lines are on GPIOA and GPIOB (see [Core/Inc/hub75.h](Board2_Display/Core/Inc/hub75.h) for pin mapping). Direct 3.3V drive — no level shifter.

## Key Conventions

- Audio sample buffers use DMA double-buffering; the half-complete and full-complete callbacks in `main.c` fill alternate halves — do not block inside them.
- The HUB75 framebuffer is 3-bit RGB (one byte per pixel, low 3 bits used); color constants are in [Core/Inc/hub75_color.h](Board2_Display/Core/Inc/hub75_color.h).
- Keypad columns are driven on GPIOB; rows are read on GPIOC (rows 0–3) and GPIOA (row 3).
- Both projects use `-O2` (Debug) / `-Os` (Release) optimization; timing-sensitive code (ISRs) should be kept minimal.

## graphify

This project has a graphify knowledge graph at graphify-out/.

Rules:
- Before answering architecture or codebase questions, read graphify-out/GRAPH_REPORT.md for god nodes and community structure
- If graphify-out/wiki/index.md exists, navigate it instead of reading raw files
- For cross-module "how does X relate to Y" questions, prefer `graphify query "<question>"`, `graphify path "<A>" "<B>"`, or `graphify explain "<concept>"` over grep — these traverse the graph's EXTRACTED + INFERRED edges instead of scanning files
- After modifying code files in this session, run `graphify update .` to keep the graph current (AST-only, no API cost)
