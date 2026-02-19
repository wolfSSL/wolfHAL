#include <stdint.h>
#include <wolfHAL/spi/stm32wb_spi.h>
#include <wolfHAL/spi/spi.h>
#include <wolfHAL/clock/clock.h>
#include <wolfHAL/clock/stm32wb_rcc.h>
#include <wolfHAL/error.h>
#include <wolfHAL/regmap.h>
#include <wolfHAL/bitops.h>

/*
 * STM32WB SPI Register Definitions
 *
 * The SPI peripheral provides full-duplex synchronous serial communication.
 * This driver configures it in master mode with software slave management
 * and 8-bit data frames.
 */

/* Control Register 1 - master config, clock, enable */
#define SSPI_CR1_REG  0x00
#define SSPI_CR1_CPHA WHAL_MASK(0)           /* Clock phase */
#define SSPI_CR1_CPOL WHAL_MASK(1)           /* Clock polarity */
#define SSPI_CR1_MSTR WHAL_MASK(2)           /* Master selection */
#define SSPI_CR1_BR   WHAL_MASK_RANGE(5, 3)  /* Baud rate prescaler */
#define SSPI_CR1_SPE  WHAL_MASK(6)           /* SPI enable */
#define SSPI_CR1_SSI  WHAL_MASK(8)           /* Internal slave select */
#define SSPI_CR1_SSM  WHAL_MASK(9)           /* Software slave management */

/* Control Register 2 - data size, FIFO threshold */
#define SSPI_CR2_REG   0x04
#define SSPI_CR2_DS    WHAL_MASK_RANGE(11, 8) /* Data size (0111 = 8-bit) */
#define SSPI_CR2_FRXTH WHAL_MASK(12)          /* FIFO reception threshold */

/* Status Register */
#define SSPI_SR_REG  0x08
#define SSPI_SR_RXNE WHAL_MASK(0) /* Receive buffer not empty */
#define SSPI_SR_TXE  WHAL_MASK(1) /* Transmit buffer empty */
#define SSPI_SR_BSY  WHAL_MASK(7) /* Busy flag */

/* Data Register - 8/16-bit access */
#define SSPI_DR_REG  0x0C
#define SSPI_DR_MASK WHAL_MASK_RANGE(7, 0)

/* 8-bit data size value for DS field */
#define SSPI_DS_8BIT 0x7

/*
 * Calculate the baud rate prescaler index for a target baud rate.
 *
 * SPI baud rate = fPCLK / (2 ^ (BR + 1))
 *   BR=0 -> /2,  BR=1 -> /4,  BR=2 -> /8, ... BR=7 -> /256
 *
 * Returns the smallest prescaler that does not exceed the target baud.
 */
static uint32_t whal_Stm32wbSpi_CalcBr(size_t pclk, uint32_t targetBaud)
{
    uint32_t br;

    for (br = 0; br < 7; br++) {
        if ((pclk / (2u << br)) <= targetBaud) {
            return br;
        }
    }

    return 7;
}

/*
 * Configure SPI mode and baud rate, then enable the peripheral.
 * Must be called with SPE disabled.
 */
static void whal_Stm32wbSpi_ApplyComCfg(const whal_Regmap *reg,
                                        whal_Stm32wbSpi_Cfg *cfg,
                                        whal_Stm32wbSpi_ComCfg *comCfg)
{
    whal_Error err;
    size_t pclk;
    uint32_t cpol, cpha, br;

    /* FIXME: Derive pclk from the active RCC driver in cfg->clkCtrl, not PLL-only getrate. */
    err = WHAL_DRV_FN(Stm32wbRccPll, getrate)(cfg->clkCtrl, &pclk);
    if (err) {
        return;
    }

    br = whal_Stm32wbSpi_CalcBr(pclk, comCfg->baud);

    cpol = (comCfg->mode >> 1) & 1;
    cpha = comCfg->mode & 1;

    /* Disable SPE before reconfiguring */
    whal_Reg_Update(reg->base, SSPI_CR1_REG, SSPI_CR1_SPE,
                    whal_SetBits(SSPI_CR1_SPE, 0));

    /* Set mode and baud rate */
    whal_Reg_Update(reg->base, SSPI_CR1_REG,
                    SSPI_CR1_CPOL | SSPI_CR1_CPHA | SSPI_CR1_BR,
                    whal_SetBits(SSPI_CR1_CPOL, cpol) |
                    whal_SetBits(SSPI_CR1_CPHA, cpha) |
                    whal_SetBits(SSPI_CR1_BR, br));

    /* Re-enable SPE */
    whal_Reg_Update(reg->base, SSPI_CR1_REG, SSPI_CR1_SPE,
                    whal_SetBits(SSPI_CR1_SPE, 1));
}

whal_Error WHAL_DRV_FN(Stm32wbSpi, init)(whal_Spi *spiDev)
{
    whal_Error err;
    whal_Stm32wbSpi_Cfg *cfg;
    const whal_Regmap *reg;

    if (!spiDev || !spiDev->cfg) {
        return WHAL_EINVAL;
    }

    reg = &spiDev->regmap;
    cfg = (whal_Stm32wbSpi_Cfg *)spiDev->cfg;

    err = WHAL_DRV_FN(Stm32wbRccPll, enable)(cfg->clkCtrl, cfg->clk);
    if (err != WHAL_SUCCESS) {
        return err;
    }

    /* Master mode with software slave management */
    whal_Reg_Update(reg->base, SSPI_CR1_REG,
                    SSPI_CR1_MSTR | SSPI_CR1_SSM | SSPI_CR1_SSI,
                    whal_SetBits(SSPI_CR1_MSTR, 1) |
                    whal_SetBits(SSPI_CR1_SSM, 1) |
                    whal_SetBits(SSPI_CR1_SSI, 1));

    /* 8-bit data size and FIFO receive threshold for 8-bit */
    whal_Reg_Update(reg->base, SSPI_CR2_REG,
                    SSPI_CR2_DS | SSPI_CR2_FRXTH,
                    whal_SetBits(SSPI_CR2_DS, SSPI_DS_8BIT) |
                    whal_SetBits(SSPI_CR2_FRXTH, 1));

    return WHAL_SUCCESS;
}

whal_Error WHAL_DRV_FN(Stm32wbSpi, deinit)(whal_Spi *spiDev)
{
    whal_Error err;
    const whal_Regmap *reg;
    whal_Stm32wbSpi_Cfg *cfg;

    if (!spiDev || !spiDev->cfg) {
        return WHAL_EINVAL;
    }

    reg = &spiDev->regmap;
    cfg = (whal_Stm32wbSpi_Cfg *)spiDev->cfg;

    /* Disable SPI */
    whal_Reg_Update(reg->base, SSPI_CR1_REG, SSPI_CR1_SPE,
                    whal_SetBits(SSPI_CR1_SPE, 0));

    err = WHAL_DRV_FN(Stm32wbRccPll, disable)(cfg->clkCtrl, cfg->clk);
    if (err) {
        return err;
    }

    return WHAL_SUCCESS;
}

whal_Error WHAL_DRV_FN(Stm32wbSpi, sendrecv)(whal_Spi *spiDev, void *spiComCfg, const uint8_t *tx,
                                    size_t txLen, uint8_t *rx, size_t rxLen)
{
    const whal_Regmap *reg;
    whal_Stm32wbSpi_Cfg *cfg;
    whal_Stm32wbSpi_ComCfg *comCfg;

    if (!spiDev || !spiDev->cfg || !spiComCfg) {
        return WHAL_EINVAL;
    }

    reg = &spiDev->regmap;
    cfg = (whal_Stm32wbSpi_Cfg *)spiDev->cfg;
    comCfg = (whal_Stm32wbSpi_ComCfg *)spiComCfg;
    size_t totalLen = txLen > rxLen ? txLen : rxLen;
    size_t status;
    size_t d;

    whal_Stm32wbSpi_ApplyComCfg(reg, cfg, comCfg);

    for (size_t i = 0; i < totalLen; i++) {
        if (txLen && tx) {
            /* Wait for TX buffer empty */
            do {
                whal_Reg_Get(reg->base, SSPI_SR_REG, SSPI_SR_TXE, &status);
            } while (!status);

            /* Write data or dummy byte */
            uint8_t txByte = (i >= txLen) ? tx[txLen-1] : tx[i];
            *(volatile uint8_t *)(reg->base + SSPI_DR_REG) = txByte;
        }

        if (rxLen && rx) {
            /* Wait for RX buffer not empty */
            do {
                whal_Reg_Get(reg->base, SSPI_SR_REG, SSPI_SR_RXNE, &status);
            } while (!status);

            /* Read received byte */
            d = *(volatile uint8_t *)(reg->base + SSPI_DR_REG);
            if (i < rxLen) {
                rx[i] = (uint8_t)d;
            }
        }
    }

    /* Wait for not busy */
    do {
        whal_Reg_Get(reg->base, SSPI_SR_REG, SSPI_SR_BSY, &status);
    } while (status);

    return WHAL_SUCCESS;
}

whal_Error WHAL_DRV_FN(Stm32wbSpi, send)(whal_Spi *spiDev, void *spiComCfg, const uint8_t *data,
                                size_t dataSz)
{
    return WHAL_DRV_FN(Stm32wbSpi, sendrecv)(spiDev, spiComCfg, data, dataSz, NULL, 0);
}

whal_Error WHAL_DRV_FN(Stm32wbSpi, recv)(whal_Spi *spiDev, void *spiComCfg, uint8_t *data,
                                size_t dataSz)
{
    uint8_t dummyByte = 0xFF;
    return WHAL_DRV_FN(Stm32wbSpi, sendrecv)(spiDev, spiComCfg, &dummyByte, 1, data, dataSz);
}
