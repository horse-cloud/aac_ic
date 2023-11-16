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
#include "ledcontrol.h"
#include "touch_slider_service.h"

extern void led_strip_main();

#define RT903_CHIP_NUMBER_MAX 4
static QueueHandle_t gpio_int_evt_queue = NULL;
static QueueHandle_t gpio_switch_evt_queue = NULL;
static int8_t number = 0;
static int8_t gain_value = 0x0;

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

const uint8_t INPUT_INT[] = 
{
    INPUT_INT1,
    INPUT_INT2,
    INPUT_INT3,
    INPUT_INT4,
    INPUT_INT5,
    INPUT_INT6,
    INPUT_INT7,
    INPUT_INT8,
};

const uint8_t SMART_SURFACE_SWITCH[] = {
    SMART_SURFACE_SWITCH1,
    SMART_SURFACE_SWITCH2,
    SMART_SURFACE_SWITCH3,
    SMART_SURFACE_SWITCH4,
    SMART_SURFACE_SWITCH5,
};

//保证hard_gain_play_list和soft_gain_play_list的长度一致
uint16_t hard_gain_play_list[]= {0x20, 0x55, 0x80};
uint16_t soft_gain_play_list[]= {0x55, 0x65, 0x80};
int8_t gain_play_list_len = sizeof(hard_gain_play_list)/sizeof(hard_gain_play_list[0]);

void set_i2c_master_num(uint8_t num);
void ussys_tp_main(void);

void cust_gpio_intr_anyedge_config(uint8_t gpio_num) {
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_ANYEDGE; // 设置中断触发类型为边缘触发
    io_conf.pin_bit_mask = 1ULL<<gpio_num; // 设置GPIO x为中断引脚
    io_conf.mode = GPIO_MODE_INPUT; // 设置GPIO为输入模式
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE; 
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE; 
    gpio_config(&io_conf); // 应用配置
}

void cust_gpio_intr_negedge_config(uint8_t gpio_num) {
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_NEGEDGE ; // 设置中断触发类型为下降沿触发
    io_conf.pin_bit_mask = 1ULL<<gpio_num; // 设置GPIO x为中断引脚
    io_conf.mode = GPIO_MODE_INPUT; // 设置GPIO为输入模式
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE; 
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE; 
    gpio_config(&io_conf); // 应用配置
}

bool is_rt903_online(DEF_RT903_INFO rt903)
{
    return rt903.is_online;
}


void rt903_vibrate_task(void* arg) {
    int i =0;
    for (;;) {
        /* receive gpio_int_evt_queue data,  */
        uint8_t gpio_num = 0;
        printf("rt903_vibrate_task enter, i:%d\n", i++);
        if(xQueueReceive(gpio_int_evt_queue, &gpio_num, portMAX_DELAY) == pdPASS ){
            int level = gpio_get_level(gpio_num);
            vTaskDelay(10 / portTICK_PERIOD_MS);//消抖，隔10ms再获取状态
            if(level == gpio_get_level(gpio_num)){
                int j = number % EFFECT_NUMBER_MAX;
                printf("rt903_vibrate_task enter, gpio:%d, level:%d, effect number is :%d\n", gpio_num, level, j);
                switch (gpio_num)
                {
                    case INPUT_INT1:
                    case INPUT_INT2:
                        rt903x_Ram_prepare(RT903_INFO[0], hard_gain_play_list[gain_value], j,gpio_num);
                        rt903x_Ram_play(RT903_INFO[0]);
                        break;
                    case INPUT_INT3:
                    case INPUT_INT4:
                        if(0 == level){
                            rt903x_Ram_prepare(RT903_INFO[1], hard_gain_play_list[gain_value], j,gpio_num);
                            rt903x_Ram_play(RT903_INFO[1]);
                        }
                        break;
                    case INPUT_INT5:
                    case INPUT_INT6:
                    case INPUT_INT7:
                    case INPUT_INT8:
                        rt903x_Ram_prepare(RT903_INFO[2], soft_gain_play_list[gain_value], j,gpio_num);
                        rt903x_Ram_play(RT903_INFO[2]);
                        break;
    /*
                    case GPIO_NUM_6:
                    // if(level == 0){// 定义此触发为播放音效，播放音效时，只在按下时播放，抬起时不播放
                        prepare_ram_data(j);
                        rt903x_Ram_play(RT903_INFO[0]);
                    //  }
                        break;
                    case GPIO_NUM_7:
                        prepare_ram_data(j);
                        rt903x_Ram_play(RT903_INFO[1]);    
                        break;
                    case GPIO_NUM_8:
                        prepare_ram_data(j);
                        rt903x_Ram_play(RT903_INFO[3]);    
                        break;
                    case GPIO_NUM_10:
                    case GPIO_NUM_14:
                        if(0 == level){
                            vTaskDelay(10 / portTICK_PERIOD_MS);//消抖，隔10ms再获取状态
                            if(0 == gpio_get_level(gpio_num)){
                                gain_value++;
                                if(gain_value >= gain_play_list_len) gain_value = 0;
                                rt903x_stream_play_demo(RT903_INFO[1]);//临时使用音效提醒切换成功
                                printf("gain_value++:%d\n", gain_value);
                            }
                        }
                        break;
                    case GPIO_NUM_35:
                        if(0 == level){
                            vTaskDelay(10 / portTICK_PERIOD_MS);//消抖，隔10ms再获取状态
                            if(0 == gpio_get_level(gpio_num)){
                                number++;
                                if(number >= EFFECT_NUMBER_MAX) number = 0;
                                rt903x_stream_play_demo(RT903_INFO[0]);//临时使用音效提醒切换成功
                                printf("number--:%d\n", number);
                            }
                        }
                        break;*/
                    default:
                        break;
                }
            }
            // 延时50ms，在延时期间的消息清空，不予响应
            vTaskDelay(pdMS_TO_TICKS(50));
            xQueueReset(gpio_int_evt_queue);
        }
    }
    vTaskDelete(NULL); // 删除任务
}

void smart_surface_switch_dispatch(){
    int i =0;
    for (;;) {
        /* receive gpio_switch_evt_queue data,  */
        uint8_t gpio_num = 0;
        if(xQueueReceive(gpio_switch_evt_queue, &gpio_num, portMAX_DELAY) == pdPASS ){
            int level = gpio_get_level(gpio_num);
            if(0 == level){
                vTaskDelay(10 / portTICK_PERIOD_MS);//消抖，隔10ms再获取状态
                if(level == gpio_get_level(gpio_num)){
                    printf("smart_surface_switch_dispatch:gpio_num:%d\n", gpio_num);
                    switch (gpio_num){
                        case SMART_SURFACE_SWITCH1://切换效果
                            number++;
                            if(number >= EFFECT_NUMBER_MAX) number = 0;
                            break;
                        case SMART_SURFACE_SWITCH2://切gain值
                            gain_value++;
                            if(gain_value >= gain_play_list_len) gain_value = 0;
                            break;
                        case SMART_SURFACE_SWITCH3:
                        case SMART_SURFACE_SWITCH4:
                        case SMART_SURFACE_SWITCH5:

                        default:
                            break;
                    }
                    rt903x_stream_play_demo(RT903_INFO[0]);//临时使用音效提醒切换成功
                }
            }

        }
        // 延时50ms，在延时期间的消息清空，不予响应
        vTaskDelay(pdMS_TO_TICKS(50));
        xQueueReset(gpio_switch_evt_queue); 
    }
}


void cust_gpio_isr_handler(void* arg)
{
    uint8_t gpio_num = (uint8_t) arg;
    switch (gpio_num)
    {
        case INPUT_INT1:
        case INPUT_INT2:
        case INPUT_INT3:
        case INPUT_INT4:
        case INPUT_INT5:
        case INPUT_INT6:
        case INPUT_INT7:
        case INPUT_INT8:
            xQueueSendFromISR(gpio_int_evt_queue, &gpio_num, NULL);
            break;
        case SMART_SURFACE_SWITCH1:
        case SMART_SURFACE_SWITCH2:
        case SMART_SURFACE_SWITCH3:
        case SMART_SURFACE_SWITCH4:
        case SMART_SURFACE_SWITCH5:
            xQueueSendFromISR(gpio_switch_evt_queue, &gpio_num, NULL);
        default:
            break;
    }
}

//控制板载灯光
void rgb_control(){
    //重新修改io为gpio7， 设置为输出模式
    led_strip_main();
}

void app_main(void)
{
    vTaskDelay(pdMS_TO_TICKS(1000));
#if 1
//创建振动处理任务，在gpio触发时处理cust_gpio_isr_handler中发送的消息
    gpio_int_evt_queue = xQueueCreate(10, sizeof(uint8_t));
    gpio_switch_evt_queue = xQueueCreate(10, sizeof(uint8_t));
    
//注册中断服务函数，中断优先级1
    gpio_install_isr_service(1);
    //初始化压力中断引脚
    for(int i=0; i < INT_DATA_MAX;i++){
        cust_gpio_intr_anyedge_config(INPUT_INT[i]);
        //cust_gpio_intr_negedge_config(INPUT_INT[i]);
        gpio_isr_handler_remove(INPUT_INT[i]);
        gpio_isr_handler_add(INPUT_INT[i],cust_gpio_isr_handler,(void*)INPUT_INT[i]);   //第一个参数触发中断源，第二个回调函数，第三个传入参数
    }

    //初始化开关中断引脚
    for(int i=0; i < SWITCH_MAX;i++){
        //cust_gpio_intr_anyedge_config(SMART_SURFACE_SWITCH[i]);
        cust_gpio_intr_negedge_config(SMART_SURFACE_SWITCH[i]);
        gpio_isr_handler_remove(SMART_SURFACE_SWITCH[i]);
        gpio_isr_handler_add(SMART_SURFACE_SWITCH[i],cust_gpio_isr_handler,(void*)SMART_SURFACE_SWITCH[i]);   //第一个参数触发中断源，第二个回调函数，第三个传入参数
    }


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

//创建子任务
    xTaskCreate(rt903_vibrate_task, "rt903_vibrate_task", 2048, NULL, 10, NULL);
    xTaskCreate(smart_surface_switch_dispatch, "smart_surface_switch_dispatch", 2048, NULL, 10, NULL);

//通过判断rt903 chip是否online来决定对应的gpio或者事件是否触发振动task
#if 1
    set_i2c_master_num(I2C_MASTER_NUM0);
    ussys_tp_main();
    set_i2c_master_num(I2C_MASTER_NUM1);
    ussys_tp_main();
#endif 
#endif 
//触摸区域初始化
//     touch_slider_init();

//ledc 初始化
    // ledc_init();
    // //灯光控制
    // xTaskCreate(aac_ledc_task, "aac_ledc_task", 2048, NULL, 10, NULL);
	//filesystem_gpio_setup();

//点亮板载灯
 //   rgb_control();
    while (true) {
        printf("Hello from app_main!\n");
        vTaskDelay(portMAX_DELAY);
    }

}
