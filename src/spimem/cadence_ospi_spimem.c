/* cadence_ospi_spimem.c
 *
 * Copyright (C) 2026 wolfSSL Inc.
 *
 * This file is part of wolfHAL.
 *
 * wolfHAL is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * wolfHAL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1335, USA
 */

#include <stdint.h>
#ifdef WHAL_CFG_CADENCE_OSPI_SPIMEM_SINGLE_INSTANCE
#include "wolfHAL_board.h"  /* provides WHAL_CFG_CADENCE_OSPI_SPIMEM_DEV */
#endif
#include <wolfHAL/spimem/cadence_ospi_spimem.h>
#include <wolfHAL/spimem/spimem.h>
#include <wolfHAL/error.h>
#include <wolfHAL/reg.h>
#include <wolfHAL/timeout.h>
#include <wolfHAL/bitops.h>

/* Cadence OSPI registers (AM028 OSPI module, AM026 ch. 66), all 32 bits */

/* CONFIG - controller enable, clock divider, chip select, DTR, PHY, idle
 * status */
#define OSPI_CONFIG_REG 0x00
#define OSPI_CONFIG_ENB_SPI_Pos 0 /* Controller enable */
#define OSPI_CONFIG_ENB_SPI_Msk (1UL << OSPI_CONFIG_ENB_SPI_Pos)

#define OSPI_CONFIG_PHY_MODE_EN_Pos 3 /* PHY mode enable */
#define OSPI_CONFIG_PHY_MODE_EN_Msk (1UL << OSPI_CONFIG_PHY_MODE_EN_Pos)

#define OSPI_CONFIG_PERIPH_CS_LINES_Pos 10 /* Chip select lines */
#define OSPI_CONFIG_PERIPH_CS_LINES_Msk \
    (WHAL_BITMASK(4) << OSPI_CONFIG_PERIPH_CS_LINES_Pos)

#define OSPI_CONFIG_ENB_DMA_IF_Pos 15 /* DMA peripheral interface enable */
#define OSPI_CONFIG_ENB_DMA_IF_Msk (1UL << OSPI_CONFIG_ENB_DMA_IF_Pos)

#define OSPI_CONFIG_MSTR_BAUD_DIV_Pos 19 /* Clock divider */
#define OSPI_CONFIG_MSTR_BAUD_DIV_Msk \
    (WHAL_BITMASK(4) << OSPI_CONFIG_MSTR_BAUD_DIV_Pos)

#define OSPI_CONFIG_ENABLE_DTR_Pos 24 /* DTR protocol enable */
#define OSPI_CONFIG_ENABLE_DTR_Msk (1UL << OSPI_CONFIG_ENABLE_DTR_Pos)

#define OSPI_CONFIG_PIPELINE_PHY_Pos 25 /* Pipeline PHY mode enable */
#define OSPI_CONFIG_PIPELINE_PHY_Msk (1UL << OSPI_CONFIG_PIPELINE_PHY_Pos)

#define OSPI_CONFIG_DUAL_BYTE_OPCODE_Pos 30 /* Dual-byte opcode enable */
#define OSPI_CONFIG_DUAL_BYTE_OPCODE_Msk \
    (1UL << OSPI_CONFIG_DUAL_BYTE_OPCODE_Pos)

#define OSPI_CONFIG_IDLE_Pos 31 /* Serial interface idle (retimed) */
#define OSPI_CONFIG_IDLE_Msk (1UL << OSPI_CONFIG_IDLE_Pos)

/* DEV_INSTR_RD_CONFIG - read opcode and lines per phase; STIG uses its
 * instr type (AM028). Line fields: 0 = 1 line, 1 = 2, 2 = 4, 3 = 8. */
#define OSPI_DEV_INSTR_RD_CONFIG_REG 0x04
#define OSPI_DEV_INSTR_RD_CONFIG_RESET 0x3 /* Reset value: 03h, 1-bit */
#define OSPI_DEV_INSTR_RD_OPCODE_Pos 0 /* Read opcode */
#define OSPI_DEV_INSTR_RD_OPCODE_Msk \
    (WHAL_BITMASK(8) << OSPI_DEV_INSTR_RD_OPCODE_Pos)

#define OSPI_DEV_INSTR_RD_INSTR_TYPE_Pos 8 /* Opcode lines */
#define OSPI_DEV_INSTR_RD_INSTR_TYPE_Msk \
    (WHAL_BITMASK(2) << OSPI_DEV_INSTR_RD_INSTR_TYPE_Pos)

#define OSPI_DEV_INSTR_RD_ADDR_TYPE_Pos 12 /* Address lines */
#define OSPI_DEV_INSTR_RD_ADDR_TYPE_Msk \
    (WHAL_BITMASK(2) << OSPI_DEV_INSTR_RD_ADDR_TYPE_Pos)

#define OSPI_DEV_INSTR_RD_DATA_TYPE_Pos 16 /* Data lines */
#define OSPI_DEV_INSTR_RD_DATA_TYPE_Msk \
    (WHAL_BITMASK(2) << OSPI_DEV_INSTR_RD_DATA_TYPE_Pos)

/* DEV_INSTR_WR_CONFIG - write opcode and the address and data lines */
#define OSPI_DEV_INSTR_WR_CONFIG_REG 0x08
#define OSPI_DEV_INSTR_WR_CONFIG_RESET 0x2 /* Reset value: 02h, 1-bit */

#define OSPI_DEV_INSTR_WR_ADDR_TYPE_Pos 12 /* Address lines */
#define OSPI_DEV_INSTR_WR_ADDR_TYPE_Msk \
    (WHAL_BITMASK(2) << OSPI_DEV_INSTR_WR_ADDR_TYPE_Pos)

#define OSPI_DEV_INSTR_WR_DATA_TYPE_Pos 16 /* Data lines */
#define OSPI_DEV_INSTR_WR_DATA_TYPE_Msk \
    (WHAL_BITMASK(2) << OSPI_DEV_INSTR_WR_DATA_TYPE_Pos)

/* RD_DATA_CAPTURE - read data sample delay */
#define OSPI_RD_DATA_CAPTURE_REG 0x10

/* DEV_SIZE_CONFIG - address bytes, page, block and flash size */
#define OSPI_DEV_SIZE_CONFIG_REG 0x14
#define OSPI_DEV_SIZE_ADDR_BYTES_Msk 0xFUL

/* SRAM_PARTITION_CFG - SRAM split between indirect read and write */
#define OSPI_SRAM_PARTITION_CFG_REG 0x18

/* IND_AHB_ADDR_TRIGGER - indirect trigger address */
#define OSPI_IND_AHB_ADDR_TRIG_REG 0x1C

/* DMA_PERIPH_CONFIG - DMA single and burst sizes */
#define OSPI_DMA_PERIPH_CONFIG_REG 0x20

/* INDIRECT_READ_XFER_CTRL - start or cancel an indirect read */
#define OSPI_IND_RD_XFER_CTRL_REG 0x60
#define OSPI_IND_RD_XFER_START_Pos 0 /* Start indirect read */
#define OSPI_IND_RD_XFER_START_Msk (1UL << OSPI_IND_RD_XFER_START_Pos)

#define OSPI_IND_RD_XFER_CANCEL_Pos 1 /* Cancel indirect reads */
#define OSPI_IND_RD_XFER_CANCEL_Msk (1UL << OSPI_IND_RD_XFER_CANCEL_Pos)

/* INDIRECT_READ_XFER_WATERMARK - SRAM fill level for a DMA request */
#define OSPI_IND_RD_XFER_WATERMARK_REG 0x64

/* INDIRECT_READ_XFER_START - flash start address of an indirect read */
#define OSPI_IND_RD_XFER_START_REG 0x68

/* INDIRECT_READ_XFER_NUM_BYTES - bytes in an indirect read */
#define OSPI_IND_RD_XFER_NUM_BYTES_REG 0x6C

/* INDIRECT_TRIGGER_ADDR_RANGE - trigger window size (2^n bytes) */
#define OSPI_IND_TRIG_ADDR_RANGE_REG 0x80

/* FLASH_CMD_CTRL - STIG command setup and start */
#define OSPI_FLASH_CMD_CTRL_REG 0x90
#define OSPI_FLASH_CMD_CTRL_EXEC_Pos 0 /* Execute the command */
#define OSPI_FLASH_CMD_CTRL_EXEC_Msk (1UL << OSPI_FLASH_CMD_CTRL_EXEC_Pos)

#define OSPI_FLASH_CMD_CTRL_EXEC_STATUS_Pos 1 /* Command in progress */
#define OSPI_FLASH_CMD_CTRL_EXEC_STATUS_Msk \
    (1UL << OSPI_FLASH_CMD_CTRL_EXEC_STATUS_Pos)

#define OSPI_FLASH_CMD_CTRL_NUM_DUMMY_Pos 7 /* Dummy cycles */
#define OSPI_FLASH_CMD_CTRL_NUM_DUMMY_Msk \
    (WHAL_BITMASK(5) << OSPI_FLASH_CMD_CTRL_NUM_DUMMY_Pos)

#define OSPI_FLASH_CMD_CTRL_NUM_WR_BYTES_Pos 12 /* Write bytes - 1 */
#define OSPI_FLASH_CMD_CTRL_NUM_WR_BYTES_Msk \
    (WHAL_BITMASK(3) << OSPI_FLASH_CMD_CTRL_NUM_WR_BYTES_Pos)

#define OSPI_FLASH_CMD_CTRL_ENB_WRITE_Pos 15 /* Write data enable */
#define OSPI_FLASH_CMD_CTRL_ENB_WRITE_Msk \
    (1UL << OSPI_FLASH_CMD_CTRL_ENB_WRITE_Pos)

#define OSPI_FLASH_CMD_CTRL_NUM_ADDR_BYTES_Pos 16 /* Address bytes - 1 */
#define OSPI_FLASH_CMD_CTRL_NUM_ADDR_BYTES_Msk \
    (WHAL_BITMASK(2) << OSPI_FLASH_CMD_CTRL_NUM_ADDR_BYTES_Pos)

#define OSPI_FLASH_CMD_CTRL_ENB_ADDR_Pos 19 /* Address enable */
#define OSPI_FLASH_CMD_CTRL_ENB_ADDR_Msk \
    (1UL << OSPI_FLASH_CMD_CTRL_ENB_ADDR_Pos)

#define OSPI_FLASH_CMD_CTRL_NUM_RD_BYTES_Pos 20 /* Read bytes - 1 */
#define OSPI_FLASH_CMD_CTRL_NUM_RD_BYTES_Msk \
    (WHAL_BITMASK(3) << OSPI_FLASH_CMD_CTRL_NUM_RD_BYTES_Pos)

#define OSPI_FLASH_CMD_CTRL_ENB_READ_Pos 23 /* Read data enable */
#define OSPI_FLASH_CMD_CTRL_ENB_READ_Msk \
    (1UL << OSPI_FLASH_CMD_CTRL_ENB_READ_Pos)

#define OSPI_FLASH_CMD_CTRL_OPCODE_Pos 24 /* Command opcode */
#define OSPI_FLASH_CMD_CTRL_OPCODE_Msk \
    (WHAL_BITMASK(8) << OSPI_FLASH_CMD_CTRL_OPCODE_Pos)

/* FLASH_CMD_ADDR - STIG command address */
#define OSPI_FLASH_CMD_ADDR_REG 0x94

/* FLASH_RD_DATA_LOWER/UPPER - STIG read data, bytes 0-3 and 4-7 */
#define OSPI_FLASH_RD_DATA_LOWER_REG 0xA0
#define OSPI_FLASH_RD_DATA_UPPER_REG 0xA4

/* FLASH_WR_DATA_LOWER/UPPER - STIG write data, bytes 0-3 and 4-7 */
#define OSPI_FLASH_WR_DATA_LOWER_REG 0xA8
#define OSPI_FLASH_WR_DATA_UPPER_REG 0xAC

/* STIG limits per command (AM028 flash_cmd_ctrl_reg). */
#define OSPI_STIG_MAX_ADDR_BYTES 4
#define OSPI_STIG_MAX_DATA_BYTES 8
#define OSPI_STIG_MAX_DUMMY      31

/* DMA start address and length are 4-byte aligned (AM026 p706, p709). */
#define OSPI_DMA_MIN_ALIGN 4

#ifdef WHAL_CFG_CADENCE_OSPI_SPIMEM_DIRECT_API_MAPPING
#define whal_CadenceOspi_SpiMem_Init      whal_SpiMem_Init
#define whal_CadenceOspi_SpiMem_Deinit    whal_SpiMem_Deinit
#define whal_CadenceOspi_SpiMem_Exec      whal_SpiMem_Exec
#define whal_CadenceOspi_SpiMem_MaxDataSz whal_SpiMem_MaxDataSz
#endif /* WHAL_CFG_CADENCE_OSPI_SPIMEM_DIRECT_API_MAPPING */

#ifdef WHAL_CFG_CADENCE_OSPI_SPIMEM_SINGLE_INSTANCE
const whal_SpiMem whal_CadenceOspi_SpiMem_Dev = WHAL_CFG_CADENCE_OSPI_SPIMEM_DEV;
#endif

/* Wait for the controller to be idle (AM028 config_reg idle_fld). */
static whal_Error whal_CadenceOspi_SpiMem_WaitIdle(uintptr_t base,
                                                   whal_Timeout *timeout)
{
    return whal_Reg32_ReadPoll(base, OSPI_CONFIG_REG, OSPI_CONFIG_IDLE_Msk,
                               OSPI_CONFIG_IDLE_Msk, timeout);
}

/* Line count to instr register field (AM028 dev_instr_rd_config_reg).
 * WHAL_EINVAL if not 1, 2, 4 or 8. */
static whal_Error whal_CadenceOspi_SpiMem_LinesField(uint8_t lines,
                                                     uint32_t *field)
{
    if (lines == WHAL_SPIMEM_LINES_1) {
        *field = 0;
    }
    else if (lines == WHAL_SPIMEM_LINES_2) {
        *field = 1;
    }
    else if (lines == WHAL_SPIMEM_LINES_4) {
        *field = 2;
    }
    else if (lines == WHAL_SPIMEM_LINES_8) {
        *field = 3;
    }
    else {
        return WHAL_EINVAL;
    }
    return WHAL_SUCCESS;
}

/* Set op's lines per phase (unused phases 1 line) and DTR. DTR changes with
 * the controller off, as for the clock (AM028 config_reg). */
static whal_Error whal_CadenceOspi_SpiMem_SetBusMode(uintptr_t base,
                                                     whal_Timeout *timeout,
                                                     const whal_SpiMemOp *op)
{
    uint32_t instrType;
    uint32_t addrType = 0;
    uint32_t dataType = 0;
    uint32_t config;
    uint32_t dtr = 0;
    whal_Error err;

    err = whal_CadenceOspi_SpiMem_LinesField(op->opLines, &instrType);
    if (err) {
        return err;
    }
    if (op->addrBytes > 0) {
        err = whal_CadenceOspi_SpiMem_LinesField(op->addrLines, &addrType);
        if (err) {
            return err;
        }
    }
    if (op->dir != WHAL_SPIMEM_DIR_NONE) {
        err = whal_CadenceOspi_SpiMem_LinesField(op->dataLines, &dataType);
        if (err) {
            return err;
        }
    }
    if (op->dtr) {
        dtr = OSPI_CONFIG_ENABLE_DTR_Msk;
    }

    whal_Reg32_Update(base, OSPI_DEV_INSTR_RD_CONFIG_REG,
                      OSPI_DEV_INSTR_RD_INSTR_TYPE_Msk |
                      OSPI_DEV_INSTR_RD_ADDR_TYPE_Msk |
                      OSPI_DEV_INSTR_RD_DATA_TYPE_Msk,
                      whal_SetBits(OSPI_DEV_INSTR_RD_INSTR_TYPE_Msk,
                                   OSPI_DEV_INSTR_RD_INSTR_TYPE_Pos,
                                   instrType) |
                      whal_SetBits(OSPI_DEV_INSTR_RD_ADDR_TYPE_Msk,
                                   OSPI_DEV_INSTR_RD_ADDR_TYPE_Pos,
                                   addrType) |
                      whal_SetBits(OSPI_DEV_INSTR_RD_DATA_TYPE_Msk,
                                   OSPI_DEV_INSTR_RD_DATA_TYPE_Pos,
                                   dataType));
    whal_Reg32_Update(base, OSPI_DEV_INSTR_WR_CONFIG_REG,
                      OSPI_DEV_INSTR_WR_ADDR_TYPE_Msk |
                      OSPI_DEV_INSTR_WR_DATA_TYPE_Msk,
                      whal_SetBits(OSPI_DEV_INSTR_WR_ADDR_TYPE_Msk,
                                   OSPI_DEV_INSTR_WR_ADDR_TYPE_Pos,
                                   addrType) |
                      whal_SetBits(OSPI_DEV_INSTR_WR_DATA_TYPE_Msk,
                                   OSPI_DEV_INSTR_WR_DATA_TYPE_Pos,
                                   dataType));

    config = whal_Reg32_Read(base, OSPI_CONFIG_REG);
    if ((config & OSPI_CONFIG_ENABLE_DTR_Msk) == dtr) {
        return WHAL_SUCCESS;
    }
    config &= ~(uint32_t)(OSPI_CONFIG_ENB_SPI_Msk |
                          OSPI_CONFIG_ENABLE_DTR_Msk);
    whal_Reg32_Write(base, OSPI_CONFIG_REG, config);
    whal_Reg32_Write(base, OSPI_CONFIG_REG, config | dtr);
    whal_Reg32_Write(base, OSPI_CONFIG_REG,
                     config | dtr | OSPI_CONFIG_ENB_SPI_Msk);
    return whal_CadenceOspi_SpiMem_WaitIdle(base, timeout);
}

/* Send one STIG command: opcode, addrBytes address bytes, dummy cycles and
 * up to 8 data bytes in (dir IN) or out (dir OUT). */
static whal_Error whal_CadenceOspi_SpiMem_Stig(uintptr_t base,
                                               whal_Timeout *timeout,
                                               uint8_t opcode, uint32_t addr,
                                               uint8_t addrBytes,
                                               uint8_t dummyCycles,
                                               uint8_t dir, uint8_t *data,
                                               size_t len)
{
    uint32_t ctrl;
    uint32_t lo;
    uint32_t hi;
    size_t i;
    whal_Error err;

    if (addrBytes > OSPI_STIG_MAX_ADDR_BYTES ||
        dummyCycles > OSPI_STIG_MAX_DUMMY ||
        len > OSPI_STIG_MAX_DATA_BYTES) {
        return WHAL_EINVAL;
    }
    if (len > 0 && !data) {
        return WHAL_EINVAL;
    }

    ctrl = whal_SetBits(OSPI_FLASH_CMD_CTRL_OPCODE_Msk,
                        OSPI_FLASH_CMD_CTRL_OPCODE_Pos, (uint32_t)opcode) |
           whal_SetBits(OSPI_FLASH_CMD_CTRL_NUM_DUMMY_Msk,
                        OSPI_FLASH_CMD_CTRL_NUM_DUMMY_Pos, dummyCycles);
    if (addrBytes > 0) {
        ctrl |= OSPI_FLASH_CMD_CTRL_ENB_ADDR_Msk |
                whal_SetBits(OSPI_FLASH_CMD_CTRL_NUM_ADDR_BYTES_Msk,
                             OSPI_FLASH_CMD_CTRL_NUM_ADDR_BYTES_Pos,
                             addrBytes - 1);
    }
    if (len > 0 && dir == WHAL_SPIMEM_DIR_OUT) {
        ctrl |= OSPI_FLASH_CMD_CTRL_ENB_WRITE_Msk |
                whal_SetBits(OSPI_FLASH_CMD_CTRL_NUM_WR_BYTES_Msk,
                             OSPI_FLASH_CMD_CTRL_NUM_WR_BYTES_Pos, len - 1);
    }
    if (len > 0 && dir == WHAL_SPIMEM_DIR_IN) {
        ctrl |= OSPI_FLASH_CMD_CTRL_ENB_READ_Msk |
                whal_SetBits(OSPI_FLASH_CMD_CTRL_NUM_RD_BYTES_Msk,
                             OSPI_FLASH_CMD_CTRL_NUM_RD_BYTES_Pos, len - 1);
    }

    whal_Reg32_Write(base, OSPI_FLASH_CMD_ADDR_REG, addr);
    if (len > 0 && dir == WHAL_SPIMEM_DIR_OUT) {
        /* Byte 0 goes out first, from the low byte of the lower register. */
        lo = 0;
        hi = 0;
        for (i = 0; i < len && i < 4; i++) {
            lo |= (uint32_t)data[i] << (8 * i);
        }
        for (i = 4; i < len; i++) {
            hi |= (uint32_t)data[i] << (8 * (i - 4));
        }
        whal_Reg32_Write(base, OSPI_FLASH_WR_DATA_LOWER_REG, lo);
        if (len > 4) {
            whal_Reg32_Write(base, OSPI_FLASH_WR_DATA_UPPER_REG, hi);
        }
    }
    whal_Reg32_Write(base, OSPI_FLASH_CMD_CTRL_REG,
                     ctrl | OSPI_FLASH_CMD_CTRL_EXEC_Msk);

    err = whal_Reg32_ReadPoll(base, OSPI_FLASH_CMD_CTRL_REG,
                              OSPI_FLASH_CMD_CTRL_EXEC_STATUS_Msk, 0, timeout);
    if (err) {
        return err;
    }
    /* Then CONFIG idle (bit 31, lags done). On the VEK385, write data set
     * before idle is lost and the next page program writes nothing. */
    err = whal_CadenceOspi_SpiMem_WaitIdle(base, timeout);
    if (err) {
        return err;
    }

    if (len > 0 && dir == WHAL_SPIMEM_DIR_IN) {
        lo = whal_Reg32_Read(base, OSPI_FLASH_RD_DATA_LOWER_REG);
        hi = 0;
        if (len > 4) {
            hi = whal_Reg32_Read(base, OSPI_FLASH_RD_DATA_UPPER_REG);
        }
        for (i = 0; i < len && i < 4; i++) {
            data[i] = (uint8_t)(lo >> (8 * i));
        }
        for (i = 4; i < len; i++) {
            data[i] = (uint8_t)(hi >> (8 * (i - 4)));
        }
    }

    return WHAL_SUCCESS;
}

/* Read len bytes at addr with STIG, 8 bytes per command. */
static whal_Error whal_CadenceOspi_SpiMem_StigRead(uintptr_t base,
                                                   whal_Timeout *timeout,
                                                   const whal_SpiMemOp *op,
                                                   uint32_t addr,
                                                   uint8_t *data, size_t len)
{
    size_t off;
    size_t n;
    whal_Error err;

    for (off = 0; off < len; off += n) {
        n = len - off;
        if (n > OSPI_STIG_MAX_DATA_BYTES) {
            n = OSPI_STIG_MAX_DATA_BYTES;
        }
        err = whal_CadenceOspi_SpiMem_Stig(base, timeout, op->opcode,
                                           addr + (uint32_t)off,
                                           op->addrBytes, op->dummyCycles,
                                           WHAL_SPIMEM_DIR_IN, data + off, n);
        if (err) {
            return err;
        }
    }
    return WHAL_SUCCESS;
}

/* Read len bytes at addr with an indirect read and the DMA hook. addr and
 * len are 4-byte aligned, buf and len meet dma->align. */
static whal_Error whal_CadenceOspi_SpiMem_DmaRead(
    uintptr_t base, const whal_CadenceOspi_SpiMem_Cfg *cfg, uint8_t opcode,
    uint32_t addr, uint8_t *buf, size_t len)
{
    const whal_CadenceOspi_SpiMem_Dma *dma = cfg->dma;
    whal_Error err;

    /* The indirect read sends the opcode in DEV_INSTR_RD_CONFIG, with the
     * lines Exec set there. */
    whal_Reg32_Update(base, OSPI_DEV_INSTR_RD_CONFIG_REG,
                      OSPI_DEV_INSTR_RD_OPCODE_Msk,
                      whal_SetBits(OSPI_DEV_INSTR_RD_OPCODE_Msk,
                                   OSPI_DEV_INSTR_RD_OPCODE_Pos,
                                   (uint32_t)opcode));

    /* Turn on the DMA interface (AM028 config_reg enb_dma_if_fld). */
    whal_Reg32_Update(base, OSPI_CONFIG_REG, OSPI_CONFIG_ENB_DMA_IF_Msk,
                      OSPI_CONFIG_ENB_DMA_IF_Msk);

    /* DMA settings (AM026 Table 223) and the read itself. */
    whal_Reg32_Write(base, OSPI_DMA_PERIPH_CONFIG_REG, cfg->dmaPeriphCfg);
    whal_Reg32_Write(base, OSPI_SRAM_PARTITION_CFG_REG, cfg->sramPartition);
    whal_Reg32_Write(base, OSPI_IND_RD_XFER_WATERMARK_REG, 0);
    whal_Reg32_Write(base, OSPI_IND_AHB_ADDR_TRIG_REG, cfg->indTrigBase);
    whal_Reg32_Write(base, OSPI_IND_TRIG_ADDR_RANGE_REG, cfg->indTrigRange);
    whal_Reg32_Write(base, OSPI_IND_RD_XFER_START_REG, addr);
    whal_Reg32_Write(base, OSPI_IND_RD_XFER_NUM_BYTES_REG, len);

    /* Set up the DMA first, then start the indirect read (AM026 p709). */
    err = dma->Arm(dma->ctx, buf, len);
    if (err) {
        return err;
    }
    whal_Reg32_Write(base, OSPI_IND_RD_XFER_CTRL_REG,
                     OSPI_IND_RD_XFER_START_Msk);

    err = dma->Wait(dma->ctx, buf, len);
    if (err) {
        goto cleanup;
    }

    /* The next command must wait until this access is done (AM026 p705,
     * Idle Status Bit). */
    err = whal_CadenceOspi_SpiMem_WaitIdle(base, cfg->timeout);

cleanup:
    if (err) {
        /* Stop the indirect read so the next command can run (AM028
         * indirect_read_xfer_ctrl_reg cancel_fld). */
        whal_Reg32_Write(base, OSPI_IND_RD_XFER_CTRL_REG,
                         OSPI_IND_RD_XFER_CANCEL_Msk);
    }
    return err;
}

/* DMA reads the aligned middle (with a hook, no dummy, no DTR, devSize
 * address size), STIG the rest. A DMA error fails the read. */
static whal_Error whal_CadenceOspi_SpiMem_Read(
    uintptr_t base, const whal_CadenceOspi_SpiMem_Cfg *cfg,
    const whal_SpiMemOp *op)
{
    uint8_t *data = (uint8_t *)op->data;
    size_t len = op->dataSz;
    size_t align;
    size_t head;
    size_t body;
    size_t tail;
    whal_Error err;

    /* The indirect read sends the DEV_SIZE_CONFIG address size (AM028
     * dev_size_config_reg num_addr_bytes_fld, 0 = 1 byte). */
    if (cfg->dma && op->dummyCycles == 0 && !op->dtr &&
        op->addrBytes ==
            (cfg->devSize & OSPI_DEV_SIZE_ADDR_BYTES_Msk) + 1) {
        if (!cfg->dma->Arm || !cfg->dma->Wait) {
            return WHAL_EINVAL;
        }
        align = cfg->dma->align;
        if (align < OSPI_DMA_MIN_ALIGN) {
            align = OSPI_DMA_MIN_ALIGN;
        }
        head = (align - ((uintptr_t)data % align)) % align;
        if (head > len) {
            head = len;
        }
        body = ((len - head) / align) * align;
        tail = len - head - body;

        if (body > 0 && ((op->addr + (uint32_t)head) & 3) == 0) {
            err = whal_CadenceOspi_SpiMem_StigRead(base, cfg->timeout, op,
                                                   op->addr, data, head);
            if (err) {
                return err;
            }
            err = whal_CadenceOspi_SpiMem_DmaRead(base, cfg, op->opcode,
                                                  op->addr + (uint32_t)head,
                                                  data + head, body);
            if (err) {
                return err;
            }
            return whal_CadenceOspi_SpiMem_StigRead(
                base, cfg->timeout, op, op->addr + (uint32_t)(head + body),
                data + head + body, tail);
        }
    }

    return whal_CadenceOspi_SpiMem_StigRead(base, cfg->timeout, op, op->addr,
                                            data, len);
}

whal_Error whal_CadenceOspi_SpiMem_Init(whal_SpiMem *spiMemDev)
{
    const whal_CadenceOspi_SpiMem_Cfg *cfg;
    uintptr_t base;
    uint32_t config;

#ifdef WHAL_CFG_CADENCE_OSPI_SPIMEM_SINGLE_INSTANCE
    cfg = (const whal_CadenceOspi_SpiMem_Cfg *)whal_CadenceOspi_SpiMem_Dev.cfg;
    base = whal_CadenceOspi_SpiMem_Dev.base;
    (void)spiMemDev;
#else
    if (!spiMemDev || !spiMemDev->cfg) {
        return WHAL_EINVAL;
    }
    cfg = (const whal_CadenceOspi_SpiMem_Cfg *)spiMemDev->cfg;
    base = spiMemDev->base;
#endif

    /* Controller off while clock and CS change (AM028 config_reg). DTR,
     * PHY, pipeline PHY and dual opcode an earlier stage left go off. */
    config = whal_Reg32_Read(base, OSPI_CONFIG_REG);
    config &= ~(uint32_t)OSPI_CONFIG_ENB_SPI_Msk;
    whal_Reg32_Write(base, OSPI_CONFIG_REG, config);
    config &= ~(uint32_t)(OSPI_CONFIG_MSTR_BAUD_DIV_Msk |
                          OSPI_CONFIG_PERIPH_CS_LINES_Msk |
                          OSPI_CONFIG_ENABLE_DTR_Msk |
                          OSPI_CONFIG_PHY_MODE_EN_Msk |
                          OSPI_CONFIG_PIPELINE_PHY_Msk |
                          OSPI_CONFIG_DUAL_BYTE_OPCODE_Msk);
    config |= whal_SetBits(OSPI_CONFIG_MSTR_BAUD_DIV_Msk,
                           OSPI_CONFIG_MSTR_BAUD_DIV_Pos, cfg->baudDiv) |
              whal_SetBits(OSPI_CONFIG_PERIPH_CS_LINES_Msk,
                           OSPI_CONFIG_PERIPH_CS_LINES_Pos, cfg->csLines);
    whal_Reg32_Write(base, OSPI_CONFIG_REG, config);
    whal_Reg32_Write(base, OSPI_RD_DATA_CAPTURE_REG, cfg->rdCapture);

    /* Instruction registers at their reset values (AM028): 1-bit for
     * every phase. Exec sets the lines for each operation. */
    whal_Reg32_Write(base, OSPI_DEV_INSTR_RD_CONFIG_REG,
                     OSPI_DEV_INSTR_RD_CONFIG_RESET);
    whal_Reg32_Write(base, OSPI_DEV_INSTR_WR_CONFIG_REG,
                     OSPI_DEV_INSTR_WR_CONFIG_RESET);

    /* Write the whole value (AM028 dev_size_config_reg) so the other
     * fields are not left at a wrong value. */
    whal_Reg32_Write(base, OSPI_DEV_SIZE_CONFIG_REG, cfg->devSize);

    whal_Reg32_Write(base, OSPI_CONFIG_REG, config | OSPI_CONFIG_ENB_SPI_Msk);

    return whal_CadenceOspi_SpiMem_WaitIdle(base, cfg->timeout);
}

whal_Error whal_CadenceOspi_SpiMem_Deinit(whal_SpiMem *spiMemDev)
{
    uintptr_t base;

#ifdef WHAL_CFG_CADENCE_OSPI_SPIMEM_SINGLE_INSTANCE
    base = whal_CadenceOspi_SpiMem_Dev.base;
    (void)spiMemDev;
#else
    if (!spiMemDev) {
        return WHAL_EINVAL;
    }
    base = spiMemDev->base;
#endif

    whal_Reg32_Update(base, OSPI_CONFIG_REG, OSPI_CONFIG_ENB_SPI_Msk, 0);

    return WHAL_SUCCESS;
}

whal_Error whal_CadenceOspi_SpiMem_Exec(whal_SpiMem *spiMemDev,
                                        const whal_SpiMemOp *op)
{
    const whal_CadenceOspi_SpiMem_Cfg *cfg;
    uintptr_t base;
    whal_Error err;

#ifdef WHAL_CFG_CADENCE_OSPI_SPIMEM_SINGLE_INSTANCE
    cfg = (const whal_CadenceOspi_SpiMem_Cfg *)whal_CadenceOspi_SpiMem_Dev.cfg;
    base = whal_CadenceOspi_SpiMem_Dev.base;
    (void)spiMemDev;
#else
    if (!spiMemDev || !spiMemDev->cfg) {
        return WHAL_EINVAL;
    }
    cfg = (const whal_CadenceOspi_SpiMem_Cfg *)spiMemDev->cfg;
    base = spiMemDev->base;
#endif

    if (!op) {
        return WHAL_EINVAL;
    }
    if (op->dataSz > 0 && !op->data) {
        return WHAL_EINVAL;
    }
    if (op->addrBytes > OSPI_STIG_MAX_ADDR_BYTES ||
        op->dummyCycles > OSPI_STIG_MAX_DUMMY) {
        return WHAL_EINVAL;
    }
    if (op->dir != WHAL_SPIMEM_DIR_NONE && op->dir != WHAL_SPIMEM_DIR_IN &&
        op->dir != WHAL_SPIMEM_DIR_OUT) {
        return WHAL_EINVAL;
    }
    if (op->dir == WHAL_SPIMEM_DIR_NONE && op->dataSz > 0) {
        return WHAL_EINVAL;
    }
    /* A DDR read is an even number of bytes (AM026 p703). */
    if (op->dtr && op->dir == WHAL_SPIMEM_DIR_IN && (op->dataSz & 1)) {
        return WHAL_EINVAL;
    }

    err = whal_CadenceOspi_SpiMem_SetBusMode(base, cfg->timeout, op);
    if (err) {
        return err;
    }

    if (op->dir == WHAL_SPIMEM_DIR_IN && op->addrBytes > 0 &&
        op->dataSz > 0) {
        return whal_CadenceOspi_SpiMem_Read(base, cfg, op);
    }

    return whal_CadenceOspi_SpiMem_Stig(base, cfg->timeout, op->opcode,
                                        op->addr, op->addrBytes,
                                        op->dummyCycles, op->dir,
                                        (uint8_t *)op->data, op->dataSz);
}

whal_Error whal_CadenceOspi_SpiMem_MaxDataSz(whal_SpiMem *spiMemDev,
                                             const whal_SpiMemOp *op,
                                             size_t *maxSz)
{
#ifdef WHAL_CFG_CADENCE_OSPI_SPIMEM_SINGLE_INSTANCE
    (void)spiMemDev;
#else
    if (!spiMemDev) {
        return WHAL_EINVAL;
    }
#endif
    if (!op || !maxSz) {
        return WHAL_EINVAL;
    }

    /* A read with an address is split into commands, so it has no
     * limit. */
    if (op->dir == WHAL_SPIMEM_DIR_IN && op->addrBytes > 0) {
        *maxSz = SIZE_MAX;
    }
    else {
        *maxSz = OSPI_STIG_MAX_DATA_BYTES;
    }

    return WHAL_SUCCESS;
}

#ifndef WHAL_CFG_CADENCE_OSPI_SPIMEM_DIRECT_API_MAPPING
const whal_SpiMemDriver whal_CadenceOspi_SpiMem_Driver = {
    .Init = whal_CadenceOspi_SpiMem_Init,
    .Deinit = whal_CadenceOspi_SpiMem_Deinit,
    .Exec = whal_CadenceOspi_SpiMem_Exec,
    .MaxDataSz = whal_CadenceOspi_SpiMem_MaxDataSz,
};
#endif /* !WHAL_CFG_CADENCE_OSPI_SPIMEM_DIRECT_API_MAPPING */
