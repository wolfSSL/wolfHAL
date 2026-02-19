#include <stdint.h>
#include <wolfHAL/uart/pic32cz_uart.h>
#include <wolfHAL/uart/uart.h>
#include <wolfHAL/clock/clock.h>
#include <wolfHAL/clock/pic32cz_clock.h>
#include <wolfHAL/error.h>
#include <wolfHAL/regmap.h>
#include <wolfHAL/bitops.h>

/* SERCOM USART Register Offsets */
#define SERCOM_USART_CTRLA_REG     0x00
#define SERCOM_USART_CTRLB_REG     0x04
#define SERCOM_USART_CTRLC_REG     0x08
#define SERCOM_USART_BAUD_REG      0x0C
#define SERCOM_USART_INTENCLR_REG  0x14
#define SERCOM_USART_INTENSET_REG  0x16
#define SERCOM_USART_INTFLAG_REG   0x18
#define SERCOM_USART_STATUS_REG    0x1A
#define SERCOM_USART_SYNCBUSY_REG  0x1C
#define SERCOM_USART_DATA_REG      0x28

/* CTRLA Register Bit Definitions */
#define SERCOM_USART_CTRLA_SWRST       WHAL_MASK(0)
#define SERCOM_USART_CTRLA_ENABLE      WHAL_MASK(1)
#define SERCOM_USART_CTRLA_MODE_MASK   WHAL_MASK_RANGE(4, 2)
#define SERCOM_USART_CTRLA_MODE_USART_INT_CLK 0x1
#define SERCOM_USART_CTRLA_RUNSTDBY    WHAL_MASK(7)
#define SERCOM_USART_CTRLA_IBON        WHAL_MASK(8)
#define SERCOM_USART_CTRLA_TXINV       WHAL_MASK(9)
#define SERCOM_USART_CTRLA_RXINV       WHAL_MASK(10)
#define SERCOM_USART_CTRLA_SAMPR_MASK  WHAL_MASK_RANGE(15, 13)
#define SERCOM_USART_CTRLA_SAMPR_16X_ARITH 0x0
#define SERCOM_USART_CTRLA_SAMPR_16X_FRAC  0x1
#define SERCOM_USART_CTRLA_SAMPR_8X_ARITH  0x2
#define SERCOM_USART_CTRLA_SAMPR_8X_FRAC   0x3
#define SERCOM_USART_CTRLA_SAMPR_3X_ARITH  0x4
#define SERCOM_USART_CTRLA_TXPO_MASK   WHAL_MASK_RANGE(17, 16)
#define SERCOM_USART_CTRLA_RXPO_MASK   WHAL_MASK_RANGE(21, 20)
#define SERCOM_USART_CTRLA_SAMPA_MASK  WHAL_MASK_RANGE(23, 22)
#define SERCOM_USART_CTRLA_FORM_MASK   WHAL_MASK_RANGE(27, 24)
#define SERCOM_USART_CTRLA_FORM_USART  0x0
#define SERCOM_USART_CTRLA_FORM_PARITY 0x1
#define SERCOM_USART_CTRLA_CMODE       WHAL_MASK(28)
#define SERCOM_USART_CTRLA_CPOL        WHAL_MASK(29)
#define SERCOM_USART_CTRLA_DORD        WHAL_MASK(30)

/* CTRLB Register Bit Definitions */
#define SERCOM_USART_CTRLB_CHSIZE_MASK WHAL_MASK_RANGE(2, 0)
#define SERCOM_USART_CTRLB_CHSIZE_8BIT 0x0
#define SERCOM_USART_CTRLB_CHSIZE_9BIT 0x1
#define SERCOM_USART_CTRLB_CHSIZE_5BIT 0x5
#define SERCOM_USART_CTRLB_CHSIZE_6BIT 0x6
#define SERCOM_USART_CTRLB_CHSIZE_7BIT 0x7
#define SERCOM_USART_CTRLB_SBMODE      WHAL_MASK(6)
#define SERCOM_USART_CTRLB_COLDEN      WHAL_MASK(8)
#define SERCOM_USART_CTRLB_SFDE        WHAL_MASK(9)
#define SERCOM_USART_CTRLB_ENC         WHAL_MASK(10)
#define SERCOM_USART_CTRLB_PMODE       WHAL_MASK(13)
#define SERCOM_USART_CTRLB_TXEN        WHAL_MASK(16)
#define SERCOM_USART_CTRLB_RXEN        WHAL_MASK(17)

/* INTFLAG Register Bit Definitions */
#define SERCOM_USART_INTFLAG_DRE       WHAL_MASK(0)
#define SERCOM_USART_INTFLAG_TXC       WHAL_MASK(1)
#define SERCOM_USART_INTFLAG_RXC       WHAL_MASK(2)
#define SERCOM_USART_INTFLAG_RXS       WHAL_MASK(3)
#define SERCOM_USART_INTFLAG_CTSIC     WHAL_MASK(4)
#define SERCOM_USART_INTFLAG_RXBRK     WHAL_MASK(5)
#define SERCOM_USART_INTFLAG_ERROR     WHAL_MASK(7)

/* STATUS Register Bit Definitions */
#define SERCOM_USART_STATUS_PERR       WHAL_MASK(0)
#define SERCOM_USART_STATUS_FERR       WHAL_MASK(1)
#define SERCOM_USART_STATUS_BUFOVF     WHAL_MASK(2)
#define SERCOM_USART_STATUS_CTS        WHAL_MASK(3)
#define SERCOM_USART_STATUS_ISF        WHAL_MASK(4)
#define SERCOM_USART_STATUS_COLL       WHAL_MASK(5)
#define SERCOM_USART_STATUS_TXE        WHAL_MASK(6)

/* SYNCBUSY Register Bit Definitions */
#define SERCOM_USART_SYNCBUSY_SWRST    WHAL_MASK(0)
#define SERCOM_USART_SYNCBUSY_ENABLE   WHAL_MASK(1)
#define SERCOM_USART_SYNCBUSY_CTRLB    WHAL_MASK(2)

/* DATA Register */
#define SERCOM_USART_DATA_MASK         WHAL_MASK_RANGE(8, 0)

static void whal_Pic32czUart_WaitSync(const whal_Regmap *reg, size_t mask)
{
    size_t busy = 1;
    while (busy) {
        whal_Reg_Get(reg->base, SERCOM_USART_SYNCBUSY_REG, mask, &busy);
    }
}

whal_Error WHAL_DRV_FN(Pic32czUart, init)(whal_Uart *uartDev)
{
    whal_Error err;
    whal_Pic32czUart_Cfg *cfg;
    const whal_Regmap *reg;

    if (!uartDev) {
        return WHAL_EINVAL;
    }

    reg = &uartDev->regmap;
    cfg = (whal_Pic32czUart_Cfg *)uartDev->cfg;

    /* Enable peripheral clock */
    err = WHAL_DRV_FN(Pic32czClockPll, enable)(cfg->clkCtrl, cfg->clk);
    if (err != WHAL_SUCCESS) {
        return err;
    }

    /* Configure CTRLA: internal clock, async mode, LSB first, 16x sampling */
    whal_Reg_Update(reg->base, SERCOM_USART_CTRLA_REG,
                    SERCOM_USART_CTRLA_MODE_MASK |
                    SERCOM_USART_CTRLA_SAMPR_MASK |
                    SERCOM_USART_CTRLA_TXPO_MASK |
                    SERCOM_USART_CTRLA_RXPO_MASK |
                    SERCOM_USART_CTRLA_FORM_MASK |
                    SERCOM_USART_CTRLA_CMODE |
                    SERCOM_USART_CTRLA_DORD,
                    whal_SetBits(SERCOM_USART_CTRLA_MODE_MASK, SERCOM_USART_CTRLA_MODE_USART_INT_CLK) |
                    whal_SetBits(SERCOM_USART_CTRLA_SAMPR_MASK, SERCOM_USART_CTRLA_SAMPR_16X_ARITH) |
                    whal_SetBits(SERCOM_USART_CTRLA_TXPO_MASK, cfg->txPad) |
                    whal_SetBits(SERCOM_USART_CTRLA_RXPO_MASK, cfg->rxPad) |
                    whal_SetBits(SERCOM_USART_CTRLA_FORM_MASK, SERCOM_USART_CTRLA_FORM_USART) |
                    whal_SetBits(SERCOM_USART_CTRLA_CMODE, 0) |  /* Async mode */
                    whal_SetBits(SERCOM_USART_CTRLA_DORD, 1));   /* LSB first */

    /* Configure CTRLB: 8-bit char size, 1 stop bit, no parity */
    whal_Reg_Update(reg->base, SERCOM_USART_CTRLB_REG,
                    SERCOM_USART_CTRLB_CHSIZE_MASK |
                    SERCOM_USART_CTRLB_SBMODE |
                    SERCOM_USART_CTRLB_PMODE,
                    whal_SetBits(SERCOM_USART_CTRLB_CHSIZE_MASK, SERCOM_USART_CTRLB_CHSIZE_8BIT) |
                    whal_SetBits(SERCOM_USART_CTRLB_SBMODE, 0) |
                    whal_SetBits(SERCOM_USART_CTRLB_PMODE, 0));

    /* Wait for CTRLB sync */
    whal_Pic32czUart_WaitSync(reg, SERCOM_USART_SYNCBUSY_CTRLB);

    /* Set baud rate */
    whal_Reg_Update(reg->base, SERCOM_USART_BAUD_REG,
                    0xFFFF,
                    cfg->baud);

    /* Enable transmitter and receiver */
    whal_Reg_Update(reg->base, SERCOM_USART_CTRLB_REG,
                    SERCOM_USART_CTRLB_TXEN | SERCOM_USART_CTRLB_RXEN,
                    whal_SetBits(SERCOM_USART_CTRLB_TXEN, 1) |
                    whal_SetBits(SERCOM_USART_CTRLB_RXEN, 1));

    /* Wait for CTRLB sync */
    whal_Pic32czUart_WaitSync(reg, SERCOM_USART_SYNCBUSY_CTRLB);

    /* Enable SERCOM USART */
    whal_Reg_Update(reg->base, SERCOM_USART_CTRLA_REG,
                    SERCOM_USART_CTRLA_ENABLE,
                    whal_SetBits(SERCOM_USART_CTRLA_ENABLE, 1));

    /* Wait for enable sync */
    whal_Pic32czUart_WaitSync(reg, SERCOM_USART_SYNCBUSY_ENABLE);

    return WHAL_SUCCESS;
}

whal_Error WHAL_DRV_FN(Pic32czUart, deinit)(whal_Uart *uartDev)
{
    whal_Error err;
    const whal_Regmap *reg;
    whal_Pic32czUart_Cfg *cfg;

    if (!uartDev) {
        return WHAL_EINVAL;
    }

    reg = &uartDev->regmap;
    cfg = (whal_Pic32czUart_Cfg *)uartDev->cfg;

    /* Disable SERCOM USART */
    whal_Reg_Update(reg->base, SERCOM_USART_CTRLA_REG,
                    SERCOM_USART_CTRLA_ENABLE,
                    whal_SetBits(SERCOM_USART_CTRLA_ENABLE, 0));

    /* Wait for disable sync */
    whal_Pic32czUart_WaitSync(reg, SERCOM_USART_SYNCBUSY_ENABLE);

    /* Disable transmitter and receiver */
    whal_Reg_Update(reg->base, SERCOM_USART_CTRLB_REG,
                    SERCOM_USART_CTRLB_TXEN | SERCOM_USART_CTRLB_RXEN,
                    whal_SetBits(SERCOM_USART_CTRLB_TXEN, 0) |
                    whal_SetBits(SERCOM_USART_CTRLB_RXEN, 0));

    /* Wait for CTRLB sync */
    whal_Pic32czUart_WaitSync(reg, SERCOM_USART_SYNCBUSY_CTRLB);

    /* Disable peripheral clock */
    err = WHAL_DRV_FN(Pic32czClockPll, disable)(cfg->clkCtrl, cfg->clk);
    if (err != WHAL_SUCCESS) {
        return err;
    }

    return WHAL_SUCCESS;
}

whal_Error WHAL_DRV_FN(Pic32czUart, send)(whal_Uart *uartDev, const uint8_t *data, size_t dataSz)
{
    const whal_Regmap *reg;

    if (!uartDev || !data) {
        return WHAL_EINVAL;
    }

    reg = &uartDev->regmap;

    for (size_t i = 0; i < dataSz; ++i) {
        size_t dataRegEmpty = 0;

        /* Wait for data register to be empty */
        while (!dataRegEmpty) {
            whal_Reg_Get(reg->base, SERCOM_USART_INTFLAG_REG,
                         SERCOM_USART_INTFLAG_DRE, &dataRegEmpty);
        }

        /* Write data to transmit register */
        whal_Reg_Update(reg->base, SERCOM_USART_DATA_REG,
                        SERCOM_USART_DATA_MASK,
                        whal_SetBits(SERCOM_USART_DATA_MASK, data[i]));
    }

    /* Wait for transmission complete */
    {
        size_t txComplete = 0;
        while (!txComplete) {
            whal_Reg_Get(reg->base, SERCOM_USART_INTFLAG_REG,
                         SERCOM_USART_INTFLAG_TXC, &txComplete);
        }
        /* Clear TXC flag by writing 1 */
        whal_Reg_Update(reg->base, SERCOM_USART_INTFLAG_REG,
                        SERCOM_USART_INTFLAG_TXC,
                        whal_SetBits(SERCOM_USART_INTFLAG_TXC, 1));
    }

    return WHAL_SUCCESS;
}

whal_Error WHAL_DRV_FN(Pic32czUart, recv)(whal_Uart *uartDev, uint8_t *data, size_t dataSz)
{
    const whal_Regmap *reg;

    if (!uartDev || !data) {
        return WHAL_EINVAL;
    }

    reg = &uartDev->regmap;

    for (size_t i = 0; i < dataSz; ++i) {
        size_t dataReceived = 0;
        size_t rxData;

        /* Wait for receive complete */
        while (!dataReceived) {
            whal_Reg_Get(reg->base, SERCOM_USART_INTFLAG_REG,
                         SERCOM_USART_INTFLAG_RXC, &dataReceived);
        }

        /* Read received data */
        whal_Reg_Get(reg->base, SERCOM_USART_DATA_REG,
                     SERCOM_USART_DATA_MASK, &rxData);

        data[i] = (uint8_t)rxData;
    }

    return WHAL_SUCCESS;
}

