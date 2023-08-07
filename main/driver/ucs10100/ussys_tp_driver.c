/**
* @file ussys_tp_driver.c
*
* UltraSense Systems Touch Point Driver API
*
* Copyright (c) 2022 UltraSense Systems, Inc. All Rights Reserved.
*/

/*
* EDIT HISTORY FOR FILE
*
* This section contains comments describing changes made to the module.
*
*
* when         version    who              what
* ----------   --------   ----------       ---------------------------------
* 02/20/2023   1.0.0      Hanghang         base version for refactored FW.
* 03/11/2023   1.0.1      Hanghang         add python calibration noise measure code.
* 03/20/2023   1.0.2      Hanghang         trimming offset by using fd reg.
* 04/20/2023   1.0.3      Hanghang         fix algo update issue.
* 05/12/2023   1.0.4      Hanghang         fix Cap Auto Calibration issue.
*/

#include <math.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "BrahmsFW_C.h"
#include "ussys_tp_driver.h"
#if USSYS_CAP_ENABLED
#include "ussys_tp_cap_calibration.h"
#endif

/**********************************************************
 *********************** global values ********************
 **********************************************************/
static ussys_algo1st_ths_t algo1st_ths;

/**********************************************************
 ******************** Utlity macro & function *************
 **********************************************************/
void udelay(ussys_tp_dev_t *dev, uint32_t delay_us)
{
	uint64_t curr_ts_us = dev->get_timestamp_us();

	while((uint32_t)(dev->get_timestamp_us() - curr_ts_us) < delay_us) {
		
	}
}

void mdelay(ussys_tp_dev_t *dev, uint32_t delay_ms)
{
	udelay(dev, MS_TO_US(delay_ms));
}

/**********************************************************
 ******************** I2C functions ***********************
 **********************************************************/
/* [bd mode] write reg & memory */
static int ussys_tp_bd_write(ussys_tp_dev_t *dev, uint16_t reg_addr, uint8_t *val, uint16_t size)
{
	uint8_t *buf = malloc(size + 5);
	int rc = 0, i2c_retry_cnt = 0;

	if (NULL == buf)
		return -1;

	buf[0] = 0x02;/* bd write */
	buf[1] = (uint8_t)(reg_addr >> 8);
	buf[2] = (uint8_t)reg_addr;
	buf[3] = (uint8_t)(size >> 8);
	buf[4] = (uint8_t)size;
	memcpy(&buf[5], val, size);

	do {
		rc = dev->i2c_write(dev, buf, size + 5);
		if (rc == 0)
			break;
		i2c_retry_cnt++;
	} while (i2c_retry_cnt < USSYS_TP_I2C_MAX_RETRY_NUM);

	free(buf);

	if (i2c_retry_cnt == USSYS_TP_I2C_MAX_RETRY_NUM) {
		debug_info("dev%d(%#X) bd i2c write %#X failed! \r\n", dev->dev_idx, dev->i2c_addr, reg_addr);
		return -2;
	}

	return 0;
}

/* [bd mode] read reg & memory (oneshot) */
static int ussys_tp_bd_read_oneshot(ussys_tp_dev_t *dev, uint16_t reg_addr, uint8_t *val, uint16_t size)
{
	uint8_t buf[] = {0x01,/*bd read*/
					(uint8_t)(reg_addr >> 8),
					(uint8_t)reg_addr,
					(uint8_t)(size >> 8),
					(uint8_t)size};
	int rc = 0, i2c_retry_cnt = 0;

	do {
		rc = dev->i2c_write(dev, buf, ARRAY_SIZE(buf));
		rc += dev->i2c_read(dev, val, size);
		if (rc == 0)
			break;
		i2c_retry_cnt++;
	} while (i2c_retry_cnt < USSYS_TP_I2C_MAX_RETRY_NUM);

	if (i2c_retry_cnt == USSYS_TP_I2C_MAX_RETRY_NUM) {
		debug_info("dev%d(%#X) bd i2c read %#X failed! \r\n", dev->dev_idx, dev->i2c_addr, reg_addr);
		return -1;
	}

	return 0;
}

/* [bd mode] read reg & memory */
static int ussys_tp_bd_read(ussys_tp_dev_t *dev, uint16_t reg_addr, uint8_t *val, uint32_t size)
{
	uint32_t offset = 0;
	int rc = 0;

	while(offset <	size) {
		if( (size - offset) > USSYS_TP_I2C_CHUNK_SIZE) {
			rc += ussys_tp_bd_read_oneshot(dev, reg_addr + offset, val + offset, USSYS_TP_I2C_CHUNK_SIZE);
			offset += USSYS_TP_I2C_CHUNK_SIZE;
		} else {
			rc += ussys_tp_bd_read_oneshot(dev, reg_addr + offset, val + offset, (size - offset));
			offset = size;
		}
	}

	return rc;
}

/* [bd mode] write reg & memory with bitmask */
static int ussys_tp_bd_write_bitmask(ussys_tp_dev_t *dev, uint16_t reg_addr, uint8_t mask, uint8_t val)
{
	uint8_t orig = 0, tmp = 0;
	int rc = 0;

	rc = ussys_tp_bd_read(dev, reg_addr, &orig, 1);
	if (rc != 0)
		return rc;

	tmp = orig & (~mask);
	tmp |= val & mask;

	rc = ussys_tp_bd_write(dev, reg_addr, &tmp, 1);
	return rc;
}

/* [fd mode] write 1-byte fd reg */
static int ussys_tp_write_fd_reg(ussys_tp_dev_t *dev, uint8_t reg_addr, uint8_t *val, uint16_t size)
{
	uint8_t *buf = malloc(size + 1);
	int rc = 0, i2c_retry_cnt = 0;

	if (NULL == buf)
		return -1;

	buf[0] = reg_addr;	//reg start addr;
	memcpy(&buf[1], val, size);

	do {
		rc = dev->i2c_write(dev, buf, size + 1);
		if (rc == 0)
			break;
		i2c_retry_cnt++;
	} while (i2c_retry_cnt < USSYS_TP_I2C_MAX_RETRY_NUM);

	free(buf);

	if (i2c_retry_cnt == USSYS_TP_I2C_MAX_RETRY_NUM) {
		debug_info("dev%d(%#X) i2c write fd reg %#X failed!\r\n", dev->dev_idx, dev->i2c_addr, reg_addr);
		return -2;
	}

	return 0;
}

/* [fd mode] read one byte fd reg */
/*static*/ int ussys_tp_read_fd_reg(ussys_tp_dev_t *dev, uint8_t reg_addr, uint8_t *val, uint16_t size)
{
	int rc = 0, i2c_retry_cnt = 0;

	do {
		rc = dev->i2c_write(dev, &reg_addr, 1);
		rc += dev->i2c_read(dev, val, size);
		if (rc == 0)
			break;
		i2c_retry_cnt++;
	} while (i2c_retry_cnt < USSYS_TP_I2C_MAX_RETRY_NUM);

	if (i2c_retry_cnt == USSYS_TP_I2C_MAX_RETRY_NUM) {
		debug_info("dev%d(%#X) i2c read fd reg %#X failed!\r\n", dev->dev_idx, dev->i2c_addr, reg_addr);
		return -1;
	}

	return 0;
}

/* [fd mode] read one byte fd reg */
/*static*/ int ussys_tp_write_fd_reg_bitmask(ussys_tp_dev_t *dev, uint8_t reg_addr, uint8_t mask, uint8_t val)
{
	uint8_t orig = 0, tmp = 0;
	int rc = 0;

	rc = ussys_tp_read_fd_reg(dev, reg_addr, &orig, 1);
	if (rc != 0)
		return rc;

	tmp = orig & (~mask);
	tmp |= val & mask;

	rc = ussys_tp_write_fd_reg(dev, reg_addr, &tmp, 1);
	return rc;
}

/* [fd mode] write two bytes memory addr(e.g. dsram) */
/*static*/ int ussys_tp_write_mem(ussys_tp_dev_t *dev, uint16_t mem_addr, uint8_t *val, uint16_t size)
{
	uint8_t *buf = malloc(size + 5);
	int rc = 0, i2c_retry_cnt = 0;

	if (NULL == buf)
		return -1;

	buf[0] = TOUCH_POINT_FD_REG_ADDR_INDEX_H;
	buf[1] = (uint8_t)(mem_addr >> 8);
	buf[2] = TOUCH_POINT_FD_REG_ADDR_INDEX_L;
	buf[3] = (uint8_t)mem_addr;
	buf[4] = TOUCH_POINT_FD_REG_BURST_WRITE;
	memcpy(&buf[5], val, size);

	do {
		rc = dev->i2c_write(dev, buf, size + 5);
		if (rc == 0)
			break;
		i2c_retry_cnt++;
	} while (i2c_retry_cnt < USSYS_TP_I2C_MAX_RETRY_NUM);

	free(buf);

	if (i2c_retry_cnt == USSYS_TP_I2C_MAX_RETRY_NUM) {
		debug_info("dev%d(%#X) i2c write mem addr %#X failed!\r\n", dev->dev_idx, dev->i2c_addr, mem_addr);
		return -2;
	}

	return 0;
}

/* [fd mode] read two bytes memory addr(e.g. dsram) (oneshot) */
static int ussys_tp_read_mem_oneshot(ussys_tp_dev_t *dev, uint16_t mem_addr, uint8_t *val, uint16_t size)
{
	uint8_t buf[] = {TOUCH_POINT_FD_REG_ADDR_INDEX_H,
					(uint8_t)(mem_addr >> 8),
					TOUCH_POINT_FD_REG_ADDR_INDEX_L,
					(uint8_t)mem_addr,
					TOUCH_POINT_FD_REG_BURST_READ};
	int rc = 0, i2c_retry_cnt = 0;

	do {
		rc = dev->i2c_write(dev, buf, ARRAY_SIZE(buf));
		rc += dev->i2c_read(dev, val, size);
		if (rc == 0)
			break;
		i2c_retry_cnt++;
	} while (i2c_retry_cnt < USSYS_TP_I2C_MAX_RETRY_NUM);

	if (i2c_retry_cnt == USSYS_TP_I2C_MAX_RETRY_NUM) {
		debug_info("dev%d(%#X) i2c read mem addr %#X failed!\r\n", dev->dev_idx, dev->i2c_addr, mem_addr);
		return -1;
	}

	return 0;
}

/* [fd mode] read two bytes memory addr(e.g. dsram) */
/*static*/ int ussys_tp_read_mem(ussys_tp_dev_t *dev, uint16_t mem_addr, uint8_t *val, uint32_t size)
{
	uint32_t offset = 0;
    int rc = 0;

    while(offset <  size) {
        if( (size - offset) > USSYS_TP_I2C_CHUNK_SIZE) {
            rc += ussys_tp_read_mem_oneshot(dev, mem_addr + offset, val + offset, USSYS_TP_I2C_CHUNK_SIZE);
            offset += USSYS_TP_I2C_CHUNK_SIZE;
        } else {
            rc += ussys_tp_read_mem_oneshot(dev, mem_addr + offset, val + offset, (size - offset));
            offset = size;
        }
    }

    return rc;
}

/* [fd mode] read two bytes memory addr(e.g. dsram) with bitmask */
/*static*/ int ussys_tp_write_mem_bitmask(ussys_tp_dev_t *dev, uint16_t mem_addr, uint8_t mask, uint8_t val)
{
	uint8_t orig = 0, tmp = 0;
	int rc = 0;

	rc = ussys_tp_read_mem(dev, mem_addr, &orig, 1);
	if (rc != 0)
		return rc;

	tmp = orig & ~mask;
	tmp |= val & mask;

	rc = ussys_tp_write_mem(dev, mem_addr, &tmp, 1);
	return rc;
}

/**********************************************************
 ******************** Basic functions *********************
 **********************************************************/
/*
 * exit bd mode and enter fd mode
 */
/*static*/ int ussys_tp_exit_bd_enter_fd(ussys_tp_dev_t *dev)
{
	return ussys_tp_bd_write_bitmask(dev, TOUCH_POINT_REG_FORCE_RUNNING_RST, 0x22, 0x02);
}

/*
 * exit fd mode and enter bd mode
 */
/*static*/ int ussys_tp_exit_fd_enter_bd(ussys_tp_dev_t *dev)
{
	return ussys_tp_write_mem_bitmask(dev, TOUCH_POINT_REG_FORCE_RUNNING_RST, 0x22, 0x20);
}

static int ussys_tp_read_whoami(ussys_tp_dev_t *dev)
{
	int rc = 0;
	uint8_t whoami = 0/*, si_rev = 0*/;

	/* [bd mode] read whoami */
	/* rc = ussys_tp_bd_read(dev, TOUCH_POINT_REG_WHOAMI_16BIT_ADDR, &whoami, sizeof(whoami));
	if (TOUCH_POINT_A0A1_WHOAMI_VALUE == whoami || TOUCH_POINT_A2_WHOAMI_VALUE == whoami) {
		dev->info.whoami = whoami;
		dev->info.is_otp_part = false;
		ussys_tp_exit_bd_enter_fd(dev);
	} else {
		dev->info.is_otp_part = true;
	}
	debug_info("bd read dev%d(%#X) whoami (rc:%d ,whoami:%#X ,is_otp_part:%d)\r\n", dev->dev_idx, dev->i2c_addr, rc, whoami, dev->info.is_otp_part);*/

	/* [fd mode] read whoami */
	rc = ussys_tp_read_fd_reg(dev, TOUCH_POINT_FD_REG_WHOAMI, &whoami, sizeof(whoami));
	if (TOUCH_POINT_A0A1_WHOAMI_VALUE == whoami || TOUCH_POINT_A2_WHOAMI_VALUE == whoami) {
		dev->info.whoami = whoami;
		debug_info("fd read dev%d(%#X) whoami successfully (whoami:%#X)\r\n", dev->dev_idx, dev->i2c_addr, whoami);
	} else {
		debug_info("fd read dev%d(%#X) whoami failed! (whoami:%#X)\r\n", dev->dev_idx, dev->i2c_addr, whoami);
		return -1;
	}

	/* read silicon revision */
	/*rc = ussys_tp_read_mem(dev, TOUCH_POINT_MEM_SI_REV_NEW, &si_rev, sizeof(si_rev));
	if (0xA0 == si_rev || 0xA1 == si_rev || 0xA2 == si_rev) {
		dev->info.si_rev = si_rev;
		debug_info("read dev%d(%#X) si rev successfully (si_rev:%#X)\r\n", dev->dev_idx, dev->i2c_addr, si_rev);
	} else {
		debug_info("read dev%d(%#X) si rev failed! (si_rev:%#X)\r\n", dev->dev_idx, dev->i2c_addr, si_rev);
		//return -2;//skip this error for non-otp parts.
	}*/
	return rc;
}

/*
 * wakeup via reg to wakeup sensor.
 */
/*static*/ int ussys_tp_wakeup_via_reg(ussys_tp_dev_t *dev)
{
#if 0
	ussys_tp_info_t *info = &dev->info;
	uint32_t time_diff_us = (uint32_t)(dev->get_timestamp_us() - dev->info.last_toggle_ts_us);
	int rc = 0;

	if (info->toggled && (time_diff_us < MS_TO_US(MIN_TOGGLE_INTERVAL_MS))) {
		debug_info("reject dev%d[%#X] int reg toggle request(%d < %d ms)!\r\n",
			dev->dev_idx,
			dev->i2c_addr,
			time_diff_us / 1000,
			MIN_TOGGLE_INTERVAL_MS);
		return 0;
	}

	debug_info("dev%d(%#X) toggle via fd reg\r\n", dev->dev_idx, dev->i2c_addr);
	rc += ussys_tp_write_fd_reg_bitmask(dev, TOUCH_POINT_FD_REG_SC, TOUCH_POINT_FD_SC_HOST_INTERRUPT, TOUCH_POINT_FD_SC_HOST_INTERRUPT);
	
	mdelay(dev, 1);
	
	rc += ussys_tp_write_fd_reg_bitmask(dev, TOUCH_POINT_FD_REG_SC, TOUCH_POINT_FD_SC_HOST_INTERRUPT, 0);

	if (rc == 0) {
		info->toggled = true;
		info->last_toggle_ts_us = dev->get_timestamp_us();
	}
	return rc;
#endif

	return 0;
}

/*
 * notify algo to update param, fw will clear this bit after received it.
 */
/*static*/ int ussys_tp_notify_algo_update_param(ussys_tp_dev_t *dev)
{
	int rc = 0;
	
	rc += ussys_tp_write_fd_reg_bitmask(dev, TOUCH_POINT_FD_REG_GPR_SENSOR_CONTROL, SENSOR_CONTROL_HOST_UPDATE, SENSOR_CONTROL_HOST_UPDATE);
	mdelay(dev, 50);
	rc += ussys_tp_write_fd_reg_bitmask(dev, TOUCH_POINT_FD_REG_GPR_SENSOR_CONTROL, SENSOR_CONTROL_HOST_UPDATE, 0);
	
	return rc;
}

/* hold or release fw */
/*static*/ int ussys_tp_put_to_hold(ussys_tp_dev_t *dev, uint8_t en)
{
	int rc = 0;
	uint8_t value = en ? SENSOR_CONTROL_HOLD_MODE : 0;/* bit1: 0->no holding, 1->holding */

	rc = ussys_tp_write_fd_reg_bitmask(dev, TOUCH_POINT_FD_REG_GPR_SENSOR_CONTROL, SENSOR_CONTROL_HOLD_MODE, value);

	if (en)
		mdelay(dev, 50);

	return rc;
}

/*
 * force & hold rst
 */
static int ussys_tp_force_reset_hold(ussys_tp_dev_t *dev)
{
	int rc = 0;

	rc = ussys_tp_write_mem_bitmask(dev, TOUCH_POINT_REG_FORCE_RUNNING_RST, TOUCH_POINT_FORCE_BRAHM_RST_BIT_MASK, TOUCH_POINT_FORCE_BRAHM_RST_BIT_MASK);
	if (rc < 0) {
		debug_info("dev%d(%#X) force rst failed!\r\n", dev->dev_idx, dev->i2c_addr);
	}

	rc = ussys_tp_write_mem_bitmask(dev, TOUCH_POINT_REG_USP_ONOFF_MODE, TOUCH_POINT_RELEASE_BRAHM_RST_BIT_MASK, 0);
	if (rc < 0) {
		debug_info("dev%d(%#X) hold rst failed!\r\n", dev->dev_idx, dev->i2c_addr);
	}

	return rc;
}

/*
 * release rst 
 */
static int ussys_tp_force_reset_release(ussys_tp_dev_t *dev)
{
	int rc = 0;

	rc = ussys_tp_write_mem_bitmask(dev, TOUCH_POINT_REG_FORCE_RUNNING_RST, TOUCH_POINT_FORCE_BRAHM_RST_BIT_MASK, TOUCH_POINT_FORCE_BRAHM_RST_BIT_MASK);
	if (rc < 0) {
		debug_info("dev%d(%#X) force rst failed!\r\n", dev->dev_idx, dev->i2c_addr);
	}

	rc = ussys_tp_write_mem_bitmask(dev, TOUCH_POINT_REG_FORCE_RUNNING_RST, TOUCH_POINT_FORCE_BRAHM_RST_BIT_MASK, 0);
	if (rc < 0) {
		debug_info("dev%d(%#X) hold rst failed!\r\n", dev->dev_idx, dev->i2c_addr);
	}

	rc = ussys_tp_write_mem_bitmask(dev, TOUCH_POINT_REG_USP_ONOFF_MODE, TOUCH_POINT_RELEASE_BRAHM_RST_BIT_MASK, TOUCH_POINT_RELEASE_BRAHM_RST_BIT_MASK);
	if (rc < 0) {
		debug_info("dev%d(%#X) release rst failed!\r\n", dev->dev_idx, dev->i2c_addr);
	}

	return rc;
}

#if ENABLE_OTP_DUMP
/*
 * clk gating setting
 */
static int ussys_tp_set_clk_gating(ussys_tp_dev_t *dev)
{
	uint8_t value = 0x1F;

	return ussys_tp_write_mem(dev, TOUCH_POINT_REG_CLK_GATING, &value, sizeof(value));
}

/*
 * dump otp memory
 */
static int ussys_tp_dump_otp(ussys_tp_dev_t *dev)
{
#define TOUCH_POINT_OTP_MEM_START_ADDR		(0x1A00)
#define TOUCH_POINT_OTP_MEM_END_ADDR		(0x1AFF)//0x29FF
/* convert otp addr to otp buffer index */
#define TOUCH_POINT_OTPADDR2IDX(addr)		(addr - TOUCH_POINT_OTP_MEM_START_ADDR)
#define TOUCH_POINT_OTP_LOTID_ADDR          (0x1AEB)
#define TOUCH_POINT_OTP_DIEID_ADDR          (0x1AF7)  
    
	int rc = 0, i = 0, line = 0;
	int size = (TOUCH_POINT_OTP_MEM_END_ADDR - TOUCH_POINT_OTP_MEM_START_ADDR + 1);
	int first_line_addr = TOUCH_POINT_OTP_MEM_START_ADDR >> 4;
	int last_line_addr = TOUCH_POINT_OTP_MEM_END_ADDR >> 4;//(TOUCH_POINT_OTP_MEM_FW_START_ADDR - 1) >> 4;
	uint8_t *otp_mem = malloc(size);

	if (NULL == otp_mem)
		return 0;

	/* switch to bd mode */
	ussys_tp_exit_fd_enter_bd(dev);

	/* read otp mem */
	rc = ussys_tp_bd_read(dev, TOUCH_POINT_OTP_MEM_START_ADDR, otp_mem, size);
	if (rc < 0) {
		debug_info("read dev%d(%#X) otp failed. \r\n", dev->dev_idx, dev->i2c_addr);
	}

	/* print otp memory */
	debug_info("==========dev%d(%#X) OTP MEM START==========\r\n", dev->dev_idx, dev->i2c_addr);
	debug_info("      0x00 0x01 0x02 0x03 0x04 0x05 0x06 0x07 0x08 0x09 0x0a 0x0b 0x0c 0x0d 0x0e 0x0f\r\n");
	for (line = last_line_addr; line >= first_line_addr; line--) {
		int cnt = 0;
		char *buf = malloc(100);
		if (NULL == buf)
			break;

		cnt += sprintf(buf + cnt, "0x%03x:", line);
		for (i = 0; i < 16; i++)
		{
			int otp_addr = (line << 4) + i;
			if (otp_addr >= TOUCH_POINT_OTP_MEM_START_ADDR && otp_addr <= TOUCH_POINT_OTP_MEM_END_ADDR)
				cnt += sprintf(buf + cnt, "0x%02x ", otp_mem[TOUCH_POINT_OTPADDR2IDX(otp_addr)]);
		}
		cnt += sprintf(buf + cnt, "\r\n");
		debug_info("%s", buf);
		free(buf);
	}
    
    memcpy( &dev->info.chipID[0], &otp_mem[TOUCH_POINT_OTPADDR2IDX(TOUCH_POINT_OTP_LOTID_ADDR)], 2);
    memcpy( &dev->info.chipID[2], &otp_mem[TOUCH_POINT_OTPADDR2IDX(TOUCH_POINT_OTP_DIEID_ADDR)], 5);
	debug_info("==========dev%d(%#X) OTP MEM END============\r\n", dev->dev_idx, dev->i2c_addr);

	free(otp_mem);

	/* switch back to fd mode */
	ussys_tp_exit_bd_enter_fd(dev);
	return 0;
}
#endif

#if ENABLE_INIT_REG
/*
 * calibrate wake up timer
 */
static int ussys_tp_cal_wut(ussys_tp_dev_t *dev) {
	uint8_t reg_18a6, reg_18dc, reg_18dd, reg_1894;
	uint8_t reg_1894_initial = 0x40;
	uint8_t val = 0;
	uint16_t count_current = 0;
	uint16_t count_target = 0;
	uint16_t tmp;
	int rc = 0;

    //0:32KHz(ZForce:4KHz)
	reg_1894 = 0x3A;//0x3F;(84KHz)//0x3A;(32KHz)
	count_target = 625;//242;(84KHz)//625;(32KHz)
	rc += ussys_tp_write_mem(dev, 0x18A6, &reg_1894_initial, 1);/* make sure we're at the expected starting conditions */
	rc += ussys_tp_write_mem(dev, 0x1894, &reg_1894, 1);

	/* log initial values for debug purposes */
	rc += ussys_tp_read_mem(dev, 0x18A6, &reg_18a6, 1);
	rc += ussys_tp_read_mem(dev, 0x18DC, &reg_18dc, 1);
	rc += ussys_tp_read_mem(dev, 0x18DD, &reg_18dd, 1);

	/* start counting number of 20MHz pulses in one 32kHz clock pulse */
	val = reg_18dd & 0xF7;
	rc += ussys_tp_write_mem(dev, 0x18DD, &val, 1);
	val = reg_18dd | 0x08;
	rc += ussys_tp_write_mem(dev, 0x18DD, &val, 1);

	/* give 400us to finish counting (playing it safe) */
	mdelay(dev, 1);//udelay(dev, 400);

	/* read final count */
	rc += ussys_tp_read_mem(dev, 0x18DC, &reg_18dc, 1);
	rc += ussys_tp_read_mem(dev, 0x18DD, &reg_18dd, 1);
	count_current = (reg_18dd & 0x7) << 8 | reg_18dc;

	/* calculate the required register setting for the target frequency */
	tmp = reg_1894_initial * count_current / count_target;
	reg_18a6 = (uint8_t)(tmp);

	/* write calculated value and do another count */
	reg_18dd &= 0xF8;/* zero out the bottom three bits */
	rc += ussys_tp_write_mem(dev, 0x18A6, &reg_18a6, 1);

	debug_info("cali wut done.(rc = %d, 0x18A6=%#X)\r\n", rc, reg_18a6);
	return rc;
}

/*
 * init hw RTL regs
 */
static int ussys_tp_init_regs(ussys_tp_dev_t *dev)
{
	uint8_t acoustic_regs[]				= {0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x89,0x88,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,0x90,0x91,0x92,0x93};
    uint8_t acoustic_regs_a0a2_val[]	= {0x05,0x06,0x5F,0x00,0x03,0xFF,0x00,0x00,0x01,0x05,0x00,0xAC,0x07,0xD4,0x19,0x00,0x3A,0x0A,0x50,0x05};
    uint8_t acoustic_regs_a1_val[]		= {0x09,0x06,0x5A,0x00,0x03,0xFF,0x00,0x00,0x01,0x05,0x00,0xAC,0x07,0xA8,0x19,0x00,0x33,0x03,0x40,0x05};

    uint8_t cmos_regs[]					= {0x35,0x36,0x48,0x49,0xA1,0xA6,/*0xA7,*/0xAA,0xAC,0xAD};
    uint8_t cmos_regs_val[]				= {0x00,0x00,0x15,0x7C,0x00,0x44,/*0x7F,*/0x3E,0x0D,0x28};

    uint8_t misc_regs[]					= {0xAF, 0x94, 0xFB, 0xDD, 0xDB, 0xE9, 0xF6, 0xF8, 0xBB, 0xB8};//if need enable i2c clk-gating, need set reg 0x18FB=0x10, reg 0x18DD=0x30
    uint8_t misc_regs_val[]				= {0x00, 0x3A, 0x1F, 0x10, 0xFF, 0x50, 0x85, 0x01, 0x01, 0xC7};

    uint8_t zforce_regs[]         		= {0x30,0x31,0x32,0x33,0x34,/*0x35,0x36,*/0x37,0x38,0x39,0x3A,0x3B,0x3C,0x3D,0x3E,0x3F,0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,/*0x48,0x49,*/0x4A,0x4B,0x4C,0x4D,0x4E,0x4F};
    uint8_t zforce_regs_val[]     		= {0x50,0x00,0x00,0x00,0x96,/*0x00,0x00,*/0x1F,0x0F,0x3E,0x00,0x00,0x00,0x00,0x00,0x00,0x3F,0xF0,0xFF,0x00,0x55,0x51,0x0F,0xD0,/*0x15,0x5F,*/0x00,0x15,0xF8,0x10,0x20,0x22};

    int rc = 0;
    uint8_t i = 0;
    uint8_t tx_freq = 0;

    rc = ussys_tp_read_mem(dev, TOUCH_POINT_REG_TX_FREQ, &tx_freq, sizeof(tx_freq));
    if (rc < 0) {
        debug_info("dev%d(%#X) read tx freq failed!(rc:%d)\r\n", dev->dev_idx, dev->i2c_addr, rc);
        return rc;
    }
    debug_info("dev%d(%#X) read tx freq:%#X\r\n", dev->dev_idx, dev->i2c_addr, tx_freq);

	/* if acoustic params are blank at factory, write default params */
	if (tx_freq == 0xFF) {
		uint8_t *regs_val = (dev->info.si_rev == 0xA1) ? acoustic_regs_a1_val : acoustic_regs_a0a2_val;
		for (i = 0; i < sizeof(acoustic_regs); i++) {
			rc = ussys_tp_write_mem(dev, 0x1800 + acoustic_regs[i], regs_val + i, 1);
			if (rc < 0) {
				debug_info("dev%d(%#X) write acoustic regs failed!(rc:%d)\r\n", dev->dev_idx, dev->i2c_addr, rc);
                return rc;
            }
        }
        debug_info("dev%d(%#X) write acoustic regs ok!\r\n", dev->dev_idx, dev->i2c_addr);
    }

	if (false == dev->info.is_otp_part) {
		for (i = 0; i < sizeof(cmos_regs); i++) {
	        rc = ussys_tp_write_mem(dev, 0x1800 + cmos_regs[i], cmos_regs_val + i, 1);
	        if (rc < 0) {
	            debug_info("dev%d(%#X) write cmos regs failed!(rc:%d)\r\n", dev->dev_idx, dev->i2c_addr, rc);
	            return rc;
	        }
	    }
	    debug_info("dev%d(%#X) write cmos regs ok!\r\n", dev->dev_idx, dev->i2c_addr);
	}

    for (i = 0; i < sizeof(misc_regs); i++) {
        rc = ussys_tp_write_mem(dev, 0x1800 + misc_regs[i], misc_regs_val + i, 1);
        if (rc < 0) {
            debug_info("dev%d(%#X) write misc regs failed!(rc:%d)\r\n", dev->dev_idx, dev->i2c_addr, rc);
            return rc;
        }
    }
    debug_info("dev%d(%#X) write misc regs ok!\r\n", dev->dev_idx, dev->i2c_addr);

	for (i = 0; i < sizeof(zforce_regs); i++) {
		rc = ussys_tp_write_mem(dev, 0x1800 + zforce_regs[i], zforce_regs_val + i, 1);
		if (rc < 0) {
			debug_info("dev%d(%#X) write zforce regs failed!(rc:%d)\r\n", dev->dev_idx, dev->i2c_addr, rc);
            return rc;
        }
    }
    debug_info("dev%d(%#X) write zforce regs ok!\r\n", dev->dev_idx, dev->i2c_addr);

	/* adjust zforce gain */
	{
		uint8_t zforce_gain = 0x55;

		switch (dev->dev_idx) {
		case 0:
			zforce_gain = 0x55;
			break;
		case 1:
			zforce_gain = 0x55;
			break;
		default:
			zforce_gain = 0x55;
			break;
		}
		rc = ussys_tp_write_mem(dev, 0x1844, &zforce_gain, 1);
		debug_info("dev%d(%#X) write zforce gain(0x1844=%#X, rc:%d)\r\n", dev->dev_idx, dev->i2c_addr, zforce_gain, rc);
	}

	/* reverse zforce signal polarity(-(Z0+Z2))  */
	{
		uint8_t zforce_polarity = 0x96;

		switch (dev->dev_idx) {
		case 0:
			#if ENABLE_FLIP_ZFORCE_FE_SIGNAL
			zforce_polarity = 0x69;//-(Z0+Z2)
			#else
			zforce_polarity = 0x96;//(Z0+Z2)
			#endif
			break;
		case 1:
			zforce_polarity = 0x96;//(Z0+Z2)
			break;
		default:
			zforce_polarity = 0x96;
			break;
		}
		rc = ussys_tp_write_mem(dev, 0x1834, &zforce_polarity, 1);
		debug_info("dev%d(%#X) write zforce signal polarity(0x1834=%#X, rc:%d)\r\n", dev->dev_idx, dev->i2c_addr, zforce_polarity, rc);
	}

	if (false == dev->info.is_otp_part) {
		ussys_tp_cal_wut(dev);
	}
    return rc;
}
#endif

/*
 * read algo1st ths from chip
 */
static int ussys_tp_get_algo1st_ths(ussys_tp_dev_t *dev, ussys_algo1st_ths_t *algo1st_ths)
{
	int rc = 0;
	
	rc += ussys_tp_read_mem(dev, SWREG_THS_REVC, (uint8_t *)algo1st_ths, sizeof(ussys_algo1st_ths_t));
	
	return rc;
}

/*
 * apply algo1st ths into chip
 */
static int ussys_tp_push_algo1st_ths(ussys_tp_dev_t *dev, ussys_algo1st_ths_t *algo1st_ths)
{
	int rc = 0;
	
	rc += ussys_tp_write_mem(dev, SWREG_THS_REVC, (uint8_t *)algo1st_ths, sizeof(ussys_algo1st_ths_t));
    
	return rc;
}

/*
 * apply sensor parameters into chip
 */
/*static*/ int ussys_tp_apply_cal_param(ussys_tp_dev_t *dev)
{
	int rc = 0;

	if (NULL != dev->load_cal_param) {
		dev->load_cal_param(dev);
	}

	if (dev->cal_param.is_valid == 0 || dev->cal_param.is_valid == 0xFF)
		return 0;

    rc += ussys_tp_put_to_hold(dev, true);
    
	/* read default algo1st ths */
	rc += ussys_tp_get_algo1st_ths(dev, &algo1st_ths);
	
	#if USSYS_CAP_ENABLED
	if (dev->cal_param.cap_param_is_valid == true) {
		/* apply cap leak cnt */
		algo1st_ths.cap.s.leak_count = dev->cal_param.cap_leak_cnt;
		debug_info("dev%d(%#X) set cap leak count 0x%02x\r\n", dev->dev_idx, dev->i2c_addr, dev->cal_param.cap_leak_cnt);
		/* apply cap offset delta */
		algo1st_ths.cap.s.offset_delta = dev->cal_param.cap_offset_delta;
		debug_info("dev%d(%#X) set cap offset delta 0x%02x\r\n", dev->dev_idx, dev->i2c_addr, dev->cal_param.cap_offset_delta);
	}
	#endif
	
	#if USSYS_CASP_ENABLED
	/* apply zforce offset & gain */
	if (dev->cal_param.zforce_offset_0x1836 != 0 || dev->cal_param.zforce_offset_0x1835 != 0) {
		rc += ussys_tp_write_mem(dev, USSYS_REG_ZFORCE_SINC_OFFSET_ADDR, &dev->cal_param.zforce_offset_0x1836, 1);	
		debug_info("dev%d(%#X) set zforce offset 0x%4x=0x%02X\r\n", dev->dev_idx, dev->i2c_addr, USSYS_REG_ZFORCE_SINC_OFFSET_ADDR, dev->cal_param.zforce_offset_0x1836);
		rc += ussys_tp_write_mem(dev, USSYS_REG_ZFORCE_AMP2_OFFSET_ADDR, &dev->cal_param.zforce_offset_0x1835, 1);	
		debug_info("dev%d(%#X) set zforce offset 0x%4x=0x%02X\r\n", dev->dev_idx, dev->i2c_addr, USSYS_REG_ZFORCE_AMP2_OFFSET_ADDR, dev->cal_param.zforce_offset_0x1835);
	}
	rc += ussys_tp_write_mem(dev, USSYS_REG_ZFORCE_GAIN_ADDR, &dev->cal_param.zforce_gain_0x1844, 1);	
	debug_info("dev%d(%#X) set zforce gain 0x%4x=0X%02X\r\n", dev->dev_idx, dev->i2c_addr, USSYS_REG_ZFORCE_GAIN_ADDR, dev->cal_param.zforce_gain_0x1844);

	/* apply zforce algo param */
	algo1st_ths.casp.s.SumPlusPeakValueAtStdForcePtn = dev->cal_param.algo_zforce_contrast;
	debug_info("dev%d(%#X) set zforce contrast %d\r\n", dev->dev_idx, dev->i2c_addr, dev->cal_param.algo_zforce_contrast);
	algo1st_ths.casp.s.SumNoiseRMS = dev->cal_param.algo_zforce_noise;
	debug_info("dev%d(%#X) set zforce noise %d\r\n", dev->dev_idx, dev->i2c_addr, dev->cal_param.algo_zforce_noise);
	#endif
	
	#if USSYS_USP_ENABLED
	uint8_t reg_0x1889_inital_value = 2;
	rc += ussys_tp_write_mem(dev, 0x1889, &reg_0x1889_inital_value, 1);
	/* apply usp 20 bytes params */
	rc += ussys_tp_write_mem(dev, USSYS_REG_USP_BANK0_START_ADDR, dev->cal_param.reg_accoustic, USSYS_REG_USP_BANK0_SIZE);
	for (int i = 0; i < USSYS_REG_USP_BANK0_SIZE; i++) {
		debug_info("dev%d(%#X) set 0x%4x=0x%02x\r\n", dev->dev_idx, dev->i2c_addr, USSYS_REG_USP_BANK0_START_ADDR + i, dev->cal_param.reg_accoustic[i]);
	}
	/* apply usp max contrast */
	algo1st_ths.usp.s.MaxContrast = dev->cal_param.algo_usp_max_contrast;	
	debug_info("dev%d(%#X) set usp max contrast 0x%4x=0x%02x\r\n", dev->dev_idx, dev->i2c_addr, algo1st_ths.usp.s.MaxContrast, dev->cal_param.algo_usp_max_contrast);
	/* apply usp noise for algo */
	algo1st_ths.usp.s.NoiseRMS = dev->cal_param.algo_usp_noise_rms;
	debug_info("dev%d(%#X) set usp noise rms 0x%4x=0x%02x\r\n", dev->dev_idx, dev->i2c_addr, algo1st_ths.usp.s.NoiseRMS, dev->cal_param.algo_usp_noise_rms);
	#endif
	
	/* apply algo1st ths */
	rc += ussys_tp_push_algo1st_ths(dev, &algo1st_ths);
    
    rc += ussys_tp_put_to_hold(dev, false);
    
    /* notify fw to update algo param */
	rc += ussys_tp_notify_algo_update_param(dev);
    
	return rc;
}

/*
 * get fw version
 */
static uint8_t ussys_tp_get_fw_version(ussys_tp_dev_t *dev)
{
	uint8_t rc = 0;
	
	rc += ussys_tp_read_mem(dev, TOUCH_POINT_MEM_FW_VERSIONMain, &dev->info.fw_ver[0], 1);
    rc += ussys_tp_read_mem(dev, TOUCH_POINT_MEM_FW_VERSIONMinus, &dev->info.fw_ver[1], 1);
	debug_info("dev%d(%#X) fw version: v%d.%d.%d\r\n", dev->dev_idx, dev->i2c_addr,\
				dev->info.fw_ver[0] >> 4, (dev->info.fw_ver[0] & 0x0F), dev->info.fw_ver[1]);
    
	return rc;
}

/*
 * get supported sensors
 */
static uint8_t ussys_tp_get_supported_sensors(ussys_tp_dev_t *dev)
{
	uint8_t rc = 0;
	
	rc += ussys_tp_read_mem(dev, SWREG_SupportedSensors, &dev->info.supported_sensors, 1);
	debug_info("dev%d(%#X) supported sensors: %d, usp:%d, cap:%d, casp:%d,\r\n", dev->dev_idx, dev->i2c_addr, dev->info.supported_sensors,\
	(bool)(dev->info.supported_sensors & MASK_SENSOR_USP), (bool)(dev->info.supported_sensors & MASK_SENSOR_CAP), (bool)(dev->info.supported_sensors & MASK_SENSOR_CASP));
    
	return rc;
}

#if USSYS_CASP_ENABLED
/*
 * get polarity
 */
/*static*/ int ussys_tp_get_polarity(ussys_tp_dev_t *dev, uint8_t *en)
{
	int rc = 0;

	/* get ZForce peak height from ZForce Cal via USZ FW */
	rc += ussys_tp_get_algo1st_ths(dev, &algo1st_ths);
	*en = algo1st_ths.casp.s.ReversedPolarity;
	debug_info("dev%d(%#X) get Polairty %d\r\n", dev->dev_idx, dev->i2c_addr, *en);
    
    return rc;
}

/*
 * set polarity
 */
/*static*/ int ussys_tp_set_polairty(ussys_tp_dev_t *dev, uint8_t en)
{
	int rc = 0;
    
	/* get ZForce peak height from ZForce Cal via USZ FW */
	rc += ussys_tp_get_algo1st_ths(dev, &algo1st_ths);
	algo1st_ths.casp.s.ReversedPolarity = en;
	/* Set polairty  */
	rc += ussys_tp_push_algo1st_ths(dev, &algo1st_ths);
	debug_info("dev%d(%#X) Set Polairty %d\r\n", dev->dev_idx, dev->i2c_addr, en);

    return rc;
}

/*
 * get zforce offset for calibration
 */
static uint16_t ussys_tp_get_zforce_offset(ussys_tp_dev_t *dev)
{
	uint16_t zforce_offset = 0;
	int i = 0;

	for (i = 0; i < 16; i++) {
		uint8_t zforce_adc = 0;

		ussys_tp_read_fd_reg(dev, TOUCH_POINT_FD_REG_ZFORCE_ADC, &zforce_adc, sizeof(zforce_adc));
		zforce_offset += ((uint16_t)zforce_adc << 3);
		mdelay(dev, 5);
	}
	zforce_offset /= 16;
	debug_info("dev%d(%#X) zforce_offset %d\r\n", dev->dev_idx, dev->i2c_addr, zforce_offset);
	return zforce_offset;
}


/*
 * calibrate zforce offset using binary-search method
 */
/*static*/ int ussys_tp_cali_zforce_offset(ussys_tp_dev_t *dev)
{
	uint16_t max_offset = 1024 + 16;
	uint16_t min_offset = 1024 - 16;
    uint16_t zforce_offset = 0;
	uint8_t offset_val = 0, offset_cali_stage = 0;
    uint8_t trimming_done = 0;

  /* let downstream know that we're trimming so tht it doesn't signal
   * a press while the offset is changing.
   */
	ussys_tp_write_fd_reg_bitmask(dev, TOUCH_POINT_FD_REG_GPR_SENSOR_CONTROL, SENSOR_CONTROL_Z_TRIMMING, SENSOR_CONTROL_Z_TRIMMING);
    mdelay(dev, 25);

	/* initialize both reg 0x1835 & 0x1836 to 0x0 */
//	ussys_tp_write_mem(dev, USSYS_REG_ZFORCE_AMP2_OFFSET_ADDR, &offset_val, sizeof(offset_val));
//	ussys_tp_write_mem(dev, USSYS_REG_ZFORCE_SINC_OFFSET_ADDR, &offset_val, sizeof(offset_val));
    ussys_tp_write_fd_reg(dev, TOUCH_POINT_FD_REG_GPR0, &offset_val, sizeof(offset_val));
    ussys_tp_write_fd_reg(dev, TOUCH_POINT_FD_REG_GPR1, &offset_val, sizeof(offset_val));

	/* 1st stage: cali SINC offset 0x1836, 2nd stage: cali AMP2 offset 0x1835 */
	for (offset_cali_stage = 0; (offset_cali_stage < 2) && (!trimming_done); offset_cali_stage++) {
		uint8_t first_round = true;

		uint8_t offset_ref = 0;
		uint8_t offset_up = 0xF;
		uint8_t offset_down = 0x0;
		uint8_t offset_middle = 0x8;
		
		debug_info("dev%d(%#X) cali stage %d\r\n", dev->dev_idx, dev->i2c_addr, offset_cali_stage);
		zforce_offset = ussys_tp_get_zforce_offset(dev);
		
		while(1) {
			/* first round determines whether to add positive or negative offset */
			if (first_round) {
				if (zforce_offset > max_offset) {
					/* zforce offset is too high, need to reduce the offset */
					if (offset_cali_stage == 0)
						offset_ref = 0x02;
					else if (offset_cali_stage == 1)
						offset_ref = 0x01;
				}
				else if (zforce_offset < min_offset) {
					/* zforce offset is too low, need to increase the offset */
					if (offset_cali_stage == 0)
						offset_ref = 0x0A;
					else if (offset_cali_stage == 1)
						offset_ref = 0x09;
				}
				else {
                    trimming_done = 1;
                    debug_info("dev%d(%#X) offset %d is in the range, exit \r\n", dev->dev_idx, dev->i2c_addr, zforce_offset);
					/* zforce offset is already within the tolerance, no need to calibrate it */
					break;
                }
			} else {
				if ((offset_up - offset_down) <= 2) {
					debug_info("dev%d(%#X) offset_middle %d ,offset_up %d ,offset_down %d\r\n", dev->dev_idx, dev->i2c_addr, offset_middle, offset_up, offset_down);
					break;
				} else if (((offset_ref < 0x05) && (zforce_offset >= max_offset)) ||
						((offset_ref > 0x05) && (zforce_offset <= min_offset))) {
					/* need to increase offset compensation */
					offset_down = offset_middle;
					offset_middle = (offset_up + offset_down) / 2;
				} else if (((offset_ref < 0x05) && (zforce_offset < max_offset)) ||
						((offset_ref > 0x05) && (zforce_offset > min_offset))) {
					/* need to reduce offset compensation */
					offset_up = offset_middle;
					offset_middle = (offset_up + offset_down) / 2;
				}
			}

			/* update the offset compensation register */
			offset_val = (offset_middle << 4) | offset_ref;
			if (offset_cali_stage == 0) {
				//ussys_tp_write_mem(dev, USSYS_REG_ZFORCE_SINC_OFFSET_ADDR, &offset_val, sizeof(offset_val));
                ussys_tp_write_fd_reg(dev, TOUCH_POINT_FD_REG_GPR1, &offset_val, sizeof(offset_val));
                dev->cal_param.zforce_offset_0x1836 = offset_val;
            }
			else if (offset_cali_stage == 1) {
				//ussys_tp_write_mem(dev, USSYS_REG_ZFORCE_AMP2_OFFSET_ADDR, &offset_val, sizeof(offset_val));
                ussys_tp_write_fd_reg(dev, TOUCH_POINT_FD_REG_GPR0, &offset_val, sizeof(offset_val));
                dev->cal_param.zforce_offset_0x1835 = offset_val;
            }
	
			if (first_round)
				first_round = false;

			mdelay(dev, 20);
			zforce_offset = ussys_tp_get_zforce_offset(dev);
		}
	}

    debug_info("dev%d(%#X) 0x1836 0X%2X 0x1835 0X%2X offset %d \r\n", dev->dev_idx, dev->i2c_addr, dev->cal_param.zforce_offset_0x1836, dev->cal_param.zforce_offset_0x1835, zforce_offset);
    
	ussys_tp_write_fd_reg_bitmask(dev, TOUCH_POINT_FD_REG_GPR_SENSOR_CONTROL, SENSOR_CONTROL_Z_TRIMMING, 0);

	return 0;
}

/* private define for python calibration */
#define ZForce_INTERVAL				(5) //5ms
#define ZForce_TRIMPATTERN_DUR		(333+50)
#define ZForce_CYCNUM_INBUF			(2)
#define ZForce_P2P_OBS_MINIWIN		(ZForce_TRIMPATTERN_DUR/ZForce_INTERVAL)
#define ZForce_ADCBUF_LEN			(ZForce_P2P_OBS_MINIWIN*ZForce_CYCNUM_INBUF)
#define ZForce_ADC_MAX_Noise		(10)
/*
 * get zforce noise
 */
static float zforce_cal_calculate_get_rms_noise(uint8_t* ZForceADC)
{
    float avg = 0;
    for(int i=0; i<ZForce_ADCBUF_LEN; i++)
        avg += ZForceADC[i];
    avg /= ZForce_ADCBUF_LEN;

    float variance = 0 ;
    for(int i=0; i<ZForce_ADCBUF_LEN; i++)
        variance += (avg - ZForceADC[i])*(avg - ZForceADC[i]);

    float stdev = sqrtf(variance);
	
    return stdev;
}

/*
 * python calibrate zforce noise measure
 */
/*static*/ussys_cal_param_t zforce_python_cal_step2_measure_noise(ussys_tp_dev_t *dev, uint8_t gain, uint8_t contrast, uint8_t *threshold)
{
    int rc = 0;
    uint8_t ZForceValue;
    float rms = 0;
    int ZForceADCOffset = 0;
    uint8_t ZForceADC[ZForce_ADCBUF_LEN];
    ussys_algo1st_ths_t algo1st_ths;
    ussys_cal_param_t cal_param;
    
    debug_info("==========dev%d(%#X) NOISE MEASURE START==========\r\n", dev->dev_idx, dev->i2c_addr);
    
    debug_info("dev%d(%#X) apply param before performing noise measure...\r\n", dev->dev_idx, dev->i2c_addr);
    
    /* apply param to sensor */
	memset(&cal_param, 0, sizeof(ussys_cal_param_t));
    
    rc += ussys_tp_put_to_hold(dev, true);
    
    cal_param.zforce_gain_0x1844 = gain;
    rc += ussys_tp_write_mem(dev, USSYS_REG_ZFORCE_GAIN_ADDR, &gain, 1);
    debug_info("dev%d(%#X) set zforce gain 0X%02X\r\n", dev->dev_idx, dev->i2c_addr, gain);
    
	rc += ussys_tp_get_algo1st_ths(dev, &algo1st_ths);
    cal_param.algo_zforce_contrast = contrast;
	algo1st_ths.casp.s.SumPlusPeakValueAtStdForcePtn = contrast;
    debug_info("dev%d(%#X) set zforce plus peak %d\r\n", dev->dev_idx, dev->i2c_addr, contrast);
    rc += ussys_tp_push_algo1st_ths(dev, &algo1st_ths);
    
    rc += ussys_tp_put_to_hold(dev, false);
    
    debug_info("dev%d(%#X) performing noise measure...\r\n", dev->dev_idx, dev->i2c_addr);
    
    for (uint8_t i=0; i<ZForce_ADCBUF_LEN; i++)
		ZForceADC[i] = 0;
    
    //trimming offset
    ussys_tp_cali_zforce_offset(dev);
    
    mdelay(dev, 500);
    
    uint8_t zforce_offset = ussys_tp_get_zforce_offset(dev) / 8;
    
	while (1) {
		rc += ussys_tp_if_get_zforce_adc(dev);
		if (rc < 0) {
            cal_param.is_valid = 0;
            debug_info("dev%d(%#X) iic communication error, abort measureing noise, exit...\r\n", dev->dev_idx, dev->i2c_addr);
			return cal_param;
		}
		ZForceValue	= dev->info.zforce_adc;
		ZForceADC[ZForceADCOffset % ZForce_ADCBUF_LEN] = ZForceValue;
		ZForceADCOffset ++;
        //debug_info("dev%d(%#X) zforce calculated adc %d\r\n", dev->dev_idx, dev->i2c_addr, ZForceValue);
		
		if(ZForceADCOffset >= ZForce_ADCBUF_LEN) {
			rms = zforce_cal_calculate_get_rms_noise(ZForceADC);
            debug_info("dev%d(%#X) zforce rms %d\r\n", dev->dev_idx, dev->i2c_addr, (int)rms);
			break;
		}
		
		mdelay(dev, ZForce_INTERVAL);
	}

	uint8_t noise_rms = (uint8_t)(rms + 0.5f);
    cal_param.algo_zforce_noise = noise_rms;
    algo1st_ths.casp.s.SumNoiseRMS = noise_rms;
    
    rc += ussys_tp_put_to_hold(dev, true);
    
    //read offset reg from sensor
    rc += ussys_tp_read_mem(dev, USSYS_REG_ZFORCE_SINC_OFFSET_ADDR, &cal_param.zforce_offset_0x1836, 1);
    rc += ussys_tp_read_mem(dev, USSYS_REG_ZFORCE_AMP2_OFFSET_ADDR, &cal_param.zforce_offset_0x1835, 1);
    debug_info("dev%d(%#X) zforce offset %d sinc 0X%02X amp2 0X%02X\r\n", dev->dev_idx, dev->i2c_addr, zforce_offset, cal_param.zforce_offset_0x1836, cal_param.zforce_offset_0x1835);
    
	rc += ussys_tp_push_algo1st_ths(dev, &algo1st_ths);
    debug_info("dev%d(%#X) zforce noise %d\r\n", dev->dev_idx, dev->i2c_addr, (int)noise_rms);
    
    rc += ussys_tp_put_to_hold(dev, false);
    
    if (cal_param.algo_zforce_noise <= ZForce_ADC_MAX_Noise) {
        memcpy(&dev->cal_param, &cal_param, sizeof(ussys_cal_param_t));
        cal_param.is_valid = 1;
        debug_info("dev%d(%#X) zforce noise check pass\r\n", dev->dev_idx, dev->i2c_addr);
    }
    else {
        cal_param.is_valid = 0;
        debug_info("dev%d(%#X) zforce noise is too much, noise check failed\r\n", dev->dev_idx, dev->i2c_addr);
    }
    
    debug_info("==========dev%d(%#X) NOISE MEASURE END==========\r\n", dev->dev_idx, dev->i2c_addr);
    
    return cal_param;
}
#endif

/*
 * download fw binary into psram
 */
/*static*/ int ussys_tp_download_fw(ussys_tp_dev_t *dev, uint8_t *fw_buf, uint32_t fw_size)
{
    int rc = 0;
    uint32_t offset = 0;

	debug_info("dev%d(%#X) fw dl start...(fw size:%ld bytes)\r\n", dev->dev_idx, dev->i2c_addr, fw_size);
    while (offset < fw_size) {
        if ((fw_size - offset) > USSYS_TP_I2C_CHUNK_SIZE) {
            rc = ussys_tp_write_mem(dev, 0 + offset, fw_buf + offset, USSYS_TP_I2C_CHUNK_SIZE);
            if (rc < 0) {
                debug_info("dev%d(%#X) download fw failed!(offset:%ld, rc:%d)\r\n", dev->dev_idx, dev->i2c_addr, offset, rc);
                return rc;
            }
            offset += USSYS_TP_I2C_CHUNK_SIZE;
        } else {
            rc = ussys_tp_write_mem(dev, 0 + offset, fw_buf + offset, (fw_size - offset));
            if (rc < 0) {
                debug_info("dev%d(%#X) download fw failed!(offset:%ld, rc:%d)\r\n", dev->dev_idx, dev->i2c_addr, offset, rc);
                return rc;
            }
            offset = fw_size;
        }
    }

	debug_info("dev%d(%#X) fw dl completed\r\n", dev->dev_idx, dev->i2c_addr);
    return rc;
}

/*
 * download default fw binary into psram
 */
#define READOUTFWCHUNKSIZE 128
uint8_t readoutFWConfirmBuf[READOUTFWCHUNKSIZE];
void BrahmsBackDoor_ReadBytes(ussys_tp_dev_t *dev, uint16_t regOff16, uint8_t *pData, uint16_t Size)
{
    uint16_t offset = 0; 
    while(offset <  Size)
    {
        if( (Size-offset) > USSYS_TP_I2C_CHUNK_SIZE){
            ussys_tp_read_mem(dev, regOff16+offset, pData+offset, USSYS_TP_I2C_CHUNK_SIZE);
            offset+=USSYS_TP_I2C_CHUNK_SIZE;
        }
        else{
            ussys_tp_read_mem(dev, regOff16+offset, pData+offset, (Size-offset));
            offset = Size;
        }
    }
}
static int ussys_tp_download_default_fw(ussys_tp_dev_t *dev)
{
    int rc = 0;
    
    rc += ussys_tp_download_fw(dev, (uint8_t *)BrahmsFW_C, ARRAY_SIZE(BrahmsFW_C));

	int offset = 0; 
    while(offset<ARRAY_SIZE(BrahmsFW_C))
    {
        int lengthToRead; 
        if((ARRAY_SIZE(BrahmsFW_C)-offset)>=READOUTFWCHUNKSIZE)  
            lengthToRead = READOUTFWCHUNKSIZE; 
        else                            
            lengthToRead = (ARRAY_SIZE(BrahmsFW_C)-offset);
        BrahmsBackDoor_ReadBytes(dev, 0+offset,readoutFWConfirmBuf,lengthToRead);
        for(int i =0; i<lengthToRead;i++){
            if(BrahmsFW_C[offset+i] != readoutFWConfirmBuf[i]){
                debug_info("BrahmsBackDoor_FlushFW Error! FW readout not identical! Byte located @%d should be 0x%x, but it's 0x%x", offset+i,BrahmsFW_C[offset+i],readoutFWConfirmBuf[i]);
                return 1;
            }
        }
        offset += lengthToRead;
    }
    
    return rc;
}

#if ENABLE_REG_DUMP
/*
 * dump RTL registers
 */
static int ussys_tp_dump_regs(ussys_tp_dev_t *dev)
{
#define TOUCH_POINT_REG_START_ADDR		(0x1830)
#define TOUCH_POINT_REG_END_ADDR		(0x18FF)
#define TOUCH_POINT_REG_DUMP_SIZE		(TOUCH_POINT_REG_END_ADDR - TOUCH_POINT_REG_START_ADDR + 1)

	int i = 0, rc = 0;
	uint8_t *buf = malloc(TOUCH_POINT_REG_DUMP_SIZE);

	if (NULL == buf)
		return 0;

    rc = ussys_tp_read_mem(dev, TOUCH_POINT_REG_START_ADDR, buf, TOUCH_POINT_REG_DUMP_SIZE);
    if (rc < 0) {
		free(buf);
        debug_info("dev%d(%#X) dump registers failed!(rc:%d)\r\n", dev->dev_idx, dev->i2c_addr, rc);
        return rc;
    }

	debug_info("==========dev%d(%#X) DUMP REGISTER START==========\r\n", dev->dev_idx, dev->i2c_addr);
	for (i = 0; i < TOUCH_POINT_REG_DUMP_SIZE; i++) {
		debug_info("dev%d(%#X) 0x%04x = 0x%02x \r\n",
				dev->dev_idx,
				dev->i2c_addr,
				TOUCH_POINT_REG_START_ADDR + i,
				buf[i]);
	}
	debug_info("==========dev%d(%#X) DUMP REGISTER END==========\r\n", dev->dev_idx, dev->i2c_addr);

	free(buf);
	return rc;
}
#endif

/**********************************************************
 ******************** Driver interface functions **********
 **********************************************************/
/*
 * initialize ultrasense device
 */
int ussys_tp_if_init(ussys_tp_dev_t *dev)
{
	int rc = 0;
	
	if (NULL == dev || NULL == dev->i2c_read || NULL == dev->i2c_write
		|| NULL == dev->get_timestamp_us || NULL == dev->load_cal_param 
		|| NULL ==dev->store_cal_param) {
		debug_info("bad param!\r\n");

		return -1;
	}
	
	/* clear power down bit */
	ussys_tp_write_fd_reg_bitmask(dev, TOUCH_POINT_FD_REG_GPR_SENSOR_CONTROL, SENSOR_CONTROL_POWER_DOWN_ENABLE, 0);
    
	rc += ussys_tp_read_whoami(dev);
	if (rc < 0) {
		dev->info.available = false;
		return rc;
	}

	#if ENABLE_OTP_DUMP
	rc += ussys_tp_set_clk_gating(dev);
	rc += ussys_tp_dump_otp(dev);
	#endif
	#if ENABLE_INIT_REG
	rc += ussys_tp_init_regs(dev);
	#endif
	rc += ussys_tp_force_reset_hold(dev);
	rc += ussys_tp_download_default_fw(dev);
	rc += ussys_tp_force_reset_release(dev);
	//ensure fw is running
	mdelay(dev, 10);
	rc += ussys_tp_get_fw_version(dev);
	rc += ussys_tp_get_supported_sensors(dev);
	rc += ussys_tp_apply_cal_param(dev);
	#if USSYS_CASP_ENABLED && ENABLE_OFFSET_TRIMMING
	rc += ussys_tp_cali_zforce_offset(dev);
	#endif
	#if USSYS_CAP_ENABLED
	rc += ussys_tp_cap_auto_calibration(dev);
	#endif

	//rc += ussys_tp_set_polairty(dev, 1); // Flip polarity

	uint8_t int_mask = 0x01;
	rc += ussys_tp_write_fd_reg(dev, TOUCH_POINT_FD_REG_INT_MASK, &int_mask, sizeof(int_mask));
	#if ENABLE_REG_DUMP
    rc += ussys_tp_put_to_hold(dev, true);
	rc += ussys_tp_dump_regs(dev);
    rc += ussys_tp_put_to_hold(dev, false);
	#endif
	
	dev->info.available = true;
	debug_info("dev%d(%#X) init done(rc:%d)!\r\n", dev->dev_idx, dev->i2c_addr, rc);
	
	return rc;
}

/*
 * get button status
 * 0: default value
 * 1: button released
 * 2: button pressed
 */
int ussys_tp_if_get_btn_status(ussys_tp_dev_t *dev)
{
	int rc = 0;
	uint8_t buf = 0;

	rc = ussys_tp_read_fd_reg(dev, TOUCH_POINT_FD_REG_ALGO_STATUS, &buf, sizeof(buf));

	dev->info.btn_status = buf >> 6;
	return rc;
}

#if USSYS_CASP_ENABLED
/*
 * get zforce adc (8-bit format)
 */
int ussys_tp_if_get_zforce_adc(ussys_tp_dev_t *dev)
{
	int rc = 0;
	uint8_t val = 0;

	rc = ussys_tp_read_fd_reg(dev, TOUCH_POINT_FD_REG_ZFORCE_ADC, &val, 1);

	dev->info.zforce_adc = val << 3;
	return rc;
}
/*
 * get zforce offset(8-bit format)
 */
int ussys_tp_if_get_zforce_offset(ussys_tp_dev_t *dev)
{
	int rc = 0;
	uint8_t val = 0;

	rc = ussys_tp_read_fd_reg(dev, TOUCH_POINT_FD_REG_ZFORCE_OFFSET, &val, 1);

	dev->info.zforce_offset = val << 3;
	return rc;
}
#endif

#if USSYS_CAP_ENABLED
/*
 * get cap adc
 */
int ussys_tp_if_get_cap_adc(ussys_tp_dev_t *dev)
{
	int rc = 0;
	uint8_t buf;

	rc = ussys_tp_read_fd_reg(dev, TOUCH_POINT_FD_REG_CAP_ADC, &buf, 1);
	dev->info.cap_adc = buf + TOUCH_POINT_SNS_CAP_OFFSET_VALUE;
	return rc;
}
#endif

#if USSYS_USP_ENABLED
/*
 * get usp adc
 */
int ussys_tp_if_get_usp_adc(ussys_tp_dev_t *dev)
{
	int rc = 0;
	uint8_t buf[2] = {0, 0};

	rc = ussys_tp_read_fd_reg(dev, TOUCH_POINT_FD_REG_USP_ADC, buf, ARRAY_SIZE(buf));

	dev->info.usp_adc = /*TOUCH_POINT_SNS_USP_OFFSET_VALUE - */buf[0];
	dev->info.usp_offset = /*TOUCH_POINT_SNS_USP_OFFSET_VALUE - */buf[1];
	return rc;
}
#endif
