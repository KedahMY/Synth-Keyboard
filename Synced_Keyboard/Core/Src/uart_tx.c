#include "uart_tx.h"

static UART_HandleTypeDef huart1_tx;

void UART_TX_Init(void)
{
    __HAL_RCC_USART1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /* PA9 = USART1_TX, alternate function push-pull */
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin   = GPIO_PIN_9;
    gpio.Mode  = GPIO_MODE_AF_PP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &gpio);

    huart1_tx.Instance          = USART1;
    huart1_tx.Init.BaudRate     = 115200;
    huart1_tx.Init.WordLength   = UART_WORDLENGTH_8B;
    huart1_tx.Init.StopBits     = UART_STOPBITS_1;
    huart1_tx.Init.Parity       = UART_PARITY_NONE;
    huart1_tx.Init.Mode         = UART_MODE_TX;
    huart1_tx.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    huart1_tx.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart1_tx);
}

/*
 * Blocking TX is fine here: 8 bytes at 115200 baud takes ~695 us,
 * which is called at most once per keypress event in the main loop.
 * No ISR contention risk.
 */
void UART_TX_SendNoteEvent(const NoteEvent *evt)
{
    HAL_UART_Transmit(&huart1_tx,
                      (uint8_t *)evt,
                      NOTE_EVENT_SIZE,
                      10);   /* 10 ms timeout, way more than needed */
}
