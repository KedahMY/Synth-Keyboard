# Graph Report - .  (2026-05-05)

## Corpus Check
- Large corpus: 194 files · ~824,023 words. Semantic extraction will be expensive (many Claude tokens). Consider running on a subfolder, or use --no-semantic to run AST-only.

## Summary
- 1628 nodes · 2487 edges · 58 communities detected
- Extraction: 100% EXTRACTED · 0% INFERRED · 0% AMBIGUOUS
- Token cost: 0 input · 0 output

## Community Hubs (Navigation)
- [[_COMMUNITY_Dir Lock|Dir Lock]]
- [[_COMMUNITY_Uart Receive|Uart Receive]]
- [[_COMMUNITY_Sdmmc Sdio|Sdmmc Sdio]]
- [[_COMMUNITY_Readblocks Writeblocks|Readblocks Writeblocks]]
- [[_COMMUNITY_Timex Hallsensor|Timex Hallsensor]]
- [[_COMMUNITY_Tim Base|Tim Base]]
- [[_COMMUNITY_Nvic Itm|Nvic Itm]]
- [[_COMMUNITY_Nvic Itm|Nvic Itm]]
- [[_COMMUNITY_Nvic Clearpendingirq|Nvic Clearpendingirq]]
- [[_COMMUNITY_Nvic Clearpendingirq|Nvic Clearpendingirq]]
- [[_COMMUNITY_Tim Start|Tim Start]]
- [[_COMMUNITY_Dbgmcu Disabledbgsleepmode|Dbgmcu Disabledbgsleepmode]]
- [[_COMMUNITY_Init Lcd|Init Lcd]]
- [[_COMMUNITY_Dac Start|Dac Start]]
- [[_COMMUNITY_Sram Dma|Sram Dma]]
- [[_COMMUNITY_Nvic Mpu|Nvic Mpu]]
- [[_COMMUNITY_Syscalls Close|Syscalls Close]]
- [[_COMMUNITY_Pwr Configpvd|Pwr Configpvd]]
- [[_COMMUNITY_Lcd Read|Lcd Read]]
- [[_COMMUNITY_Mpu Arm|Mpu Arm]]
- [[_COMMUNITY_Bsp Abortcallback|Bsp Abortcallback]]
- [[_COMMUNITY_Rcc Clockconfig|Rcc Clockconfig]]
- [[_COMMUNITY_Flash Program|Flash Program]]
- [[_COMMUNITY_Flash Flashex|Flash Flashex]]
- [[_COMMUNITY_Mspinit Mspdeinit|Mspinit Mspdeinit]]
- [[_COMMUNITY_Dma Abort|Dma Abort]]
- [[_COMMUNITY_Audio Dac|Audio Dac]]
- [[_COMMUNITY_Handler Irqhandler|Handler Irqhandler]]
- [[_COMMUNITY_Tim Init|Tim Init]]
- [[_COMMUNITY_Dacex Dac|Dacex Dac]]
- [[_COMMUNITY_Tim Setconfig|Tim Setconfig]]
- [[_COMMUNITY_Animation Queue|Animation Queue]]
- [[_COMMUNITY_Sdplayer Player|Sdplayer Player]]
- [[_COMMUNITY_Exti Clearconfigline|Exti Clearconfigline]]
- [[_COMMUNITY_Hub75 Init|Hub75 Init]]
- [[_COMMUNITY_Tim Capturecallback|Tim Capturecallback]]
- [[_COMMUNITY_Songplayer Song|Songplayer Song]]
- [[_COMMUNITY_Gpio Exti|Gpio Exti]]
- [[_COMMUNITY_Rccex Rcc|Rccex Rcc]]
- [[_COMMUNITY_Tim Setconfig|Tim Setconfig]]
- [[_COMMUNITY_Mpu Arm|Mpu Arm]]
- [[_COMMUNITY_Uart Irqhandler|Uart Irqhandler]]
- [[_COMMUNITY_Fatfs Gen|Fatfs Gen]]
- [[_COMMUNITY_Keypad Keydef|Keypad Keydef]]
- [[_COMMUNITY_System Systeminit|System Systeminit]]
- [[_COMMUNITY_Sysmem Sbrk|Sysmem Sbrk]]
- [[_COMMUNITY_Tim Pwm|Tim Pwm]]
- [[_COMMUNITY_Tim Triggerhalfcpltcallback|Tim Triggerhalfcpltcallback]]
- [[_COMMUNITY_Tim Periodelapsedhalfcpltcallback|Tim Periodelapsedhalfcpltcallback]]
- [[_COMMUNITY_Synth-Keyboard|Synth-Keyboard]]
- [[_COMMUNITY_Apache|Apache]]
- [[_COMMUNITY_Software Component|Software Component]]
- [[_COMMUNITY_Software Component|Software Component]]
- [[_COMMUNITY_Configuration|Configuration]]
- [[_COMMUNITY_Apache|Apache]]
- [[_COMMUNITY_Software Component|Software Component]]
- [[_COMMUNITY_Software Component|Software Component]]
- [[_COMMUNITY_Synced Keyboard|Synced Keyboard]]

## God Nodes (most connected - your core abstractions)
1. `TIM_CCxChannelCmd()` - 30 edges
2. `SDIO_SendCommand()` - 28 edges
3. `SDMMC_GetCmdResp1()` - 23 edges
4. `main()` - 22 edges
5. `move_window()` - 19 edges
6. `find_volume()` - 19 edges
7. `TIM_CCxNChannelCmd()` - 18 edges
8. `dir_sdi()` - 14 edges
9. `dir_register()` - 14 edges
10. `follow_path()` - 14 edges

## Surprising Connections (you probably didn't know these)
- `main()` --calls--> `MX_GPIO_Init()`  [EXTRACTED]
  C:/GitHub/Synth-Keyboard/Board2_Display/Core/Src/main.c → C:/GitHub/Synth-Keyboard/Synced_Keyboard/Core/Src/main.c
- `main()` --calls--> `MX_DMA_Init()`  [EXTRACTED]
  C:/GitHub/Synth-Keyboard/Board2_Display/Core/Src/main.c → C:/GitHub/Synth-Keyboard/Synced_Keyboard/Core/Src/main.c
- `main()` --calls--> `MX_SDIO_SD_Init()`  [EXTRACTED]
  C:/GitHub/Synth-Keyboard/Board2_Display/Core/Src/main.c → C:/GitHub/Synth-Keyboard/Synced_Keyboard/Core/Src/main.c
- `main()` --calls--> `LCD_ShowOctave()`  [EXTRACTED]
  C:/GitHub/Synth-Keyboard/Board2_Display/Core/Src/main.c → C:/GitHub/Synth-Keyboard/Synced_Keyboard/Core/Src/main.c
- `main()` --calls--> `LCD_ShowSong()`  [EXTRACTED]
  C:/GitHub/Synth-Keyboard/Board2_Display/Core/Src/main.c → C:/GitHub/Synth-Keyboard/Synced_Keyboard/Core/Src/main.c

## Communities (152 total, 21 thin omitted)

### Community 2 - "Dir Lock"
Cohesion: 0.09
Nodes (75): check_fs(), chk_chr(), chk_lock(), clear_lock(), clmt_clust(), clust2sect(), cmp_lfn(), create_chain() (+67 more)

### Community 3 - "Uart Receive"
Cohesion: 0.08
Nodes (67): HAL_HalfDuplex_EnableReceiver(), HAL_HalfDuplex_EnableTransmitter(), HAL_HalfDuplex_Init(), HAL_LIN_Init(), HAL_LIN_SendBreak(), HAL_MultiProcessor_EnterMuteMode(), HAL_MultiProcessor_ExitMuteMode(), HAL_MultiProcessor_Init() (+59 more)

### Community 5 - "Sdmmc Sdio"
Cohesion: 0.1
Nodes (36): SDIO_GetCommandResponse(), SDIO_GetResponse(), SDIO_SendCommand(), SDMMC_CmdAppCommand(), SDMMC_CmdAppOperCommand(), SDMMC_CmdBlockLength(), SDMMC_CmdBusWidth(), SDMMC_CmdErase() (+28 more)

### Community 6 - "Readblocks Writeblocks"
Cohesion: 0.08
Nodes (30): HAL_SD_Abort(), HAL_SD_Abort_IT(), HAL_SD_AbortCallback(), HAL_SD_ConfigWideBusOperation(), HAL_SD_DeInit(), HAL_SD_ErrorCallback(), HAL_SD_GetCardCSD(), HAL_SD_GetCardState() (+22 more)

### Community 7 - "Timex Hallsensor"
Cohesion: 0.11
Nodes (42): HAL_TIMEx_BreakCallback(), HAL_TIMEx_CommutCallback(), HAL_TIMEx_CommutHalfCpltCallback(), HAL_TIMEx_ConfigBreakDeadTime(), HAL_TIMEx_ConfigCommutEvent(), HAL_TIMEx_ConfigCommutEvent_DMA(), HAL_TIMEx_ConfigCommutEvent_IT(), HAL_TIMEx_GetChannelNState() (+34 more)

### Community 8 - "Tim Base"
Cohesion: 0.06
Nodes (42): HAL_TIM_Base_DeInit(), HAL_TIM_Base_GetState(), HAL_TIM_Base_MspDeInit(), HAL_TIM_Base_Start(), HAL_TIM_Base_Start_DMA(), HAL_TIM_Base_Start_IT(), HAL_TIM_Base_Stop(), HAL_TIM_Base_Stop_DMA() (+34 more)

### Community 14 - "Tim Start"
Cohesion: 0.13
Nodes (29): HAL_TIM_Encoder_Start(), HAL_TIM_Encoder_Start_DMA(), HAL_TIM_Encoder_Start_IT(), HAL_TIM_Encoder_Stop(), HAL_TIM_Encoder_Stop_DMA(), HAL_TIM_Encoder_Stop_IT(), HAL_TIM_IC_Start(), HAL_TIM_IC_Start_DMA() (+21 more)

### Community 15 - "Dbgmcu Disabledbgsleepmode"
Cohesion: 0.16
Nodes (25): HAL_DBGMCU_DisableDBGSleepMode(), HAL_DBGMCU_DisableDBGStandbyMode(), HAL_DBGMCU_DisableDBGStopMode(), HAL_DBGMCU_EnableDBGSleepMode(), HAL_DBGMCU_EnableDBGStandbyMode(), HAL_DBGMCU_EnableDBGStopMode(), HAL_DeInit(), HAL_Delay() (+17 more)

### Community 16 - "Init Lcd"
Cohesion: 0.18
Nodes (23): assert_failed(), Chord_RetuneAll(), CycleWave(), Error_Handler(), GetNoteFreq(), LCD_ShowOctave(), LCD_ShowSampleRate(), LCD_ShowSong() (+15 more)

### Community 18 - "Dac Start"
Cohesion: 0.11
Nodes (12): DAC_DMAConvCpltCh1(), DAC_DMAErrorCh1(), DAC_DMAHalfConvCpltCh1(), HAL_DAC_ConvCpltCallbackCh1(), HAL_DAC_ConvHalfCpltCallbackCh1(), HAL_DAC_DeInit(), HAL_DAC_DMAUnderrunCallbackCh1(), HAL_DAC_ErrorCallbackCh1() (+4 more)

### Community 19 - "Sram Dma"
Cohesion: 0.1
Nodes (9): HAL_SRAM_DeInit(), HAL_SRAM_DMA_XferCpltCallback(), HAL_SRAM_DMA_XferErrorCallback(), HAL_SRAM_Init(), HAL_SRAM_MspDeInit(), HAL_SRAM_MspInit(), SRAM_DMACplt(), SRAM_DMACpltProt() (+1 more)

### Community 23 - "Nvic Mpu"
Cohesion: 0.18
Nodes (20): HAL_MPU_ConfigRegion(), HAL_MPU_Disable(), HAL_MPU_DisableRegion(), HAL_MPU_Enable(), HAL_MPU_EnableRegion(), HAL_NVIC_ClearPendingIRQ(), HAL_NVIC_DisableIRQ(), HAL_NVIC_EnableIRQ() (+12 more)

### Community 24 - "Syscalls Close"
Cohesion: 0.19
Nodes (18): _close(), _execve(), _exit(), _fork(), _fstat(), _getpid(), initialise_monitor_handles(), _isatty() (+10 more)

### Community 25 - "Pwr Configpvd"
Cohesion: 0.2
Nodes (18): HAL_PWR_ConfigPVD(), HAL_PWR_DeInit(), HAL_PWR_DisableBkUpAccess(), HAL_PWR_DisablePVD(), HAL_PWR_DisableSEVOnPend(), HAL_PWR_DisableSleepOnExit(), HAL_PWR_DisableWakeUpPin(), HAL_PWR_EnableBkUpAccess() (+10 more)

### Community 26 - "Lcd Read"
Cohesion: 0.24
Nodes (19): Delay(), LCD_BackLed_Control(), LCD_Clear(), LCD_DrawBitmap24x32(), LCD_DrawChar(), LCD_DrawDot(), LCD_DrawEllipse(), LCD_DrawLine() (+11 more)

### Community 28 - "Mpu Arm"
Cohesion: 0.17
Nodes (13): ARM_MPU_ClrRegion(), ARM_MPU_ClrRegion_NS(), ARM_MPU_ClrRegionEx(), ARM_MPU_Load(), ARM_MPU_Load_NS(), ARM_MPU_LoadEx(), ARM_MPU_SetMemAttr(), ARM_MPU_SetMemAttr_NS() (+5 more)

### Community 29 - "Bsp Abortcallback"
Cohesion: 0.14
Nodes (8): BSP_SD_AbortCallback(), BSP_SD_Init(), BSP_SD_IsDetected(), BSP_SD_ReadCpltCallback(), BSP_SD_WriteCpltCallback(), HAL_SD_AbortCallback(), HAL_SD_RxCpltCallback(), HAL_SD_TxCpltCallback()

### Community 30 - "Rcc Clockconfig"
Cohesion: 0.26
Nodes (15): HAL_RCC_ClockConfig(), HAL_RCC_CSSCallback(), HAL_RCC_DeInit(), HAL_RCC_DisableCSS(), HAL_RCC_EnableCSS(), HAL_RCC_GetClockConfig(), HAL_RCC_GetHCLKFreq(), HAL_RCC_GetOscConfig() (+7 more)

### Community 34 - "Flash Program"
Cohesion: 0.28
Nodes (14): FLASH_Program_HalfWord(), FLASH_SetErrorCode(), FLASH_WaitForLastOperation(), FLASH_WaitForLastOperationBank2(), HAL_FLASH_EndOfOperationCallback(), HAL_FLASH_GetError(), HAL_FLASH_Lock(), HAL_FLASH_OB_Launch() (+6 more)

### Community 35 - "Flash Flashex"
Cohesion: 0.35
Nodes (14): FLASH_OB_DisableWRP(), FLASH_OB_EnableWRP(), FLASH_OB_GetRDP(), FLASH_OB_GetUser(), FLASH_OB_GetWRP(), FLASH_OB_ProgramData(), FLASH_OB_RDP_LevelConfig(), FLASH_OB_UserConfig() (+6 more)

### Community 36 - "Mspinit Mspdeinit"
Cohesion: 0.14
Nodes (5): HAL_FSMC_MspDeInit(), HAL_FSMC_MspInit(), HAL_MspInit(), HAL_SRAM_MspDeInit(), HAL_SRAM_MspInit()

### Community 37 - "Dma Abort"
Cohesion: 0.27
Nodes (13): DMA_SetConfig(), HAL_DMA_Abort(), HAL_DMA_Abort_IT(), HAL_DMA_DeInit(), HAL_DMA_GetError(), HAL_DMA_GetState(), HAL_DMA_Init(), HAL_DMA_IRQHandler() (+5 more)

### Community 38 - "Audio Dac"
Cohesion: 0.19
Nodes (9): Audio_FillHalf(), Audio_NoteOff(), Audio_NoteOn(), Audio_SetNote(), Audio_SetSampleRate(), Audio_Stop(), current_sr_hz(), HAL_DAC_ConvCpltCallbackCh1() (+1 more)

### Community 39 - "Handler Irqhandler"
Cohesion: 0.21
Nodes (9): BusFault_Handler(), DebugMon_Handler(), HardFault_Handler(), MemManage_Handler(), NMI_Handler(), PendSV_Handler(), SVC_Handler(), SysTick_Handler() (+1 more)

### Community 41 - "Tim Init"
Cohesion: 0.2
Nodes (14): HAL_TIM_Base_Init(), HAL_TIM_Base_MspInit(), HAL_TIM_Encoder_Init(), HAL_TIM_Encoder_MspInit(), HAL_TIM_IC_Init(), HAL_TIM_IC_MspInit(), HAL_TIM_OC_Init(), HAL_TIM_OC_MspInit() (+6 more)

### Community 42 - "Dacex Dac"
Cohesion: 0.18
Nodes (6): DAC_DMAConvCpltCh2(), DAC_DMAErrorCh2(), DAC_DMAHalfConvCpltCh2(), HAL_DACEx_ConvCpltCallbackCh2(), HAL_DACEx_ConvHalfCpltCallbackCh2(), HAL_DACEx_ErrorCallbackCh2()

### Community 44 - "Tim Setconfig"
Cohesion: 0.24
Nodes (12): HAL_TIM_IC_ConfigChannel(), HAL_TIM_OC_ConfigChannel(), HAL_TIM_OnePulse_ConfigChannel(), HAL_TIM_PWM_ConfigChannel(), TIM_OC1_SetConfig(), TIM_OC2_SetConfig(), TIM_OC3_SetConfig(), TIM_OC4_SetConfig() (+4 more)

### Community 45 - "Animation Queue"
Cohesion: 0.31
Nodes (8): animation_tick(), draw_keyboard_row(), octave_to_visual_block(), process_events(), queue_pop(), resolve_note_visual(), spawn_note(), stop_growing()

### Community 46 - "Sdplayer Player"
Cohesion: 0.29
Nodes (6): CloseFile(), SdPlayer_LoadNext(), SdPlayer_Play(), SdPlayer_Stop(), SdPlayer_Tick(), SdPlayer_Toggle()

### Community 47 - "Exti Clearconfigline"
Cohesion: 0.33
Nodes (9): HAL_EXTI_ClearConfigLine(), HAL_EXTI_ClearPending(), HAL_EXTI_GenerateSWI(), HAL_EXTI_GetConfigLine(), HAL_EXTI_GetHandle(), HAL_EXTI_GetPending(), HAL_EXTI_IRQHandler(), HAL_EXTI_RegisterCallback() (+1 more)

### Community 49 - "Tim Capturecallback"
Cohesion: 0.2
Nodes (10): HAL_TIM_IC_CaptureCallback(), HAL_TIM_IRQHandler(), HAL_TIM_OC_DelayElapsedCallback(), HAL_TIM_PeriodElapsedCallback(), HAL_TIM_PWM_PulseFinishedCallback(), HAL_TIM_TriggerCallback(), TIM_DMACaptureCplt(), TIM_DMADelayPulseCplt() (+2 more)

### Community 52 - "Rccex Rcc"
Cohesion: 0.39
Nodes (7): HAL_RCCEx_DisablePLL2(), HAL_RCCEx_DisablePLLI2S(), HAL_RCCEx_EnablePLL2(), HAL_RCCEx_EnablePLLI2S(), HAL_RCCEx_GetPeriphCLKConfig(), HAL_RCCEx_GetPeriphCLKFreq(), HAL_RCCEx_PeriphCLKConfig()

### Community 53 - "Tim Setconfig"
Cohesion: 0.28
Nodes (9): HAL_TIM_ConfigClockSource(), HAL_TIM_ConfigOCrefClear(), HAL_TIM_SlaveConfigSynchro(), HAL_TIM_SlaveConfigSynchro_IT(), TIM_ETR_SetConfig(), TIM_ITRx_SetConfig(), TIM_SlaveTimer_SetConfig(), TIM_TI1_ConfigInputStage() (+1 more)

### Community 55 - "Uart Irqhandler"
Cohesion: 0.38
Nodes (3): HAL_UART_RxCpltCallback(), HAL_UARTEx_RxEventCallback(), uart_rx_buffer_process()

### Community 59 - "Fatfs Gen"
Cohesion: 0.47
Nodes (4): FATFS_LinkDriver(), FATFS_LinkDriverEx(), FATFS_UnLinkDriver(), FATFS_UnLinkDriverEx()

### Community 61 - "Keypad Keydef"
Cohesion: 0.6
Nodes (3): Keypad_AllColumnsOff(), Keypad_PushEvent(), Keypad_ScanOneColumn()

### Community 62 - "System Systeminit"
Cohesion: 0.7
Nodes (3): SystemCoreClockUpdate(), SystemInit(), SystemInit_ExtMemCtl()

## Knowledge Gaps
- **9 isolated node(s):** `Synth-Keyboard`, `Apache License`, `This software component is provided to you as part of a software package and`, `This software component is provided to you as part of a software package and`, `Configuration	Synced_Keyboard` (+4 more)
  These have ≤1 connection - possible missing edges or undocumented components.
- **21 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `TIM_CCxChannelCmd()` connect `Tim Start` to `Tim Base`?**
  _High betweenness centrality (0.000) - this node is a cross-community bridge._
- **What connects `Synth-Keyboard`, `Apache License`, `This software component is provided to you as part of a software package and` to the rest of the system?**
  _9 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `Get Set` be split into smaller, more focused modules?**
  _Cohesion score 0.02 - nodes in this community are weakly interconnected._
- **Should `Get Set` be split into smaller, more focused modules?**
  _Cohesion score 0.02 - nodes in this community are weakly interconnected._
- **Should `Dir Lock` be split into smaller, more focused modules?**
  _Cohesion score 0.09 - nodes in this community are weakly interconnected._
- **Should `Uart Receive` be split into smaller, more focused modules?**
  _Cohesion score 0.08 - nodes in this community are weakly interconnected._
- **Should `Get Set` be split into smaller, more focused modules?**
  _Cohesion score 0.04 - nodes in this community are weakly interconnected._