#include "ledcontrol.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"


//ledc配置结构体
ledc_channel_config_t 	g_ledc_ch_r;
ledc_channel_config_t   g_ledc_ch_g;
ledc_channel_config_t   g_ledc_ch_b;
/*
* void ledc_init(void):定时器0用在PWM模式，输出2通道的LEDC信号
* @param[in]   void  		       :无
* @retval      void                :无
*/
void ledc_init(void)
{
	//定时器配置结构体
	ledc_timer_config_t 	ledc_timer;
	//定时器配置->timer0
	ledc_timer.duty_resolution = LEDC_TIMER_13_BIT; //PWM分辨率
	ledc_timer.freq_hz = LEDC_FREQUENCY;                      //频率
	ledc_timer.speed_mode = LEDC_HS_MODE;//LEDC_HIGH_SPEED_MODE;  	//速度
	ledc_timer.timer_num = LEDC_TIMER_0;           	// 选择定时器
    ledc_timer.clk_cfg = LEDC_AUTO_CLK;
	ledc_timer_config(&ledc_timer);					//设置定时器PWM模式

	//PWM通道0配置->LED_R_GPIO->红色灯
	g_ledc_ch_r.channel    = LEDC_CHANNEL_0;		//PWM通道
	g_ledc_ch_r.duty       = 0;						//占空比
	g_ledc_ch_r.gpio_num   = LED_R_GPIO;					//IO映射
	g_ledc_ch_r.speed_mode = LEDC_HS_MODE;//LEDC_HIGH_SPEED_MODE;	//速度
	g_ledc_ch_r.timer_sel  = LEDC_TIMER_0;			//选择定时器
	ledc_channel_config(&g_ledc_ch_r);				//配置PWM
	
	//PWM通道1配置->LED_G_GPIO->绿色灯
	g_ledc_ch_g.channel    = LEDC_CHANNEL_1;		//PWM通道
	g_ledc_ch_g.duty       = 0;						//占空比
	g_ledc_ch_g.gpio_num   = LED_G_GPIO;					//IO映射
	g_ledc_ch_g.speed_mode = LEDC_HS_MODE;//LEDC_HIGH_SPEED_MODE;	//速度
	g_ledc_ch_g.timer_sel  = LEDC_TIMER_0;			//选择定时器
	ledc_channel_config(&g_ledc_ch_g);				//配置PWM
	
	//PWM通道2配置->LED_R_GPIO->蓝色灯
	g_ledc_ch_b.channel    = LEDC_CHANNEL_2;		//PWM通道
	g_ledc_ch_b.duty       = 0;						//占空比
	g_ledc_ch_b.gpio_num   = LED_B_GPIO;					//IO映射
	g_ledc_ch_b.speed_mode = LEDC_HS_MODE;//LEDC_HIGH_SPEED_MODE;	//速度
	g_ledc_ch_b.timer_sel  = LEDC_TIMER_0;			//选择定时器
	ledc_channel_config(&g_ledc_ch_b);				//配置PWM
	
	//注册LEDC服务:相当于使能
	ledc_fade_func_install(0);
}

void aac_ledc_task(void* arg)
{
    for (;;) {
    		//ledc 红灯渐变至100%，时间LEDC_FADE_TIME
		ledc_set_fade_with_time(g_ledc_ch_r.speed_mode,
                    g_ledc_ch_r.channel, 
					LEDC_MAX_DUTY,
                    LEDC_FADE_TIME);
		//渐变开始
		ledc_fade_start(g_ledc_ch_r.speed_mode,
				g_ledc_ch_r.channel, 
				LEDC_FADE_NO_WAIT);
		//ledc 绿灯渐变至100%，时间LEDC_FADE_TIME
		ledc_set_fade_with_time(g_ledc_ch_g.speed_mode,
                    g_ledc_ch_g.channel, 
					LEDC_MAX_DUTY,
                    LEDC_FADE_TIME);
		//渐变开始
		ledc_fade_start(g_ledc_ch_g.speed_mode,
				g_ledc_ch_g.channel, 
				LEDC_FADE_NO_WAIT);
		//ledc 蓝灯渐变至100%，时间LEDC_FADE_TIME
		ledc_set_fade_with_time(g_ledc_ch_b.speed_mode,
                    g_ledc_ch_b.channel, 
					LEDC_MAX_DUTY,
                    LEDC_FADE_TIME);
		//渐变开始
		ledc_fade_start(g_ledc_ch_b.speed_mode,
				g_ledc_ch_b.channel, 
				LEDC_FADE_NO_WAIT);
		//延时LEDC_FADE_TIME，给LEDC控制时间
        vTaskDelay(LEDC_FADE_TIME / portTICK_PERIOD_MS);
		

		//ledc 红灯 渐变至0%，时间LEDC_FADE_TIME
		ledc_set_fade_with_time(g_ledc_ch_r.speed_mode,
                    g_ledc_ch_r.channel, 
					0,
                    LEDC_FADE_TIME);
		//渐变开始
		ledc_fade_start(g_ledc_ch_r.speed_mode,
				g_ledc_ch_r.channel, 
				LEDC_FADE_NO_WAIT);
		//ledc 绿灯渐变至0%，时间LEDC_FADE_TIME
		ledc_set_fade_with_time(g_ledc_ch_g.speed_mode,
                    g_ledc_ch_g.channel, 
					0,
                    LEDC_FADE_TIME);
		//渐变开始
		ledc_fade_start(g_ledc_ch_g.speed_mode,
				g_ledc_ch_g.channel, 
				LEDC_FADE_NO_WAIT);
		//ledc 蓝灯渐变至0%，时间LEDC_FADE_TIME
		ledc_set_fade_with_time(g_ledc_ch_b.speed_mode,
                    g_ledc_ch_b.channel, 
					0,
                    LEDC_FADE_TIME);
		//渐变开始
		ledc_fade_start(g_ledc_ch_b.speed_mode,
				g_ledc_ch_b.channel, 
				LEDC_FADE_NO_WAIT);
		//延时LEDC_FADE_TIME，给LEDC控制时间
        vTaskDelay(LEDC_FADE_TIME / portTICK_PERIOD_MS);
    }
}


//gpio输出不同占空比的pwm(不带缓冲时间)
//LEDC_NUM：输出的引脚，LEDC_DUTY：占空比
//无返回值
void ledp_pwm_Output(int LEDC_GPIO_NUM, int LEDC_CHNNEL_NUM, uint32_t LEDC_DUTY)
{
    ledc_channel_config_t ledc_channel = 
        {
            .channel    = LEDC_CHNNEL_NUM,
            .duty       = 0,
            .gpio_num   = LEDC_GPIO_NUM,
            .speed_mode = LEDC_MODE,
            .hpoint     = 0,
            .timer_sel  = LEDC_TIMER
        };
    ledc_channel_config(&ledc_channel);
    ledc_fade_func_install(0);

	ledc_set_duty(ledc_channel.speed_mode, ledc_channel.channel, LEDC_DUTY);
	ledc_update_duty(ledc_channel.speed_mode, ledc_channel.channel);

}
//显示不同亮度的红灯
//LEDC_DUTY：占空比
//无返回值
void ledp_red_Output(uint32_t LEDC_DUTY)
{
	ledp_pwm_Output(LED_R_GPIO, LEDC_CHANNEL_0, LEDC_DUTY);
	ledp_pwm_Output(LED_G_GPIO, LEDC_CHANNEL_1, LEDC_DUTY_0);
	ledp_pwm_Output(LED_B_GPIO, LEDC_CHANNEL_2, LEDC_DUTY_0);
}
//显示不同亮度的绿灯
//LEDC_DUTY：占空比
//无返回值
void ledp_green_Output(uint32_t LEDC_DUTY)
{
	ledp_pwm_Output(LED_R_GPIO, LEDC_CHANNEL_0, LEDC_DUTY_0);
	ledp_pwm_Output(LED_G_GPIO, LEDC_CHANNEL_1, LEDC_DUTY);
	ledp_pwm_Output(LED_B_GPIO, LEDC_CHANNEL_2, LEDC_DUTY_0);
}
//显示不同亮度的蓝灯
//LEDC_DUTY：占空比
//无返回值
void ledp_blue_Output(uint32_t LEDC_DUTY)
{
	ledp_pwm_Output(LED_R_GPIO, LEDC_CHANNEL_0, LEDC_DUTY_0);
	ledp_pwm_Output(LED_G_GPIO, LEDC_CHANNEL_1, LEDC_DUTY_0);
	ledp_pwm_Output(LED_B_GPIO, LEDC_CHANNEL_2, LEDC_DUTY);
}
