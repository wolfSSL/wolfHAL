#ifndef WHAL_STM32WB_UART_H
#define WHAL_STM32WB_UART_H

#include <stdint.h>
#include <wolfHAL/clock/clock.h>
#include <wolfHAL/uart/uart.h>
#include <stddef.h>

/*
 * @file stm32wb_uart.h
 * @brief STM32 UART driver configuration.
 */

/*
 * @brief STM32 UART configuration parameters.
 */
typedef struct whal_Stm32wbUart_Cfg {
    whal_Clock *clkCtrl;
    void *clk;
    uint32_t baud;
} whal_Stm32wbUart_Cfg;

whal_Error WHAL_DRV_FN(Stm32wbUart, init)(whal_Uart *uartDev);
whal_Error WHAL_DRV_FN(Stm32wbLpuart, init)(whal_Uart *uartDev);
whal_Error WHAL_DRV_FN(Stm32wbUart, deinit)(whal_Uart *uartDev);
whal_Error WHAL_DRV_FN(Stm32wbUart, send)(whal_Uart *uartDev, const uint8_t *data, size_t dataSz);
whal_Error WHAL_DRV_FN(Stm32wbUart, recv)(whal_Uart *uartDev, uint8_t *data, size_t dataSz);

/* LPUART shares deinit/send/recv with UART */
#define whal_drv_Stm32wbLpuart_deinit WHAL_DRV_FN(Stm32wbUart, deinit)
#define whal_drv_Stm32wbLpuart_send   WHAL_DRV_FN(Stm32wbUart, send)
#define whal_drv_Stm32wbLpuart_recv   WHAL_DRV_FN(Stm32wbUart, recv)

#endif /* WHAL_STM32WB_UART_H */
