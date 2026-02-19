#include <stdint.h>
#include <wolfHAL/clock/clock.h>
#include <wolfHAL/clock/stm32wb_rcc.h>
#include <wolfHAL/flash/stm32wb_flash.h>
#include <wolfHAL/flash/flash.h>
#include <wolfHAL/error.h>
#include <wolfHAL/bitops.h>

/*
 * STM32WB Flash Register Definitions
 *
 * The flash controller manages embedded flash memory operations including
 * programming, erasing, and access control. Flash is organized in 4 KB pages.
 */

/* Access Control Register - configures latency and caches */
#define ST_FLASH_ACR_REG 0x00
#define ST_FLASH_ACR_LATENCY WHAL_MASK_RANGE(2, 0) /* Wait states (0-3) */

/* Key Register - unlock sequence for write operations */
#define ST_FLASH_KEYR_REG 0x08
#define ST_FLASH_KEYR_KEY WHAL_MASK_RANGE(31, 0)

/* Status Register - operation status and error flags */
#define ST_FLASH_SR_REG 0x10
#define ST_FLASH_SR_EOP WHAL_MASK(0)       /* End of operation */
#define ST_FLASH_SR_OP_ERR WHAL_MASK(1)    /* Operation error */
#define ST_FLASH_SR_PROG_ERR WHAL_MASK(3)  /* Programming error */
#define ST_FLASH_SR_WRP_ERR WHAL_MASK(4)   /* Write protection error */
#define ST_FLASH_SR_PGA_ERR WHAL_MASK(5)   /* Programming alignment error */
#define ST_FLASH_SR_SIZ_ERR WHAL_MASK(6)   /* Size error */
#define ST_FLASH_SR_PGS_ERR WHAL_MASK(7)   /* Programming sequence error */
#define ST_FLASH_SR_MISS_ERR WHAL_MASK(8)  /* Fast programming miss error */
#define ST_FLASH_SR_FAST_ERR WHAL_MASK(9)  /* Fast programming error */
#define ST_FLASH_SR_RD_ERR WHAL_MASK(14)   /* Read protection error */
#define ST_FLASH_SR_OPTV_ERR WHAL_MASK(15) /* Option validity error */
#define ST_FLASH_SR_BSY WHAL_MASK(16)      /* Busy flag */
#define ST_FLASH_SR_CFGBSY WHAL_MASK(18)   /* Configuration busy */
#define ST_FLASH_SR_PESD WHAL_MASK(19)     /* Programming/erase suspended */

/* Combined mask for all error flags */
#define ST_FLASH_SR_ALL_ERR (ST_FLASH_SR_OP_ERR | ST_FLASH_SR_PROG_ERR | ST_FLASH_SR_WRP_ERR | \
                             ST_FLASH_SR_PGA_ERR | ST_FLASH_SR_SIZ_ERR | ST_FLASH_SR_PGS_ERR | \
                             ST_FLASH_SR_MISS_ERR | ST_FLASH_SR_FAST_ERR | ST_FLASH_SR_RD_ERR | \
                             ST_FLASH_SR_OPTV_ERR)

/* Control Register - enables operations and selects pages */
#define ST_FLASH_CR_REG 0x14
#define ST_FLASH_CR_PG WHAL_MASK(0)            /* Programming enable */
#define ST_FLASH_CR_PER WHAL_MASK(1)           /* Page erase enable */
#define ST_FLASH_CR_PNB WHAL_MASK_RANGE(10, 3) /* Page number for erase */
#define ST_FLASH_CR_STRT WHAL_MASK(16)         /* Start erase operation */
#define ST_FLASH_CR_LOCK WHAL_MASK(31)         /* Lock flash control */

whal_Error WHAL_DRV_FN(Stm32wbFlash, init)(whal_Flash *flashDev)
{
    whal_Error err;
    whal_Stm32wbFlash_Cfg *cfg = flashDev->cfg;

    err = WHAL_DRV_FN(Stm32wbRccPll, enable)(cfg->clkCtrl, cfg->clk);
    if (err) {
        return err;
    }

    return WHAL_SUCCESS;
}

whal_Error WHAL_DRV_FN(Stm32wbFlash, deinit)(whal_Flash *flashDev)
{
    whal_Error err;
    whal_Stm32wbFlash_Cfg *cfg = flashDev->cfg;

    err = WHAL_DRV_FN(Stm32wbRccPll, disable)(cfg->clkCtrl, cfg->clk);
    if (err) {
        return err;
    }

    return WHAL_SUCCESS;
}

whal_Error WHAL_DRV_FN(Stm32wbFlash, lock)(whal_Flash *flashDev, size_t addr, size_t len)
{
    (void)addr;
    (void)len;

    const whal_Regmap *regmap = &flashDev->regmap;

    /* Setting LOCK bit prevents further flash modifications until next unlock */
    whal_Reg_Update(regmap->base, ST_FLASH_CR_REG, ST_FLASH_CR_LOCK,
                    whal_SetBits(ST_FLASH_CR_LOCK, 1));

    return WHAL_SUCCESS;
}

whal_Error WHAL_DRV_FN(Stm32wbFlash, unlock)(whal_Flash *flashDev, size_t addr, size_t len)
{
    (void)addr;
    (void)len;

    const whal_Regmap *regmap = &flashDev->regmap;

    /*
     * Unlock sequence: write KEY1 then KEY2 to KEYR register.
     * Incorrect sequence or order will trigger a bus error.
     */
    whal_Reg_Update(regmap->base, ST_FLASH_KEYR_REG, ST_FLASH_KEYR_KEY, 0x45670123);
    whal_Reg_Update(regmap->base, ST_FLASH_KEYR_REG, ST_FLASH_KEYR_KEY, 0xCDEF89AB);

    return WHAL_SUCCESS;
}

whal_Error WHAL_DRV_FN(Stm32wbFlash, read)(whal_Flash *flashDev, size_t addr, uint8_t *data,
                             size_t dataSz)
{
    (void)flashDev;

    /* Flash is memory-mapped, so reading is a simple memory copy */
    uint8_t *flashAddr = (uint8_t *)addr;
    for (size_t i = 0; i < dataSz; ++i) {
        data[i] = flashAddr[i];
    }
    return WHAL_SUCCESS;
}

/*
 * Internal helper for write and erase operations.
 *
 * For write (write=1): Programs data in 64-bit (8 byte) chunks.
 * For erase (write=0): Erases 4 KB pages covering the address range.
 */
static whal_Error whal_Stm32wbFlash_writeOrErase(whal_Flash *flashDev, size_t addr, const uint8_t *data,
                                            size_t dataSz, uint8_t write)
{
    whal_Stm32wbFlash_Cfg *cfg = flashDev->cfg;
    const whal_Regmap *regmap = &flashDev->regmap;
    size_t bsy;
    size_t pesd;
    size_t cfgbsy = 0;

    /* Validate address alignment and bounds */
    if (addr & 0xf || addr < cfg->startAddr || addr + dataSz > cfg->startAddr + cfg->size) {
        return WHAL_EINVAL;
    }

    /* Check if flash is busy or suspended */
    whal_Reg_Get(regmap->base, ST_FLASH_SR_REG, ST_FLASH_SR_BSY, &bsy);
    whal_Reg_Get(regmap->base, ST_FLASH_SR_REG, ST_FLASH_SR_PESD, &pesd);

    if (bsy || pesd) {
        return WHAL_ENOTREADY;
    }

    /* Clear all error flags by writing 1 to each */
    whal_Reg_Update(regmap->base, ST_FLASH_SR_REG, ST_FLASH_SR_ALL_ERR, 0xffffffff);

    if (write) {
        /* Enable flash programming mode */
        whal_Reg_Update(regmap->base, ST_FLASH_CR_REG, ST_FLASH_CR_PG, 1);

        /* Program data in 64-bit (8 byte) double-word chunks */
        for (size_t i = 0; i < dataSz; i += 8) {
            uint32_t *flashAddr = (uint32_t *)(addr + i);
            uint32_t *dataAddr = (uint32_t *)(data + i);

            /* Write both 32-bit words to trigger the 64-bit programming */
            flashAddr[0] = dataAddr[0];
            flashAddr[1] = dataAddr[1];

            /* Wait for programming to complete */
            do {
                whal_Reg_Get(regmap->base, ST_FLASH_SR_REG, ST_FLASH_SR_CFGBSY, &cfgbsy);
            } while (cfgbsy);
        }
    }
    else {
        /* Calculate page range to erase (4 KB per page) */
        size_t startPage, endPage;
        startPage = (addr - cfg->startAddr) >> 12;
        endPage = ((addr - cfg->startAddr) + dataSz) >> 12;

        /* Enable page erase mode */
        whal_Reg_Update(regmap->base, ST_FLASH_CR_REG, ST_FLASH_CR_PER,
                        whal_SetBits(ST_FLASH_CR_PER, 1));

        /* Erase each page in the range */
        for (size_t page = startPage; page <= endPage; ++page) {
            /* Select page number */
            whal_Reg_Update(regmap->base, ST_FLASH_CR_REG, ST_FLASH_CR_PNB,
                            whal_SetBits(ST_FLASH_CR_PNB, page));

            /* Start erase operation */
            whal_Reg_Update(regmap->base, ST_FLASH_CR_REG, ST_FLASH_CR_STRT,
                            whal_SetBits(ST_FLASH_CR_STRT, 1));

            /* Wait for erase to complete */
            do {
                whal_Reg_Get(regmap->base, ST_FLASH_SR_REG, ST_FLASH_SR_CFGBSY, &cfgbsy);
            } while (cfgbsy);
        }

        /* Disable page erase mode */
        whal_Reg_Update(regmap->base, ST_FLASH_CR_REG, ST_FLASH_CR_PER,
                        whal_SetBits(ST_FLASH_CR_PER, 0));
    }

    /* Disable flash programming mode */
    whal_Reg_Update(regmap->base, ST_FLASH_CR_REG, ST_FLASH_CR_PG, 0);

    return WHAL_SUCCESS;
}

whal_Error WHAL_DRV_FN(Stm32wbFlash, write)(whal_Flash *flashDev, size_t addr, const uint8_t *data,
                                size_t dataSz)
{
    return whal_Stm32wbFlash_writeOrErase(flashDev, addr, data, dataSz, 1);
}

whal_Error WHAL_DRV_FN(Stm32wbFlash, erase)(whal_Flash *flashDev, size_t addr,
                                size_t dataSz)
{
    return whal_Stm32wbFlash_writeOrErase(flashDev, addr, NULL, dataSz, 0);
}

whal_Error whal_Stm32wbFlash_Ext_SetLatency(whal_Flash *flashDev, enum whal_Stm32wbFlash_Latency latency)
{
    if (!flashDev) {
        return WHAL_EINVAL;
    }

    const whal_Regmap *reg = &flashDev->regmap;
    whal_Reg_Update(reg->base, ST_FLASH_ACR_REG, ST_FLASH_ACR_LATENCY, latency);
    return WHAL_SUCCESS;
}

