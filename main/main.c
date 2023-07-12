#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include "rt903x.h"
#include "rt903x_reg.h"
#include "esp_log.h"
#include "esp_err.h"
#include <i2c_adapter.h>
#include "filesystem.h"


void ussys_tp_main(void);
static const char *TAG = "i2c-simple-example";
void app_main(void)
{
	uint8_t reg_val1=0;
    sleep(5);
	rt903x_install();
    rt903x_soft_reset();
    rt903x_init();
	printf("Hello from begin!\n");
	ussys_tp_main();
	//filesystem_gpio_setup();
    while (true) {
        printf("Hello from app_main!\n");
        sleep(5);

#if 1
        //Stream play
//         rt903x_soft_reset();
//         rt903x_init();
//         rt903x_stream_play_demo();
#else
         //RAM play
         rt903x_soft_reset();
         rt903x_init();
         rt903x_Ram_Play_Demo();
#endif
    }



}
