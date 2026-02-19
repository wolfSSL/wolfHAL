#include <stdint.h>
#include <wolfHAL/uart/stm32wb_uart.h>
#include <wolfHAL/uart/uart.h>
#include <wolfHAL/clock/clock.h>
#include <wolfHAL/clock/stm32wb_rcc.h>
#include <wolfHAL/error.h>
#include <wolfHAL/regmap.h>
#include <wolfHAL/bitops.h>

#define STUART_CR1_REG 0x00
#define STUART_CR1_UE WHAL_MASK(0)
#define STUART_CR1_RE WHAL_MASK(2)
#define STUART_CR1_TE WHAL_MASK(3)

#define STUART_BRR_REG 0x0C
#define STUART_BRR_BRR_MASK WHAL_MASK_RANGE(19, 0)

#define STUART_ISR_REG 0x1C
#define STUART_ISR_RXFNE_MASK WHAL_MASK(5)
#define STUART_ISR_TC_MASK WHAL_MASK(6)
#define STUART_ISR_TXE_MASK WHAL_MASK(7)

#define STUART_RDR_REG 0x24
#define STUART_RDR_RDR_MASK WHAL_MASK_RANGE(8, 0)

#define STUART_TDR_REG 0x28
#define STUART_TDR_TDR_MASK WHAL_MASK_RANGE(8, 0)

whal_Error WHAL_DRV_FN(Stm32wbUart, init)(whal_Uart *uartDev)
{
    whal_Error err;
    whal_Stm32wbUart_Cfg *cfg;
    const whal_Regmap *reg = &uartDev->regmap;
    size_t clockFreq;
    uint32_t brr;

    cfg = (whal_Stm32wbUart_Cfg *)uartDev->cfg;

    err = WHAL_DRV_FN(Stm32wbRccPll, enable)(cfg->clkCtrl, cfg->clk);
    if (err != WHAL_SUCCESS) {
        return err;
    }

    /* FIXME: Use the configured RCC driver from cfg->clkCtrl (not PLL-only getrate). */
    err = WHAL_DRV_FN(Stm32wbRccPll, getrate)(cfg->clkCtrl, &clockFreq);
    if (err != WHAL_SUCCESS) {
        return err;
    }

    brr = (clockFreq / cfg->baud);

    whal_Reg_Update(reg->base, STUART_BRR_REG,
                    STUART_BRR_BRR_MASK,
                    whal_SetBits(STUART_BRR_BRR_MASK, brr));
    whal_Reg_Update(reg->base, STUART_CR1_REG,
                    STUART_CR1_UE | STUART_CR1_RE | STUART_CR1_TE,
                    whal_SetBits(STUART_CR1_UE, 1) |
                    whal_SetBits(STUART_CR1_RE, 1) |
                    whal_SetBits(STUART_CR1_TE, 1));

    return WHAL_SUCCESS;
}

whal_Error WHAL_DRV_FN(Stm32wbLpuart, init)(whal_Uart *uartDev)
{
    whal_Error err;
    whal_Stm32wbUart_Cfg *cfg;
    const whal_Regmap *reg = &uartDev->regmap;
    size_t clockFreq;
    uint32_t brr;

    cfg = (whal_Stm32wbUart_Cfg *)uartDev->cfg;

    err = WHAL_DRV_FN(Stm32wbRccPll, enable)(cfg->clkCtrl, cfg->clk);
    if (err != WHAL_SUCCESS) {
        return err;
    }

    /* FIXME: Use the configured RCC driver from cfg->clkCtrl (not PLL-only getrate). */
    err = WHAL_DRV_FN(Stm32wbRccPll, getrate)(cfg->clkCtrl, &clockFreq);
    if (err != WHAL_SUCCESS) {
        return err;
    }

    brr = (clockFreq / cfg->baud) * 256;
    
    whal_Reg_Update(reg->base, STUART_BRR_REG,
                    STUART_BRR_BRR_MASK,
                    whal_SetBits(STUART_BRR_BRR_MASK, brr));
    whal_Reg_Update(reg->base, STUART_CR1_REG,
                    STUART_CR1_UE | STUART_CR1_RE | STUART_CR1_TE,
                    whal_SetBits(STUART_CR1_UE, 1) |
                    whal_SetBits(STUART_CR1_RE, 1) |
                    whal_SetBits(STUART_CR1_TE, 1));
    
    return WHAL_SUCCESS;
}

whal_Error WHAL_DRV_FN(Stm32wbUart, deinit)(whal_Uart *uartDev)
{
    whal_Error err;
    const whal_Regmap *reg = &uartDev->regmap;
    whal_Stm32wbUart_Cfg *cfg = (whal_Stm32wbUart_Cfg *)uartDev->cfg;

    whal_Reg_Update(reg->base, STUART_CR1_REG,
                    STUART_CR1_UE | STUART_CR1_RE | STUART_CR1_TE,
                    whal_SetBits(STUART_CR1_UE, 0) |
                    whal_SetBits(STUART_CR1_RE, 0) |
                    whal_SetBits(STUART_CR1_TE, 0));

    whal_Reg_Update(reg->base, STUART_BRR_REG,
                          STUART_BRR_BRR_MASK,
                          whal_SetBits(STUART_BRR_BRR_MASK, 0));

    err = WHAL_DRV_FN(Stm32wbRccPll, disable)(cfg->clkCtrl, cfg->clk);
    if (err) {
        return err;
    }

    return WHAL_SUCCESS;
}

whal_Error WHAL_DRV_FN(Stm32wbUart, send)(whal_Uart *uartDev, const uint8_t *data, size_t dataSz)
{
    const whal_Regmap *reg = &uartDev->regmap;
    
    for (size_t i = 0; i < dataSz; ++i) {
        size_t txComplete = 0;

        whal_Reg_Update(reg->base, STUART_TDR_REG, STUART_TDR_TDR_MASK, 
                        whal_SetBits(STUART_TDR_TDR_MASK, data[i]));

        while (!txComplete) {
            whal_Reg_Get(reg->base, STUART_ISR_REG, STUART_ISR_TC_MASK, &txComplete);
        }
    }

    return WHAL_SUCCESS;
}

whal_Error WHAL_DRV_FN(Stm32wbUart, recv)(whal_Uart *uartDev, uint8_t *data, size_t dataSz)
{
    const whal_Regmap *reg = &uartDev->regmap;
    size_t d;
    
    for (size_t i = 0; i < dataSz; ++i) {
        size_t dataReceived = 0;

        while (!dataReceived) {
            whal_Reg_Get(reg->base, STUART_ISR_REG, STUART_ISR_RXFNE_MASK, &dataReceived);
        }

        whal_Reg_Get(reg->base, STUART_RDR_REG, 
                     STUART_RDR_RDR_MASK, &d);

        data[i] = d;
    }

    return WHAL_SUCCESS;
}
