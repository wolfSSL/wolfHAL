#include <stdint.h>
#include <wolfHAL/flash/pic32cz_flash.h>
#include <wolfHAL/flash/flash.h>
#include <wolfHAL/error.h>
#include <wolfHAL/regmap.h>
#include <wolfHAL/bitops.h>

/*
 * PIC32CZ FCW (Flash Controller Write) Register Definitions
 *
 * The PIC32CZ uses FCW for flash write/erase operations. Each operation
 * requires writing an unlock key to FCW_KEY before triggering via FCW_CTRLA.
 */

/* Register offsets */
#define FCW_CTRLA_REG       0x00
#define FCW_MUTEX_REG       0x08
#define FCW_INTFLAG_REG     0x14
#define FCW_STATUS_REG      0x18
#define FCW_KEY_REG         0x1C
#define FCW_ADDR_REG        0x20
#define FCW_SRCADDR_REG        0x24
#define FCW_DATA_REG(n)     (0x28 + ((n) * 4))

/* CTRLA: NVM operation command and pre-program bit */
#define FCW_CTRLA_NVMOP_MASK               WHAL_MASK_RANGE(3, 0)
#define FCW_CTRLA_NVMOP_SINGLE_DWORD       0x1
#define FCW_CTRLA_NVMOP_QUAD_DWORD         0x2
#define FCW_CTRLA_NVMOP_PAGE_ERASE         0x4
#define FCW_CTRLA_PREPG                     WHAL_MASK(7)

/* MUTEX: NVM locking and ownership values */
#define FCW_MUTEX_LOCK WHAL_MASK(0)
#define FCW_MUTEX_OWNER WHAL_MASK_RANGE(2, 1)

/* INTFLAG: operation completion and error flags (write-1-to-clear) */
#define FCW_INTFLAG_DONE        WHAL_MASK(0)
#define FCW_INTFLAG_KEYERR      WHAL_MASK(1)
#define FCW_INTFLAG_CFGERR      WHAL_MASK(2)
#define FCW_INTFLAG_FIFOERR     WHAL_MASK(3)
#define FCW_INTFLAG_BUSERR      WHAL_MASK(4)
#define FCW_INTFLAG_WPERR       WHAL_MASK(5)
#define FCW_INTFLAG_OPERR       WHAL_MASK(6)
#define FCW_INTFLAG_SECERR      WHAL_MASK(7)
#define FCW_INTFLAG_HTDPGM      WHAL_MASK(8)
#define FCW_INTFLAG_BORERR      WHAL_MASK(12)
#define FCW_INTFLAG_WRERR       WHAL_MASK(13)

#define FCW_INTFLAG_ALL_ERR     (FCW_INTFLAG_KEYERR | FCW_INTFLAG_CFGERR | \
                                 FCW_INTFLAG_FIFOERR | FCW_INTFLAG_BUSERR | \
                                 FCW_INTFLAG_WPERR | FCW_INTFLAG_OPERR | \
                                 FCW_INTFLAG_SECERR | FCW_INTFLAG_HTDPGM | \
                                 FCW_INTFLAG_BORERR | FCW_INTFLAG_WRERR)

#define FCW_INTFLAG_ALL         (FCW_INTFLAG_DONE | FCW_INTFLAG_ALL_ERR)

/* STATUS: busy flag */
#define FCW_STATUS_BUSY         WHAL_MASK(0)

/* KEY: unlock value for write/erase operations */
#define FCW_UNLOCK_WRKEY        0x91C32C01

/* Flash geometry */
#define FCW_PAGE_SIZE           4096
#define FCW_DWORD_SIZE          8
#define FCW_QDWORD_SIZE         32

static void whal_Pic32czFlash_MutexLock(const whal_Regmap *reg)
{
    size_t locked = 1;
    while (locked) {
        whal_Reg_Get(reg->base, FCW_MUTEX_REG, FCW_MUTEX_LOCK, &locked);
    }

    whal_Reg_Update(reg->base, FCW_MUTEX_REG, FCW_MUTEX_LOCK | FCW_MUTEX_OWNER,
                    whal_SetBits(FCW_MUTEX_LOCK, 1) |
                    whal_SetBits(FCW_MUTEX_OWNER, 1));
}

static void whal_Pic32czFlash_MutexUnlock(const whal_Regmap *reg)
{
    whal_Reg_Update(reg->base, FCW_MUTEX_REG, FCW_MUTEX_LOCK | FCW_MUTEX_OWNER,
                    whal_SetBits(FCW_MUTEX_LOCK, 0) |
                    whal_SetBits(FCW_MUTEX_OWNER, 1));
}

static void whal_Pic32czFlash_WaitBusy(const whal_Regmap *reg)
{
    size_t busy = 1;
    while (busy) {
        whal_Reg_Get(reg->base, FCW_STATUS_REG, FCW_STATUS_BUSY, &busy);
    }
}

/*
 * Execute an FCW command: unlock, trigger, wait, and check for errors.
 * Caller must set up FCW_ADDR and FCW_DATA registers before calling.
 */
static whal_Error whal_Pic32czFlash_ExecCmd(const whal_Regmap *reg, size_t cmd)
{
    size_t errFlags;

    /* Write unlock key */
    whal_Reg_Update(reg->base, FCW_KEY_REG, 0xFFFFFFFF, FCW_UNLOCK_WRKEY);

    /* Trigger operation with pre-program enabled */
    whal_Reg_Update(reg->base, FCW_CTRLA_REG,
                    FCW_CTRLA_NVMOP_MASK | FCW_CTRLA_PREPG,
                    whal_SetBits(FCW_CTRLA_NVMOP_MASK, cmd) | FCW_CTRLA_PREPG);

    /* Wait for completion */
    whal_Pic32czFlash_WaitBusy(reg);

    /* Check for errors */
    whal_Reg_Get(reg->base, FCW_INTFLAG_REG, FCW_INTFLAG_ALL_ERR, &errFlags);

    /* Clear DONE flag */
    whal_Reg_Update(reg->base, FCW_INTFLAG_REG, FCW_INTFLAG_DONE,
                    FCW_INTFLAG_DONE);

    if (errFlags) {
        /* Clear error flags */
        whal_Reg_Update(reg->base, FCW_INTFLAG_REG, FCW_INTFLAG_ALL_ERR,
                        FCW_INTFLAG_ALL_ERR);
        return WHAL_EINVAL;
    }

    return WHAL_SUCCESS;
}

whal_Error WHAL_DRV_FN(Pic32czFlash, init)(whal_Flash *flashDev)
{
    const whal_Regmap *reg;

    if (!flashDev) {
        return WHAL_EINVAL;
    }

    reg = &flashDev->regmap;

    whal_Pic32czFlash_MutexUnlock(reg);
    whal_Reg_Update(reg->base, FCW_KEY_REG, 0xFFFFFFFF, 0);
    whal_Reg_Update(reg->base, FCW_ADDR_REG, 0xFFFFFFFF, 0);
    whal_Reg_Update(reg->base, FCW_SRCADDR_REG, 0xFFFFFFFF, 0);

    /* Clear all pending interrupt flags */
    whal_Reg_Update(reg->base, FCW_INTFLAG_REG, FCW_INTFLAG_ALL,
                    FCW_INTFLAG_ALL);

    return WHAL_SUCCESS;
}

whal_Error WHAL_DRV_FN(Pic32czFlash, deinit)(whal_Flash *flashDev)
{
    const whal_Regmap *reg;

    if (!flashDev) {
        return WHAL_EINVAL;
    }

    reg = &flashDev->regmap;

    whal_Pic32czFlash_MutexUnlock(reg);
    whal_Reg_Update(reg->base, FCW_KEY_REG, 0xFFFFFFFF, 0);
    whal_Reg_Update(reg->base, FCW_ADDR_REG, 0xFFFFFFFF, 0);
    whal_Reg_Update(reg->base, FCW_SRCADDR_REG, 0xFFFFFFFF, 0);

    /* Clear all pending interrupt flags */
    whal_Reg_Update(reg->base, FCW_INTFLAG_REG, FCW_INTFLAG_ALL,
                    FCW_INTFLAG_ALL);

    return WHAL_SUCCESS;
}

whal_Error WHAL_DRV_FN(Pic32czFlash, lock)(whal_Flash *flashDev, size_t addr, size_t len)
{
    /*
     * TODO: Implement using FCW_PWP[0..7] region write-protect registers.
     * Each FCW operation already requires the unlock key, providing
     * inherent per-operation protection.
     */
    (void)addr;
    (void)len;

    if (!flashDev) {
        return WHAL_EINVAL;
    }

    return WHAL_SUCCESS;
}

whal_Error WHAL_DRV_FN(Pic32czFlash, unlock)(whal_Flash *flashDev, size_t addr, size_t len)
{
    (void)addr;
    (void)len;

    if (!flashDev) {
        return WHAL_EINVAL;
    }

    return WHAL_SUCCESS;
}

whal_Error WHAL_DRV_FN(Pic32czFlash, read)(whal_Flash *flashDev, size_t addr, uint8_t *data,
                             size_t dataSz)
{
    const whal_Regmap *reg;
    uint8_t *flashAddr = (uint8_t *)addr;
    size_t i;

    if (!flashDev || !data) {
        return WHAL_EINVAL;
    }

    reg = &flashDev->regmap;

    whal_Pic32czFlash_MutexLock(reg);

    /* Flash is memory-mapped; read directly */
    for (i = 0; i < dataSz; i++) {
        data[i] = flashAddr[i];
    }

    whal_Pic32czFlash_MutexUnlock(reg);

    return WHAL_SUCCESS;
}

whal_Error WHAL_DRV_FN(Pic32czFlash, write)(whal_Flash *flashDev, size_t addr, const uint8_t *data,
                              size_t dataSz)
{
    const whal_Regmap *reg;
    const uint32_t *src;
    whal_Error err;
    size_t offset = 0;

    if (!flashDev || !data) {
        return WHAL_EINVAL;
    }

    /* Require double-word alignment */
    if ((addr & 0x7) || (dataSz & 0x7)) {
        return WHAL_EINVAL;
    }

    reg = &flashDev->regmap;
    src = (const uint32_t *)data;

    whal_Pic32czFlash_MutexLock(reg);

    while (offset < dataSz) {
        size_t curAddr = addr + offset;
        size_t remaining = dataSz - offset;

        whal_Pic32czFlash_WaitBusy(reg);

        if (!(curAddr & 0x1F) && remaining >= FCW_QDWORD_SIZE) {
            /* Quad double word write (32 bytes) */
            size_t j;
            for (j = 0; j < 8; j++) {
                whal_Reg_Update(reg->base, FCW_DATA_REG(j),
                                0xFFFFFFFF, src[offset / 4 + j]);
            }
            whal_Reg_Update(reg->base, FCW_ADDR_REG, 0xFFFFFFFF, curAddr);

            err = whal_Pic32czFlash_ExecCmd(reg,
                                            FCW_CTRLA_NVMOP_QUAD_DWORD);
            if (err) {
                whal_Pic32czFlash_MutexUnlock(reg);
                return err;
            }
            offset += FCW_QDWORD_SIZE;
        } else {
            /* Single double word write (8 bytes) */
            whal_Reg_Update(reg->base, FCW_DATA_REG(0),
                            0xFFFFFFFF, src[offset / 4]);
            whal_Reg_Update(reg->base, FCW_DATA_REG(1),
                            0xFFFFFFFF, src[offset / 4 + 1]);
            whal_Reg_Update(reg->base, FCW_ADDR_REG, 0xFFFFFFFF, curAddr);

            err = whal_Pic32czFlash_ExecCmd(reg,
                                            FCW_CTRLA_NVMOP_SINGLE_DWORD);
            if (err) {
                whal_Pic32czFlash_MutexUnlock(reg);
                return err;
            }
            offset += FCW_DWORD_SIZE;
        }

    }

    whal_Pic32czFlash_MutexUnlock(reg);

    return WHAL_SUCCESS;
}

whal_Error WHAL_DRV_FN(Pic32czFlash, erase)(whal_Flash *flashDev, size_t addr, size_t dataSz)
{
    const whal_Regmap *reg;
    whal_Error err;
    size_t pageAddr;
    size_t endAddr;

    if (!flashDev) {
        return WHAL_EINVAL;
    }

    reg = &flashDev->regmap;

    /* Align down to page boundary */
    pageAddr = addr & ~(FCW_PAGE_SIZE - 1);
    endAddr = addr + dataSz;

    whal_Pic32czFlash_MutexLock(reg);

    while (pageAddr < endAddr) {
        whal_Pic32czFlash_WaitBusy(reg);

        whal_Reg_Update(reg->base, FCW_ADDR_REG, 0xFFFFFFFF, pageAddr);

        err = whal_Pic32czFlash_ExecCmd(reg, FCW_CTRLA_NVMOP_PAGE_ERASE);
        if (err) {
            whal_Pic32czFlash_MutexUnlock(reg);
            return err;
        }

        pageAddr += FCW_PAGE_SIZE;
    }

    whal_Pic32czFlash_MutexUnlock(reg);

    return WHAL_SUCCESS;
}

