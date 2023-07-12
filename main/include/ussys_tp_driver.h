/**
* @file ussys_tp_driver.h
*
* UltraSense Systems Touch Point Driver Header File
*
* Copyright (c) 2022 UltraSense Systems, Inc. All Rights Reserved.
*/

#ifndef __USSYS_TP_DRIVER_H
#define __USSYS_TP_DRIVER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "Algo1stParam.h"
#include "BrahmsShared.h"

#define MS_TO_US(x)								(x * 1000)
#define debug_info(...)							printf("[ussys] "__VA_ARGS__)

#ifndef bool
	#define bool								int
	#define false								((bool)0)
	#define true								((bool)1)
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr)							(sizeof(arr)/sizeof(arr[0]))
#endif

/**********************************************************
 ******************** Common Constants ********************
 **********************************************************/
/* MAJOR.MINOR.PATCH */
#define USSYS_CAP_ENABLED							(1)
#define USSYS_CASP_ENABLED							(1)
#define USSYS_USP_ENABLED							(0)
#define USSYS_TEMP_ENABLED							(0)
#define USSYS_TP_DRIVER_VERSION						"1.0.4"
#define ENABLE_OFFSET_TRIMMING                      (1)
#define ENABLE_OTP_DUMP								(0)
#define ENABLE_REG_DUMP								(0)
#define ENABLE_INIT_REG								(0)
#define USSYS_TP_I2C_MAX_RETRY_NUM					(3)
#define USSYS_TP_I2C_CHUNK_SIZE						(32)
#define MIN_TOGGLE_INTERVAL_MS						(100)
#define TOUCH_POINT_A0A1_WHOAMI_VALUE				(0x62)
#define TOUCH_POINT_A2_WHOAMI_VALUE					(0x64)
#define TOUCH_POINT_SNS_USP_OFFSET_VALUE			(1500)
#define TOUCH_POINT_SNS_ZFORCE_OFFSET_VALUE			(1024)
#define TOUCH_POINT_SNS_CAP_OFFSET_VALUE			(1000)

/**********************************************************
 ******************** Register Map ********************
 **********************************************************/
/* one byte fd regs */
#define TOUCH_POINT_FD_REG_SC						(0x51)
#define TOUCH_POINT_FD_SC_HOST_INTERRUPT            (0x02)
#define TOUCH_POINT_FD_REG_INT_MASK					(0x52)
#define TOUCH_POINT_FD_REG_GPR0                     (0x54)
#define TOUCH_POINT_FD_REG_GPR1                     (0x55)
#define TOUCH_POINT_FD_REG_GPR_SENSOR_CONTROL		(0x57)
#define TOUCH_POINT_FD_REG_ADDR_INDEX_H				(0x60)
#define TOUCH_POINT_FD_REG_ADDR_INDEX_L				(0x61)
#define TOUCH_POINT_FD_REG_WHOAMI					(0x6E)
#define TOUCH_POINT_FD_REG_BURST_WRITE				(0x70)
#define TOUCH_POINT_FD_REG_BURST_READ				(0xF0)
/* RTL regs */
#define USSYS_REG_ZFORCE_AMP2_OFFSET_ADDR			(0x1835)
#define USSYS_REG_ZFORCE_SINC_OFFSET_ADDR			(0x1836)
#define USSYS_REG_ZFORCE_GAIN_ADDR					(0x1844)
#define USSYS_REG_USP_BANK0_START_ADDR				(0x1880)
#define USSYS_REG_USP_BANK0_END_ADDR				(0x1893)
#define USSYS_REG_USP_BANK0_SIZE					(USSYS_REG_USP_BANK0_END_ADDR - USSYS_REG_USP_BANK0_START_ADDR + 1)
#define TOUCH_POINT_REG_TX_FREQ						(0x1880)
#define TOUCH_POINT_REG_USP_ONOFF_MODE				(0x18E7)
#define TOUCH_POINT_RELEASE_BRAHM_RST_BIT_MASK		(0x01)
#define TOUCH_POINT_REG_FORCE_RUNNING_RST			(0x18FA)
#define TOUCH_POINT_FORCE_BRAHM_RST_BIT_MASK		(0x80)
#define TOUCH_POINT_REG_CLK_GATING					(0x18FB)
#define TOUCH_POINT_REG_WHOAMI_16BIT_ADDR			(0x18FD)
/* dsram memory */
#define SWREG_EndLessModeRestart					(0x1200)
#define SWREG_EndLessModeFIFO						(0x1201)
#define SWREG_SRAM_OFFSET_REVC						(0x1300)
#define SWREG_SupportedSensors 						(SWREG_SRAM_OFFSET_REVC+0x00)
#define SWREG_THS_REVC 								(SWREG_SRAM_OFFSET_REVC+0x20)
#define TOUCH_POINT_MEM_FW_VERSIONMain              (0x131E)
#define TOUCH_POINT_MEM_FW_VERSIONMinus             (0x131F)
#define TOUCH_POINT_MEM_SI_REV_NEW					(0x13FC)

/**********************************************************
 ********************* Data Structure *********************
 **********************************************************/
typedef enum{
	TOUCH_POINT_FD_REG_INVALID 			= 0x00,
	TOUCH_POINT_FD_REG_ALGO_STATUS		= 0x5A,	
	#if USSYS_CASP_ENABLED && !USSYS_USP_ENABLED && !USSYS_CAP_ENABLED
	//ZFORCE_ONLY_MODE
	TOUCH_POINT_FD_REG_ZFORCE_ADC,
	TOUCH_POINT_FD_REG_ZFORCE_OFFSET,
	TOUCH_POINT_FD_REG_TEMP_ADC,
	#elif USSYS_CASP_ENABLED && USSYS_USP_ENABLED && !USSYS_CAP_ENABLED
	//USP_ZFORCE_MODE
	TOUCH_POINT_FD_REG_ZFORCE_ADC,
	TOUCH_POINT_FD_REG_ZFORCE_OFFSET,
	TOUCH_POINT_FD_REG_USP_ADC,
	TOUCH_POINT_FD_REG_USP_OFFSET		= 0x6C,
	TOUCH_POINT_FD_REG_TEMP_ADC			= 0x6F,
	#elif USSYS_CASP_ENABLED && !USSYS_USP_ENABLED && USSYS_CAP_ENABLED
	//CAP_ZFORCE_MODE
	TOUCH_POINT_FD_REG_ZFORCE_ADC,
	TOUCH_POINT_FD_REG_ZFORCE_OFFSET,
	TOUCH_POINT_FD_REG_CAP_ADC,
	TOUCH_POINT_FD_REG_TEMP_ADC			= 0x6C,
	#elif !USSYS_CASP_ENABLED && !USSYS_USP_ENABLED && USSYS_CAP_ENABLED
	//CAP_DEPTH_MODE
	TOUCH_POINT_FD_REG_CAP_ADC,
	TOUCH_POINT_FD_REG_TEMP_ADC,
	#endif
	TOUCH_POINT_FD_REG_FIFO_NUM_BYTES	= 0x6C,
	TOUCH_POINT_FD_REG_CHIP_ID			= 0x6F,
	TOUCH_POINT_FD_REG_MAX				= 0xFF,
}ussys_mcu_gprx_t;

#pragma pack(1)
typedef struct ussys_cal_param {
#if USSYS_CAP_ENABLED
	/* cap leak cnt */
	uint8_t cap_leak_cnt;
	/* cap offset delta */	
	uint8_t cap_offset_delta;
	/* indicate if cal param is valid or not */
	uint8_t cap_param_is_valid;
#endif
#if USSYS_CASP_ENABLED
	/* zforce SINC offset [0x1836] */
	uint8_t zforce_offset_0x1836;
	/* zforce AMP2 offset [0x1835] */
	uint8_t zforce_offset_0x1835;
	/* zforce gain [0x1844] */
	uint8_t zforce_gain_0x1844;
	/* zforce contrast in 8-bits format */
	uint8_t algo_zforce_contrast;
	/* zforce noise in 8-bits format */
	uint8_t algo_zforce_noise;
#endif
#if USSYS_USP_ENABLED
	/* 20 bytes in total for accoustic params */
    uint8_t reg_accoustic[USSYS_REG_USP_BANK0_SIZE];
	/* usp max contrast for algo [I560uspMaxContrast] */
    uint8_t algo_usp_max_contrast;
	/* usp noise for algo [I580uspNoiseRMS] */
	uint8_t algo_usp_noise_rms;
#endif
	/* indicate if cal param is valid or not */
	uint8_t is_valid;
} ussys_cal_param_t;
#pragma pack()

typedef struct ussys_tp_info {
	/* whoami */
	uint8_t whoami;

	/* silicon revision */
	uint8_t si_rev;

	/* indicates if it's OTP-ed part */
	bool is_otp_part;

	/* indicates if this chip is initialized correctly */
	bool available;

	/* if true, toggle event occured at least once */
	bool toggled;

	/* the time when last toggle event occurs */
    uint64_t last_toggle_ts_us;

	/* cap adc(0 ~ 2047) */
	uint16_t cap_adc;
	
	/* zforce adc(0 ~ 2047) */
	uint16_t zforce_adc;

	/* zforce offset(0 ~ 2047)*/
	uint16_t zforce_offset;
	
	/* usp adc(0 ~ 2047)*/
	uint16_t usp_adc;
	
	/* usp offset(0 ~ 2047)*/
	uint16_t usp_offset;
	
	/* temp adc(0 ~ 2047)*/
	uint16_t temp_adc;

	/* button status(0:default, 1:released, 2:pressed) */
	uint8_t btn_status;
    
    uint8_t chipID[7];
	
	uint8_t fw_ver[2]; /* fw version */
	
	uint8_t supported_sensors; /* supported sensors */
} ussys_tp_info_t;

typedef struct ussys_tp_dev {
	/* dev index, must to fill */
	uint8_t dev_idx;

	/* i2c 7-bits slave address, must to fill */
	uint8_t i2c_addr;

	/* i2c read function, must to implement */
	int (*i2c_read)(struct ussys_tp_dev *dev, uint8_t *buf, uint16_t size);

	/* i2c write function, must to implement */
	int (*i2c_write)(struct ussys_tp_dev *dev, uint8_t *buf, uint16_t size);

	/* get system tick in microsecond, must to implement */
	uint64_t (*get_timestamp_us)(void);

	/* load usp & zforce cal_param from flash to dev structure */
	int (*load_cal_param)(struct ussys_tp_dev *dev);

	/* store usp & zforce cal_param from dev structure to flash */
	int (*store_cal_param)(struct ussys_tp_dev *dev);

	/* carry device's infomation */
	ussys_tp_info_t info;
	
	/* carry usp & zforce cal param */
	ussys_cal_param_t cal_param;
} ussys_tp_dev_t;

/**********************************************************
 ****************** Function Declaration ******************
 **********************************************************/
/* declaration for internal use */
void udelay(ussys_tp_dev_t *dev, uint32_t delay_us);
void mdelay(ussys_tp_dev_t *dev, uint32_t delay_ms);
int ussys_tp_read_fd_reg(ussys_tp_dev_t *dev, uint8_t reg_addr, uint8_t *val, uint16_t size);
int ussys_tp_write_fd_reg_bitmask(ussys_tp_dev_t *dev, uint8_t reg_addr, uint8_t mask, uint8_t val);
int ussys_tp_write_mem(ussys_tp_dev_t *dev, uint16_t mem_addr, uint8_t *val, uint16_t size);
int ussys_tp_read_mem(ussys_tp_dev_t *dev, uint16_t mem_addr, uint8_t *val, uint32_t size);
int ussys_tp_write_mem_bitmask(ussys_tp_dev_t *dev, uint16_t mem_addr, uint8_t mask, uint8_t val);
int ussys_tp_wakeup_via_reg(ussys_tp_dev_t *dev);
int ussys_tp_notify_algo_update_param(ussys_tp_dev_t *dev);
int ussys_tp_put_to_hold(ussys_tp_dev_t *dev, uint8_t en);
int ussys_tp_apply_cal_param(ussys_tp_dev_t *dev);
int ussys_tp_cali_zforce_offset(ussys_tp_dev_t *dev);
int ussys_tp_download_fw(ussys_tp_dev_t *dev, uint8_t *fw_buf, uint32_t fw_size);

/* user interface */
int ussys_tp_if_init(ussys_tp_dev_t *dev);
int ussys_tp_if_get_btn_status(ussys_tp_dev_t *dev);
int ussys_tp_if_get_zforce_adc(ussys_tp_dev_t *dev);
int ussys_tp_if_get_zforce_offset(ussys_tp_dev_t *dev);
int ussys_tp_if_get_cap_adc(ussys_tp_dev_t *dev);
int ussys_tp_if_get_usp_adc(ussys_tp_dev_t *dev);

#ifdef __cplusplus
}
#endif

#endif/*__USSYS_TP_DRIVER_H*/
