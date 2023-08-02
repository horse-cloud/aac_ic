#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include "rt903x.h"
#include "rt903x_reg.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_err.h"
#include <i2c_adapter.h>
#include "filesystem.h"

#define RT903_CHIP_NUMBER_MAX 4
static QueueHandle_t gpio_evt_queue = NULL;

DEF_RT903_INFO RT903_INFO[RT903_CHIP_NUMBER_MAX] = 
{
// {is_online, i2c_master_num, i2c_address, }
    {false, I2C_MASTER_NUM0, I2C_0_ADDRESS},    {false, I2C_MASTER_NUM0, I2C_1_ADDRESS},
    {false, I2C_MASTER_NUM1, I2C_0_ADDRESS},    {false, I2C_MASTER_NUM1, I2C_1_ADDRESS},
};
def_i2c_config_t i2cConfig[] = {
//i2c_master_num, i2c_master_sda_io, i2c_master_scl_io, i2c_master_freq_hz
    {I2C_MASTER_NUM0, I2C_MASTER_0_SDA_IO, I2C_MASTER_0_SCL_IO, I2C_MASTER_FREQ_HZ},
    {I2C_MASTER_NUM1, I2C_MASTER_1_SDA_IO, I2C_MASTER_1_SCL_IO, I2C_MASTER_FREQ_HZ},
};

void ussys_tp_main(void);

void cust_gpio_pullup_config(uint8_t gpio_num) {
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_POSEDGE; // 设置中断触发类型为下降沿触发
    io_conf.pin_bit_mask = 1ULL<<gpio_num; // 设置GPIO x为中断引脚
    io_conf.mode = GPIO_MODE_INPUT; // 设置GPIO为输入模式
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE; // 禁用下拉电阻
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE; // 使能上拉电阻
    gpio_config(&io_conf); // 应用配置
}

bool is_rt903_online(DEF_RT903_INFO rt903)
{
    return rt903.is_online;
}

void rt903_vibrate_task(void* arg) {
    int i =0;
    for (;;) {
        /* receive gpio_evt_queue data,  */
        uint8_t gpio_num = 0;
        printf("rt903_vibrate_task enter, i:%d\n", i++);
        if(xQueueReceive(gpio_evt_queue, &gpio_num, portMAX_DELAY) == pdPASS ){
            switch (gpio_num)
            {
                case GPIO_NUM_6:
                    rt903x_Ram_Play_Demo(RT903_INFO[0]);
                    break;
                case GPIO_NUM_7:    
                    rt903x_Ram_Play_Demo(RT903_INFO[1]);    
                    break;
                default:
                    break;
            }
            // 延时50ms，在延时期间的消息清空，不予响应
            vTaskDelay(pdMS_TO_TICKS(50));
            xQueueReset(gpio_evt_queue);
        }
    }
    vTaskDelete(NULL); // 删除任务
}


void cust_gpio_isr_handler(void* arg)
{
    uint8_t gpio_num = (uint8_t) arg;
    switch (gpio_num)
    {
        case GPIO_NUM_6:
        case GPIO_NUM_7:
            xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
            break;
        default:
            break;
    }
}


void app_main(void)
{

// 第二颗RT903，AD脚接48口，拉高
    gpio_reset_pin(GPIO_NUM_48);
    gpio_set_direction(GPIO_NUM_48, GPIO_MODE_OUTPUT); // 将GPIO48设置为输出模式
    gpio_set_level(GPIO_NUM_48, 1); // 将GPIO48设置为高电平

// gpio6 trig I2c_0_0x5e,  gpio7  trig I2c_0_0x5f
    cust_gpio_pullup_config(GPIO_NUM_6);
    cust_gpio_pullup_config(GPIO_NUM_7);
//注册中断服务函数，中断优先级1
    gpio_install_isr_service(1);
    //加入中断回调函数
    gpio_isr_handler_remove(GPIO_NUM_6);
    gpio_isr_handler_remove(GPIO_NUM_7);
    gpio_isr_handler_add(GPIO_NUM_6,cust_gpio_isr_handler,(void*)GPIO_NUM_6);   //第一个参数触发中断源，第二个回调函数，第三个传入参数
    gpio_isr_handler_add(GPIO_NUM_7,cust_gpio_isr_handler,(void*)GPIO_NUM_7);

//i2c 初始化, 需要放到gpio操作之后，不然gpio的操作会影响i2c
    i2c_master_init(i2cConfig[0]);
    i2c_master_init(i2cConfig[1]);

//rt903 初始化，四个IC全部初始化，如果未连接，设置为不在线 is_online=false
    for(int i=0;i<RT903_CHIP_NUMBER_MAX; i++){
        rt903x_soft_reset(RT903_INFO[i]);
        int32_t ret = rt903x_init(RT903_INFO[i]);
        if(ret >= 0){
            RT903_INFO[i].is_online = true;
        }else{
            printf("RT903_INFO[%d] is not online,set is_online=false!\n", i);
            RT903_INFO[i].is_online = false;
        }
    }

//创建振动处理任务，在gpio触发时处理cust_gpio_isr_handler中发送的消息
    gpio_evt_queue = xQueueCreate(10, sizeof(uint8_t));
    xTaskCreate(rt903_vibrate_task, "rt903_vibrate_task", 2048, NULL, 10, NULL);
//通过判断rt903 chip是否online来决定对应的gpio或者事件是否触发振动task

	// ussys_tp_main();
	//filesystem_gpio_setup();
    while (true) {
        printf("Hello from app_main!\n");
        vTaskDelay(portMAX_DELAY);
    }

}
