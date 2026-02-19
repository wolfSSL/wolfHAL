#ifndef WHAL_PIC32CZ_UART_H
#define WHAL_PIC32CZ_UART_H

#include <stdint.h>
#include <stddef.h>
#include <wolfHAL/clock/clock.h>
#include <wolfHAL/uart/uart.h>

/*
 * @file pic32cz_uart.h
 * @brief PIC32CZ SERCOM USART driver configuration.
 */

#define WHAL_PIC32CZ_UART_BAUD(freq_baud, freq_ref) \
    (65536UL - (65536UL * 16UL * (uint64_t)(freq_baud) / (uint64_t)(freq_ref)))

/*
 * @brief PIC32CZ SERCOM USART TX pad output options.
 */
typedef enum {
    WHAL_PIC32CZ_UART_TXPO_PAD0 = 0x0,       /* TxD on PAD[0], XCK on PAD[1] */
    WHAL_PIC32CZ_UART_TXPO_PAD0_RTS_CTS = 0x2, /* TxD on PAD[0], RTS on PAD[2], CTS on PAD[3] */
    WHAL_PIC32CZ_UART_TXPO_PAD0_TE = 0x3,    /* TxD on PAD[0], TE on PAD[2] */
} whal_Pic32czUart_TxPad;

/*
 * @brief PIC32CZ SERCOM USART RX pad input options.
 */
typedef enum {
    WHAL_PIC32CZ_UART_RXPO_PAD0 = 0x0,
    WHAL_PIC32CZ_UART_RXPO_PAD1 = 0x1,
    WHAL_PIC32CZ_UART_RXPO_PAD2 = 0x2,
    WHAL_PIC32CZ_UART_RXPO_PAD3 = 0x3,
} whal_Pic32czUart_RxPad;

/*
 * @brief PIC32CZ SERCOM USART configuration parameters.
 */
typedef struct whal_Pic32czUart_Cfg {
    whal_Clock *clkCtrl;
    void *clk;
    uint32_t baud;
    whal_Pic32czUart_TxPad txPad;
    whal_Pic32czUart_RxPad rxPad;
} whal_Pic32czUart_Cfg;

whal_Error WHAL_DRV_FN(Pic32czUart, init)(whal_Uart *uartDev);
whal_Error WHAL_DRV_FN(Pic32czUart, deinit)(whal_Uart *uartDev);
whal_Error WHAL_DRV_FN(Pic32czUart, send)(whal_Uart *uartDev, const uint8_t *data, size_t dataSz);
whal_Error WHAL_DRV_FN(Pic32czUart, recv)(whal_Uart *uartDev, uint8_t *data, size_t dataSz);

#endif /* WHAL_PIC32CZ_UART_H */
