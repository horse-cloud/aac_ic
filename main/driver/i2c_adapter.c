/*
 * i2c_adapter.c
 *
 *  Created on: 2023年7月4日
 *      Author: 60057363
 */
#include <stdint.h>
#include <stdlib.h>
#include <esp_err.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "i2c_adapter.h"
#include "esp_log.h"
#include "driver/i2c.h"
#include "sdkconfig.h"


static const char *TAG = "i2c_adapter";

/**
 * @brief i2c master initialization
 */
esp_err_t i2c_master_init(void)
{
    int i2c_master_port = I2C_MASTER_NUM;
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
        // .clk_flags = 0,          /*!< Optional, you can use I2C_SCLK_SRC_FLAG_* flags to choose i2c source clock here. */
    };
    esp_err_t err = i2c_param_config(i2c_master_port, &conf);
    if (err != ESP_OK) {
        return err;
    }
    return i2c_driver_install(i2c_master_port, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
}



int16_t I2CReadReg(uint16_t devAddr, uint16_t ReadAddr, uint8_t *data_wr, uint16_t len) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, devAddr << 1 | WRITE_BIT, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, ReadAddr, ACK_CHECK_EN);
    i2c_master_start(cmd);

    i2c_master_write_byte(cmd, devAddr << 1 | READ_BIT, ACK_CHECK_EN);
    if (len > 1) {
        i2c_master_read(cmd, data_wr, len - 1, ACK_VAL);
    }
    i2c_master_read_byte(cmd, data_wr + len - 1, NACK_VAL);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    //ESP_LOGI(TAG, "I2CReadReg,ret:%d",ret);
    return 0;
}

int16_t I2CWriteReg(uint16_t devAddr, uint16_t WriteAddr, uint8_t *data_wr, uint16_t length) {
	esp_err_t ret = 0;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    ret = i2c_master_start(cmd);
    i2c_master_write_byte(cmd, devAddr << 1 | WRITE_BIT, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, WriteAddr, ACK_CHECK_EN);
    for (int i = 0; i < length; i++) {
        ret = i2c_master_write_byte(cmd, data_wr[i], ACK_CHECK_EN);
    }
    ret = i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);

    //ESP_LOGI(TAG, "I2CWriteReg,ret:%d",ret);
    return 0;


}

