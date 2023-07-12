/**
* @file ussys_tp_cap_calibration.c
*
* UltraSense Systems tpts process API
*
* Copyright (c) 2021 UltraSense Systems, Inc. All Rights Reserved.
*/

/*
* EDIT HISTORY FOR FILE
*
* This section contains comments describing changes made to the module.
*
*
* when         version    who              what
* ----------   --------   ----------       ---------------------------------
* 03/03/2023   1.0.0      Hanghang         first version for CapForce Self Test.
*
*/

/* Private includes ----------------------------------------------------------*/
#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include "ussys_tp_driver.h"

#if USSYS_CAP_ENABLED

/* Private defines ------------------------------------------------------------*/
#define MAX(A,B)  								((A) > (B) ? (A):(B))
#define ODR_HZ 									(50) //This should match ODR (Hz) specified in 8051 FW
#define ODR_CYCLE_TIME 							(5 + (1000 / ODR_HZ)) //One ODR cycle time + additional 5ms buffer
#define TRIM_BATCH_RETRY      					(3)
#define MAX_FSnsrDevNum							(4)
#define HW_TRIM_REG_LEN							(0x93-0x80+1)
#define CAP_DELTA_REG							(0x1340) // (SWREG_THS_REVC + offsetof(struct ZForceAlgo1stGlobalTHS, cap.s.offset_delta))
#define CAP_OFFSET_DECIM_ASCEND					(0x1341) // (SWREG_THS_REVC + offsetof(struct ZForceAlgo1stGlobalTHS, cap.s.offset_decim_ascend))
#define CAP_OFFSET_DECIM_DESCEND				(0x1342) // (SWREG_THS_REVC + offsetof(struct ZForceAlgo1stGlobalTHS, cap.s.offset_decim_descend))
#define CAP_PRESS_THS							(0x1344) // (SWREG_THS_REVC + offsetof(struct ZForceAlgo1stGlobalTHS, cap.s.threshold_press))
#define CAP_LEAKCNT_REG							(0x1346) // (SWREG_THS_REVC + offsetof(struct ZForceAlgo1stGlobalTHS, cap.s.leak_count))
#define TRIM_BATCH_SAMPLE_NUM 					(50)
#define AUTOCAL_CAP_LEAKCNT_MIN   				(0x02)
#define AUTOCAL_CAP_LEAKCNT_MAX   				(0x30)
#define AUTOCAL_CAP_LEAKCNT_STEP  				(1)
#define AUTOCAL_CAP_LEAKCNT_NUM 				(((AUTOCAL_CAP_LEAKCNT_MAX - AUTOCAL_CAP_LEAKCNT_MIN) / AUTOCAL_CAP_LEAKCNT_STEP) + 1)

/* Private structures ---------------------------------------------------------*/
typedef struct TrimReport {
	int8_t cap_leakcnt;
	float cap_offset;
	float cap_noise;
	uint8_t Failed;
} TrimReportTyp;

typedef struct TrimCombData {
	float adcAir;
	float noiseAir;
} TrimCombDataTyp;

/* the absolute range for checking for the time constant.
 * - right now the 8051-side uses the delay loop at 20mhz,
 *   but that has a minimum value of 5 so that's where we
 *   start the sweep.
 * - the max depends on the material's permitivity, but
 *   the naked sensor i've been starting my tests w/
 *   (no date code) hit 'zero' ~50 so a little slop
 *   for something / something / something w/o adding
 *   too much extra test time.
 */
typedef struct CalRreport{
	//TrimReportTyp regs;
	float adc10MHz32X_Contrast;
	float stdev32X;
	float CNR;
	float stdev8X;
	float CNRdB;
}CalRreportTyp;

/* Private values -----------------------------------------------------------*/
static float avg; 
static float stdev;
static const int MIN_ACCEPTABLE_ADC_FROM_TLT = 300;
static const int MAX_ACCEPTABLE_ADC_FROM_TLT = 1600;
static const float time_per_count_us = 0.6f;
static const float leaktime_overhead_us = 7.5f;
static const int resistance_leak_MOhm = 2;
static const int ADC_CHAR_OFFSET = -647;
static const int ADC_CHAR_GAIN = 2338;
static const float probe_cap_pF = 1.5f;
static const float adc_max_voltage = 1.1f;
static float stdevValues[TRIM_BATCH_SAMPLE_NUM];
static TrimCombDataTyp trimCTSData[AUTOCAL_CAP_LEAKCNT_NUM /*46*/];
//static uint8_t flagSlotMaskTrim[MAX_FSnsrDevNum];
static uint8_t plotDuringTrim = 1;
static uint8_t curStepNum = 0;
static TrimReportTyp bestConf[MAX_FSnsrDevNum];
static int showADC[MAX_FSnsrDevNum];
//static float adc10MHz32X_Contrast[MAX_FSnsrDevNum]={-1,-1,-1,-1};
//static float adc10MHz32X_Contrast[MAX_FSnsrDevNum]={-1,-1,-1,-1,-1,-1,-1,-1};
static uint8_t adcBytes[TRIM_BATCH_SAMPLE_NUM+1];
extern ussys_tp_dev_t FSnsr_dev[MAX_FSnsrDevNum];

/* calculate the mean and variance */
static void ussys_tp_cap_stdev(void)
{
	//for average
	float sum = 0;
	for(int i = 0; i < TRIM_BATCH_SAMPLE_NUM; i++)
		sum += stdevValues[i];

	//for average
	avg = sum / TRIM_BATCH_SAMPLE_NUM;

	//for variance
	float variance = 0 ;
	for(int i = 0; i < TRIM_BATCH_SAMPLE_NUM; i++)
		variance += (avg - stdevValues[i]) * (avg - stdevValues[i]);
	variance /= (TRIM_BATCH_SAMPLE_NUM - 1);

	//for stdev
	stdev = sqrtf(variance);
}

/* trimming sensor and calculate offset((1250+-250)/Contrast(170~240)) */
static void ussys_tp_cap_statistics_a_batch_adc(ussys_tp_dev_t *dev)
{
	//begin the trigger of this setting
	uint8_t value = 0xFF;
	uint16_t trim_retry_cnt = 0;
	uint8_t rc = 0;

	rc += ussys_tp_write_mem(dev, SWREG_EndLessModeRestart, &value, sizeof(value));

	/* a chunk of the noise is when polling overlaps w/ the sense and
	* something that looks very i2c-ish (hard to tell if it's strictly
	* sda or scl or some combination on the scope) gets coupled into
	* ncs. the per-sample timing isn't 1ms but this pushes us well
	* outside the window for overlap.
	*/
	mdelay(dev, TRIM_BATCH_SAMPLE_NUM);

	//readout the state byte and whole set of 8bit ADC in one shoot
	while(trim_retry_cnt < TRIM_BATCH_RETRY) {
		ussys_tp_read_mem(dev, SWREG_EndLessModeRestart, adcBytes, TRIM_BATCH_SAMPLE_NUM+1);
		if(adcBytes[0]!=0x00) {
			mdelay(dev, 10);
			trim_retry_cnt++;
			debug_info("* P SLOT%d %d Wait Endless Mode Data not ready \n\r",dev->dev_idx, (int)(dev->get_timestamp_us));
		}
		else
			break;
	}

	//adjust the data , recover its original scale , and send it for stdev
	for(int i = 0; i < TRIM_BATCH_SAMPLE_NUM; i++)
		stdevValues[i] = adcBytes[i + 1] * 8;

	ussys_tp_cap_stdev();

	if(plotDuringTrim) {
		showADC[dev->dev_idx] = (int)avg;
	}
}

/* cap self test step1 */
static void ussys_tp_cap_trimming_begin(ussys_tp_dev_t *dev)
{
	uint8_t check_mode = 0;
	uint8_t rc = 0;

	//reset bestConf to 0x00
	//default bestConf[x].Failed = 0;
	memset(&bestConf[dev->dev_idx], 0x0, sizeof(TrimReportTyp));

	bestConf[dev->dev_idx].cap_leakcnt = -1;

	//reset the showADC for showing in plot
	memset(showADC, 0, sizeof(showADC));

	uint8_t value = 0xFE;
	ussys_tp_write_mem(dev, SWREG_EndLessModeRestart, &value, sizeof(value));
	rc += ussys_tp_write_fd_reg_bitmask(dev, TOUCH_POINT_FD_REG_GPR_SENSOR_CONTROL, SENSOR_CONTROL_ENDLESS_MODE, SENSOR_CONTROL_ENDLESS_MODE);
	mdelay(dev, 25);
	rc += ussys_tp_read_fd_reg(dev, TOUCH_POINT_FD_REG_GPR_SENSOR_CONTROL, &check_mode, 1);

	debug_info("* P STEP%d SLOT%d CapSense_AutoCal...0x%X#\n\r", curStepNum, dev->dev_idx, ((dev->i2c_addr)<<1));
}

/* cap self test step2 */
static void ussys_tp_cap_auto_calibration_process(ussys_tp_dev_t *dev)
{
	curStepNum = 1;
	memset(trimCTSData, 0, sizeof(trimCTSData));
	uint8_t d, j;
	bool found_valid_pair = false;

	ussys_tp_write_fd_reg_bitmask(dev, TOUCH_POINT_FD_REG_GPR_SENSOR_CONTROL, SENSOR_CONTROL_DEVELOPMENT_MODE, SENSOR_CONTROL_DEVELOPMENT_MODE);
	mdelay(dev, ODR_CYCLE_TIME);

	for(d = AUTOCAL_CAP_LEAKCNT_MIN, j = 0; d >= AUTOCAL_CAP_LEAKCNT_MIN && d <= AUTOCAL_CAP_LEAKCNT_MAX; d += AUTOCAL_CAP_LEAKCNT_STEP, j++) {
		ussys_tp_write_mem(dev, CAP_LEAKCNT_REG, &d, 1);
		ussys_tp_notify_algo_update_param(dev);
		mdelay(dev, ODR_CYCLE_TIME);

		// harvest data from each sensor
		ussys_tp_cap_statistics_a_batch_adc(dev);

		if (((dev->i2c_addr) << 1) & 0x20 /* in 8051, pull up does 2047 - adc val*/)
			avg = 2047 - avg;

		trimCTSData[j].adcAir    = avg;
		trimCTSData[j].noiseAir  = stdev;

		if (plotDuringTrim)
			debug_info("* P STEP%d SLOT%d CapSense_AutoCal...0x%X :: %d, %d #\n\r", curStepNum, dev->dev_idx, d, (int)trimCTSData[j].adcAir, (int)trimCTSData[j].noiseAir );

		if (avg >= MIN_ACCEPTABLE_ADC_FROM_TLT && avg <= MAX_ACCEPTABLE_ADC_FROM_TLT)
		{
			found_valid_pair = true;
			break;
		}
	}
	//use acceptable tlt to calculate parasitic capacitance
	if (found_valid_pair) {
		float initial_voltage, log_term, parasitic_cap_pF;
		if (((dev->i2c_addr)<<1) & 0x20 /* pull up requires 1.8v*/) {
			initial_voltage = 1.8f;
			log_term = log(1.0f - (float)(avg - ADC_CHAR_OFFSET)/(ADC_CHAR_GAIN * initial_voltage));
		}
		else {
			initial_voltage = 1.4f;
			log_term = log((float)(avg - ADC_CHAR_OFFSET)/(ADC_CHAR_GAIN * initial_voltage));
		}
		parasitic_cap_pF = (-1.0f * (time_per_count_us * d + leaktime_overhead_us)) / (resistance_leak_MOhm * log_term);

		debug_info("* P STEP%d SLOT%d CapSense_AutoCal...Parasitic Cap(pF): %f #\n\r", curStepNum, dev->dev_idx, parasitic_cap_pF);

		//calculate optimal tlt
		float optimal_lt_us, optimal_lt_rounded_us;
		uint8_t optimal_lt_count;
		if (((dev->i2c_addr)<<1) & 0x20 /* pull up requires 1.8v*/) {
			log_term = log(1.0f/(1 - adc_max_voltage/initial_voltage));
			optimal_lt_us = resistance_leak_MOhm * parasitic_cap_pF * log_term;
		}
		else {
			log_term = log((parasitic_cap_pF + probe_cap_pF)/parasitic_cap_pF);
			optimal_lt_us = resistance_leak_MOhm * parasitic_cap_pF * (probe_cap_pF + parasitic_cap_pF) * log_term / probe_cap_pF;
		}
		optimal_lt_count = (uint8_t) ((optimal_lt_us - leaktime_overhead_us) / time_per_count_us);
		optimal_lt_rounded_us = optimal_lt_count * time_per_count_us + leaktime_overhead_us;
		debug_info("* P STEP%d SLOT%d CapSense_AutoCal...Optimal Leaktime(us): %f; correspond to %d counts #\n\r", curStepNum, dev->dev_idx, optimal_lt_us, optimal_lt_count);

		//estimate fullscale
		float e_with_probe = exp(-1.0f * optimal_lt_rounded_us/(resistance_leak_MOhm * (parasitic_cap_pF + probe_cap_pF)));
		float e_without_probe = exp(-1.0f * optimal_lt_rounded_us/(resistance_leak_MOhm * (parasitic_cap_pF)));
		int full_scale = (int)(ADC_CHAR_GAIN * initial_voltage * (e_with_probe - e_without_probe));
		debug_info("* P STEP%d SLOT%d CapSense_AutoCal...Estimated full scale (LSB): %d #\n\r", curStepNum, dev->dev_idx, full_scale);

		//use the lt to probe baseline, here no need to invert data for pull up registers
		ussys_tp_write_mem(dev, CAP_LEAKCNT_REG, &optimal_lt_count, 1);
		ussys_tp_notify_algo_update_param(dev);

		mdelay(dev, ODR_CYCLE_TIME);
		// harvest data
		ussys_tp_cap_statistics_a_batch_adc(dev);
		debug_info("* P STEP%d SLOT%d CapSense_AutoCal...Measured baseline (LSB): %d #\n\r", curStepNum, dev->dev_idx, (int)avg);

		if(1 /* TODO (DR): add sanity check */) {
			if (full_scale > 255)
			dev->cal_param.cap_offset_delta = 255;
			else
			dev->cal_param.cap_offset_delta = full_scale;

			dev->cal_param.cap_leak_cnt = optimal_lt_count;

			ussys_tp_write_mem(dev, CAP_DELTA_REG, &dev->cal_param.cap_offset_delta, sizeof(uint8_t));
			ussys_tp_write_mem(dev, CAP_LEAKCNT_REG, &dev->cal_param.cap_leak_cnt, sizeof(uint8_t));

			ussys_tp_notify_algo_update_param(dev);
			mdelay(dev, ODR_CYCLE_TIME);
		}
	}
	else {
		debug_info("* P STEP%d SLOT%d ERROR!! Failed to find a valid ADC/Leaktime pair! #\n\r", curStepNum, dev->dev_idx);
	}

	ussys_tp_write_fd_reg_bitmask(dev, TOUCH_POINT_FD_REG_GPR_SENSOR_CONTROL, SENSOR_CONTROL_DEVELOPMENT_MODE, 0);
}

/* cap self test step3 */
static void ussys_tp_cap_trimming_end(ussys_tp_dev_t *dev)
{
	uint8_t rc = 0;

	rc += ussys_tp_write_fd_reg_bitmask(dev, TOUCH_POINT_FD_REG_GPR_SENSOR_CONTROL, SENSOR_CONTROL_ENDLESS_MODE, 0);

	ussys_tp_notify_algo_update_param(dev);

	mdelay(dev, ODR_CYCLE_TIME); //make sure it's not endless mode any more.
}

/* cap self test */
int ussys_tp_cap_auto_calibration(ussys_tp_dev_t *dev)
{
    ussys_tp_cap_trimming_begin(dev);
    ussys_tp_cap_auto_calibration_process(dev);
    ussys_tp_cap_trimming_end(dev);
    return 0;  
}

#endif
