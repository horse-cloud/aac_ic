#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include "esp_timer.h"
#include "ussys_tp_driver.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_sleep.h"
#include "rt903x.h"


#define BUTTON_NUM					(4)
#define ENABLE_HARDCODE_CAL_PARAM	(1)
#define ENABLE_IRQ_TEST				(0)

#define I2C_MASTER_NUM              0                          /*!< I2C master i2c port number, the number of i2c peripheral interfaces available will depend on the chip */
#define ACK_CHECK_EN 0x1            /*!< I2C master will check ack from slave*/
#define WRITE_BIT I2C_MASTER_WRITE  /*!< I2C master write */
#define READ_BIT I2C_MASTER_READ    /*!< I2C master read */
#define I2C_MASTER_TIMEOUT_MS       1000
#define ACK_VAL 0x0                 /*!< I2C ack value */
#define NACK_VAL 0x1                /*!< I2C nack value */


#if ENABLE_HARDCODE_CAL_PARAM
static ussys_cal_param_t hardcode_cal_param[] = {
	/* sensor0 config */
	{
		#if USSYS_CAP_ENABLED
		//cap sensor
		0x10,0x60,false,
		#endif
		#if USSYS_CASP_ENABLED
		//casp sensor
		0x00,0x00,0x5E,0x32,0x01,
		#endif
		#if USSYS_USP_ENABLED
		//usp sensor
		0x05,0x06,0x5F,0x00,0x03,0xFF,0x00,0x00,0x05,0x01,
		0x00,0xAC,0x07,0xD4,0x19,0x00,0x3A,0x0A,0x50,0x05,
		0xFF,0x0A,
		#endif
		//valid
		true
	},
	
	/* sensor1 config */
	{
		#if USSYS_CAP_ENABLED
		//cap sensor
		0x10,0x60,false,
		#endif
		#if USSYS_CASP_ENABLED
		//casp sensor
		0x00,0x00,0x5E,0x32,0x06,
		#endif
		#if USSYS_USP_ENABLED
		//usp sensor
		0x05,0x06,0x5F,0x00,0x03,0xFF,0x00,0x00,0x05,0x01,
		0x00,0xAC,0x07,0xD4,0x19,0x00,0x3A,0x0A,0x50,0x05,
		0xFF,0x0A,
		#endif
		//valid
		true
	},
};
#endif

static int ussys_i2c_read(ussys_tp_dev_t *dev, uint8_t *buf, uint16_t size)
{
	uint16_t dev_addr = dev->i2c_addr;
    if (size == 0) {
        return ESP_OK;
    }
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (dev_addr << 1) | READ_BIT, ACK_CHECK_EN);
    if (size > 1) {
        i2c_master_read(cmd, buf, size - 1, ACK_VAL);
    }
    i2c_master_read_byte(cmd, buf + size - 1, NACK_VAL);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
	if (0 != ret){
		debug_info("ussys_i2c_read,ret:%d!\r\n",ret);
	}
    return ret;

    //HAL_I2C_Master_Receive(&hi2c1, dev->i2c_addr << 1, buf, size, 100)

}



static int ussys_i2c_write(ussys_tp_dev_t *dev, uint8_t *buf, uint16_t size)
{
	uint16_t devAddr = dev->i2c_addr;
    if (size == 0) {
        return ESP_OK;
    }
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (devAddr << 1) | WRITE_BIT, ACK_CHECK_EN);
    for (int i = 0; i < size; i++) {
        i2c_master_write_byte(cmd, buf[i], ACK_CHECK_EN);
    }
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
	if (0 != ret){
		debug_info("ussys_i2c_write,ret2:%d!\r\n",ret);
	}
    return ret;

    //HAL_I2C_Master_Transmit(&hi2c1, dev->i2c_addr << 1, buf, size, 100) == HAL_OK)

}

static uint64_t ussys_get_timestamp_us(void)
{
	//uint64_t ts = 1000 * HAL_GetTick();
	uint64_t ts = esp_timer_get_time();
	
	return ts;
}

/* load usp & zforce cal params from flash to dev structure */
int ussys_load_cal_param(ussys_tp_dev_t *dev)
{
#if ENABLE_HARDCODE_CAL_PARAM
	if (dev->dev_idx < ARRAY_SIZE(hardcode_cal_param)) {
		memcpy(&dev->cal_param, &hardcode_cal_param[dev->dev_idx], sizeof(ussys_cal_param_t));
		return 0;
	} else {
		debug_info("hardcode_cal_param array size is too small!\r\n");
		return -1;
	}
#else
	//return ussys_read_cal_data_from_flash(dev->dev_idx, (uint8_t *)&dev->cal_param, sizeof(ussys_cal_param_t));
  /* fake parameter load from flash */
  dev->cal_param.is_valid = 0xff;
  return 0;
#endif
}

/* store usp & zforce cal params from dev structure to flash */
int ussys_store_cal_param(ussys_tp_dev_t *dev)
{
	if (!dev->cal_param.is_valid)
		return -1;
	return 0;//return ussys_write_cal_data_into_flash(dev->dev_idx, (uint8_t *)&dev->cal_param, sizeof(ussys_cal_param_t));
}

void ussys_tp_main(void)
{
	ussys_tp_dev_t ussys_tp_dev[BUTTON_NUM];
	uint64_t ts = 0;
	int i = 0;
	int rc = 0;
	
	for (int i = 0; i < BUTTON_NUM; i++) {
		ussys_tp_dev_t *dev = &ussys_tp_dev[i];
		memset(dev, 0, sizeof(ussys_tp_dev_t));

		dev->dev_idx = i;
		if (i == 0) {
			dev->i2c_addr = 0x27;
		} 
		else if (i == 1) {
			dev->i2c_addr = 0x2F;
		}else if (i == 2){
			dev->i2c_addr = 0x37;
		}else if (i == 3){
			dev->i2c_addr = 0x3F;
		}

		dev->i2c_read			= ussys_i2c_read;
		dev->i2c_write			= ussys_i2c_write;
		dev->get_timestamp_us	= ussys_get_timestamp_us;
		dev->load_cal_param		= ussys_load_cal_param;
		dev->store_cal_param	= ussys_store_cal_param;
		ussys_tp_if_init(dev);
	}
#if 0	
	ts = ussys_get_timestamp_us();
	uint8_t last_status = 1;//default status  is released
	while (1) {
		#if ENABLE_IRQ_TEST
		if (irq_fired) {
			irq_fired = 0;
			debug_info("irq fired!\r\n");
		}
		#else
		if ((uint32_t)((ussys_get_timestamp_us() - ts)/1000) >= 10/*ms*/) {			
			int cnt = 0;
			char *buf = malloc(256);

			ts = ussys_get_timestamp_us();

			if (NULL != buf) {
				cnt += sprintf(buf+cnt, "ts %6d ", (int)ussys_get_timestamp_us());

				for (i = 0; i < BUTTON_NUM; i++) {
					ussys_tp_dev_t *dev = &ussys_tp_dev[i];
					
					cnt += sprintf(buf+cnt, "[%d] %#X ",
							dev->dev_idx,
							dev->i2c_addr);
					#if USSYS_CAP_ENABLED
					ussys_tp_if_get_cap_adc(dev);
					cnt += sprintf(buf+cnt, "%4d ",
							dev->info.cap_adc);	
					#endif
					#if USSYS_CASP_ENABLED
					ussys_tp_if_get_zforce_adc(dev);
                    ussys_tp_if_get_zforce_offset(dev);
					cnt += sprintf(buf+cnt, "%4d %4d ",
							dev->info.zforce_adc,
							dev->info.zforce_offset);
					#endif
					#if USSYS_USP_ENABLED
					ussys_tp_if_get_usp_adc(dev);
					cnt += sprintf(buf+cnt, "%4d ",
							dev->info.usp_adc);	
					#endif
					#if USSYS_TEMP_ENABLED
					ussys_tp_if_get_temp_adc(dev);
					cnt += sprintf(buf+cnt, "%4d ",
							dev->info.temp_adc);	
					#endif
					ussys_tp_if_get_btn_status(dev);
					cnt += sprintf(buf+cnt, "%d %s ",
							dev->info.btn_status,
							(dev->info.btn_status == 2) ? " pressed" : "released");
				// 原来是release，变成了press， 或者原来是press变成了release
					debug_info("last_status%d, btn_status:%d\r\n", last_status, dev->info.btn_status);
				if(last_status != dev->info.btn_status){
					last_status = dev->info.btn_status;
					rt903x_Ram_Play_Demo();
				}
				}

				debug_info("%s\r\n", buf);
				
				free(buf);
			}
		}
		#endif
	}
#endif
}

