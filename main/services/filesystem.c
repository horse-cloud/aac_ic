/*
 * filesystem.c
 *
 *  Created on: 2023年7月10日
 *      Author: 60057363
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"


#include "filesystem.h"

static const char *TAG = "filesystem";
//static QueueHandle_t  gpio_evt_queue = NULL;
//
//
//void IRAM_ATTR gpio_isr_handler(void* arg){
//    uint32_t gpio_num = (uint32_t) arg;
//    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
//}
//
//void gpio_task_example(void* arg)
//{
//	uint32_t io_num;
//	for(;;) {
//		if(xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY))
//		{
//			printf("GPIO[%d] intr, val: %d\n", io_num, gpio_get_level(io_num));
//		}
//	}
//}
//
//
///**
// * @brief gpio initialization
// */
//esp_err_t filesystem_gpio_init(void)
//{
//	gpio_config_t io_conf;
//	io_conf.intr_type = GPIO_PIN_INTR_POSEDGE;//interrupt of rising edge
//	io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;//bit mask of the pins, use GPIO32 here
//	io_conf.mode = GPIO_MODE_INPUT;//set as input mode
//	io_conf.pull_up_en = 0;//enable pull-up mode
//	gpio_config(&io_conf);
//
//	gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
//	//install gpio isr service
//	gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
//	//hook isr handler for specific gpio pin
//	gpio_isr_handler_add(GPIO_INPUT_IO_WAKEUP, gpio_isr_handler, (void*) GPIO_INPUT_IO_WAKEUP);
//
//	xTaskCreate(gpio_task_example, "gpio_task_example", 2048, NULL, 10, NULL);
//	return 0;
//}
//
//int32_t filesystem_gpio_setup()
//{
//	ESP_ERROR_CHECK(filesystem_gpio_init());
//	ESP_LOGI(TAG, "gpio initialized successfully");
//	return 0;
//}
