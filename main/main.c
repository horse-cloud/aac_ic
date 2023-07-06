#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include "rt903x.h"
#include "rt903x_reg.h"
#include "esp_log.h"
#include "esp_err.h"
#include <i2c_adapter.h>

static const char *TAG = "i2c-simple-example";
void app_main(void)
{
	uint8_t reg_val1=0;
	rt903x_install();
    while (true) {
        printf("Hello from app_main!\n");
        sleep(5);

#if 0
        //Stream play
         rt903x_soft_reset();
         rt903x_init();
         rt903x_stream_play_demo();
#else
         //RAM play
         rt903x_soft_reset();
         rt903x_init();

         reg_val1 = 0;
         I2CReadReg(I2C_ADDRESS, REG_DEV_ID, &reg_val1, 1);
         ESP_LOGI(TAG, "app_main,REG_DEV_ID:%d",reg_val1);
         I2CReadReg(I2C_ADDRESS, REG_OSC_CFG1, &reg_val1, 1);
         ESP_LOGI(TAG, "app_main,reg_val1:%d",reg_val1);
         rt903x_Ram_Play_Demo();
#endif
    }



}
