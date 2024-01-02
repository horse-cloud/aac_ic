#include "touch_slider_service.h"
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "touch_element/touch_slider.h"

static const char *TAG = "Touch Slider Example";
#define TOUCH_SLIDER_CHANNEL_NUM     8

static touch_slider_handle_t slider_handle; //Touch slider handle

// ADC1 跟 ADC2 有区别，目前使用ADC2的touch pad
static const touch_pad_t channel_array[TOUCH_SLIDER_CHANNEL_NUM] = { //Touch slider channels array
    TOUCH_PAD_NUM4,
    TOUCH_PAD_NUM5,
    TOUCH_PAD_NUM6,
    TOUCH_PAD_NUM7,
    TOUCH_PAD_NUM8,
    TOUCH_PAD_NUM9,
    TOUCH_PAD_NUM10,
    TOUCH_PAD_NUM11,
};

/**
 * Using finger slide from slider's beginning to the ending, and output the RAW channel signal, then calculate all the
 * channels sensitivity of the slider, and you can decrease or increase the detection sensitivity by adjusting the threshold divider
 * which locates in touch_slider_global_config_t. Please keep in mind that the real sensitivity totally depends on the
 * physical characteristics, if you want to decrease or increase the detection sensitivity, keep the ratio of those channels the same.
 */
static const float channel_sens_array[TOUCH_SLIDER_CHANNEL_NUM] = {  //Touch slider channels sensitivity array
    0.310F,
    0.310F,
    0.310F,
    0.310F,
    0.310F,
    0.310F,
    0.310F,
    0.310F,
};
// for test 
//just for get touch pad raw data
uint32_t curVals=0;
static void get_raw_data_handler_task(void *arg){
    while(1){
        vTaskDelay(pdMS_TO_TICKS(1000));
        touch_pad_read_raw_data(TOUCH_PAD_NUM4, &curVals);
        ESP_LOGI(TAG, "Slider read raw data, pad4: /*%"PRIu32"*/",curVals);
        // touch_pad_read_raw_data(TOUCH_PAD_NUM5, &curVals);
        // ESP_LOGI(TAG, "Slider read raw data, pad5: %"PRIu32,curVals);
        // touch_pad_read_raw_data(TOUCH_PAD_NUM6, &curVals);
        // ESP_LOGI(TAG, "Slider read raw data, pad6: %"PRIu32,curVals);
        // touch_pad_read_raw_data(TOUCH_PAD_NUM7, &curVals);
        // ESP_LOGI(TAG, "Slider read raw data, pad7: %"PRIu32,curVals);
        // touch_pad_read_raw_data(TOUCH_PAD_NUM8, &curVals);
        // ESP_LOGI(TAG, "Slider read raw data, pad8: %"PRIu32,curVals);
        // touch_pad_read_raw_data(TOUCH_PAD_NUM9, &curVals);
        // ESP_LOGI(TAG, "Slider read raw data, pad9: %"PRIu32,curVals);
        // touch_pad_read_raw_data(TOUCH_PAD_NUM10, &curVals);
        // ESP_LOGI(TAG, "Slider read raw data, pad10: %"PRIu32,curVals);
    }
}

#if 0

/* Slider event handler task */
static void slider_handler_task(void *arg)
{
    (void) arg; //Unused
    touch_elem_message_t element_message;
    while (1) {
        /* Waiting for touch element messages */
        if (touch_element_message_receive(&element_message, portMAX_DELAY) == ESP_OK) {
            if (element_message.element_type != TOUCH_ELEM_TYPE_SLIDER) {
                continue;
            }
            /* Decode message */
            const touch_slider_message_t *slider_message = touch_slider_get_message(&element_message);
            if (slider_message->event == TOUCH_SLIDER_EVT_ON_PRESS) {
                ESP_LOGI(TAG, "Slider Press, position: %"PRIu32, slider_message->position);
            } else if (slider_message->event == TOUCH_SLIDER_EVT_ON_RELEASE) {
                ESP_LOGI(TAG, "Slider Release, position: %"PRIu32, slider_message->position);
            } else if (slider_message->event == TOUCH_SLIDER_EVT_ON_CALCULATION) {
                ESP_LOGI(TAG, "Slider Calculate, position: %"PRIu32, slider_message->position);
            }
        }

    }
}
#else 
/* Slider callback routine */
void slider_handler(touch_slider_handle_t out_handle, touch_slider_message_t *out_message, void *arg)
{
    (void) arg; //Unused
    if (out_handle != slider_handle) {
        return;
    }
    if (out_message->event == TOUCH_SLIDER_EVT_ON_PRESS) {
        ESP_LOGI(TAG, "Slider Press, position: %"PRIu32, out_message->position);
    } else if (out_message->event == TOUCH_SLIDER_EVT_ON_RELEASE) {
        ESP_LOGI(TAG, "Slider Release, position: %"PRIu32, out_message->position);
    } else if (out_message->event == TOUCH_SLIDER_EVT_ON_CALCULATION) {
        ESP_LOGI(TAG, "Slider Calculate, position: %"PRIu32, out_message->position);
    }
}
#endif

void touch_slider_init(void)
{
    /* Initialize Touch Element library */
    touch_elem_global_config_t global_config = TOUCH_ELEM_GLOBAL_DEFAULT_CONFIG();
    ESP_ERROR_CHECK(touch_element_install(&global_config));
    ESP_LOGI(TAG, "Touch element library installed");

    touch_slider_global_config_t slider_global_config = TOUCH_SLIDER_GLOBAL_DEFAULT_CONFIG();
    ESP_ERROR_CHECK(touch_slider_install(&slider_global_config));
    ESP_LOGI(TAG, "Touch slider installed");
    /* Create Touch slider */
    touch_slider_config_t slider_config = {
        .channel_array = channel_array,
        .sensitivity_array = channel_sens_array,
        .channel_num = (sizeof(channel_array) / sizeof(channel_array[0])),
        .position_range = 10
    };
    ESP_ERROR_CHECK(touch_slider_create(&slider_config, &slider_handle));
    /* Subscribe touch slider events (On Press, On Release, On Calculation) */
    ESP_ERROR_CHECK(touch_slider_subscribe_event(slider_handle, TOUCH_ELEM_EVENT_ON_PRESS | TOUCH_ELEM_EVENT_ON_RELEASE | TOUCH_ELEM_EVENT_ON_CALCULATION, NULL));
#if 0
    /* Set EVENT as the dispatch method */
    ESP_ERROR_CHECK(touch_slider_set_dispatch_method(slider_handle, TOUCH_ELEM_DISP_EVENT));
    /* Create a handler task to handle event messages */
    xTaskCreate(&slider_handler_task, "slider_handler_task", 4 * 1024, NULL, 5, NULL);
#else
    /* Set CALLBACK as the dispatch method */
    ESP_ERROR_CHECK(touch_slider_set_dispatch_method(slider_handle, TOUCH_ELEM_DISP_CALLBACK));
    /* Register a handler function to handle event messages */
    ESP_ERROR_CHECK(touch_slider_set_callback(slider_handle, slider_handler));
#endif
    xTaskCreate(&get_raw_data_handler_task, "get_raw_data_handler_task", 4 * 1024, NULL, 5, NULL);

    ESP_LOGI(TAG, "Touch slider created");
    touch_element_start();
    ESP_LOGI(TAG, "Touch element library start");
}
