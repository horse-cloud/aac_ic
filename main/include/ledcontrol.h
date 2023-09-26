#ifndef __AAC_LED_CONTROL_H__
#define __AAC_LED_CONTROL_H__

#include <stdint.h>
#include "driver/gpio.h"
#include "driver/ledc.h"

#define LED_R_GPIO GPIO_NUM_21
#define LED_G_GPIO GPIO_NUM_20
#define LED_B_GPIO GPIO_NUM_19

#define LEDC_TIMER              LEDC_TIMER_0
#define LEDC_MODE               LEDC_LOW_SPEED_MODE
//#define LEDC_CHANNEL            LEDC_CHANNEL_0
#define LEDC_DUTY_RES           LEDC_TIMER_13_BIT // Set duty resolution to 13 bits
//#define LEDC_DUTY               (205)  // Set duty to 2.5%. ((2 ** 13) - 1) * 2.5% = 204.775
#define LEDC_FREQUENCY          (5000)    // Frequency in Hertz. Set frequency at 50 Hz
#define SERVO_MAX_DEGREE        (100)   // Maximum angle in degree upto which servo can rotate

#define LEDC_DUTY_0      0
#define LEDC_MAX_DUTY         	(8191)	//2的13次方-1(13位ledc)
#define LEDC_FADE_TIME    		(2000)	//渐变时间(ms)

#define LEDC_HS_MODE LEDC_LOW_SPEED_MODE // esp32s3似乎不支持LEDC_HIGH_SPEED_MODE
#define LEDC_MODE LEDC_LOW_SPEED_MODE
#define LEDC_TIMER  LEDC_TIMER_0


void ledc_init(void);
void led_control_play();
void aac_ledc_task(void* arg);
void ledp_pwm_Output(int LEDC_GPIO_NUM, int LEDC_CHNNEL_NUM, uint32_t LEDC_DUTY);
void ledp_red_Output(uint32_t LEDC_DUTY);
void ledp_green_Output(uint32_t LEDC_DUTY);
void ledp_blue_Output(uint32_t LEDC_DUTY);

#endif// __AAC_LED_CONTROL_H__