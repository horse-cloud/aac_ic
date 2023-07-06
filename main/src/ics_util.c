#include "ics_util.h"
#include "math.h"
#include <stdint.h>
#include <stdlib.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

const float PI = 3.1415926f;
static const float lpf_coef[7] =
{
    0.657783f,
    0.592384f,
    0.533488f,
    0.480447f,
    0.432679f,
    0.389661f,
    0.350919f,
};
static struct GENERATION_CONFIG generation_config;
static struct RESAMPLE_CONFIG resample_config;

static void gen_sine_waveform(uint8_t* buf, int16_t size)
{
    double phase_step = 2 * PI * generation_config.frequency / generation_config.sample_rate;
    for (int16_t i = 0; i < size; i++)
    {
        double phase = generation_config.start_phase + phase_step * i;
        double point_val = sin(phase) * generation_config.amplitude;
        buf[i] = (int8_t)point_val;
    }
    generation_config.start_phase += phase_step * size;
}

static void low_pass_filter(float* src_buf, float* dst_buf, int16_t size, enum LOWPASS_FILTER lpf)
{
    float alpha = lpf_coef[(int32_t)lpf - 1];
    dst_buf[0] = 0;
    for (int16_t i = 1; i < size; i++)
    {
        dst_buf[i] = (1 - alpha) * src_buf[i] + alpha * dst_buf[i - 1];
    }
}

static void gen_square_waveform(uint8_t* buf, int16_t size)
{
    double phase_step = 2 * PI * generation_config.frequency / generation_config.sample_rate;
    for (int16_t i = 0; i < size; i++)
    {
        double phase = generation_config.start_phase + phase_step * i;
        int32_t square_val = ((int32_t)floor(phase / PI) % 2) == 0 ? 1 : -1;
        double point_val = square_val * generation_config.amplitude;
        buf[i] = (int8_t)point_val;
    }
    generation_config.start_phase += phase_step * size;
    if (generation_config.lpf != LPF_NONE)
    {
        float* buf1 = (float*)malloc(sizeof(float) * size);
        float* buf2 = (float*)malloc(sizeof(float) * size);
        for (int16_t i = 0; i < size; i++)
        {
            buf1[i] = (float)buf[i];
        }
        low_pass_filter(buf1, buf2, size, generation_config.lpf);
        for (int16_t i = 0; i < size/2; i++)
        {
            float t = buf2[i];
            buf2[i] = buf2[size - 1 - i];
            buf2[size - 1 -i] = t;
        }
        low_pass_filter(buf2, buf1, size, generation_config.lpf);
        for (int16_t i = 0; i < size/2; i++)
        {
            float t = buf1[i];
            buf1[i] = buf1[size - 1 - i];
            buf1[size - 1 -i] = t;
        }
        for (int16_t i = 0; i < size; i++)
        {
            buf[i] = (int8_t)buf1[i];
        }
        free(buf1);
        free(buf2);
    }
}

int16_t ics_generation_reset(const struct GENERATION_CONFIG* config)
{
    generation_config = *config;
    return 0;
}

int16_t ics_generation_waveform(uint8_t* buf, int16_t size)
{
    switch(generation_config.waveform_type)
    {
        case WAVEFORM_SINE:
            gen_sine_waveform(buf, size);
            break;
        case WAVEFORM_SQUARE:
            gen_square_waveform(buf, size);
            break;
    }
    return 0;
}

int16_t ics_resample_reset(const struct RESAMPLE_CONFIG* config)
{
    resample_config = *config;
    return 0;
}

int16_t ics_resample_waveform(const uint8_t* src_buf, int16_t src_size, uint8_t* dst_buf, int16_t* dst_size)
{
    float g = resample_config.src_f0 / resample_config.dest_f0;
    int16_t count = (int16_t)floor((src_size - 1) * g) + 1;
    if (*dst_size < count)
    {
        return -1;
    }
    *dst_size = count;
    float step = 1 / g;
    for (int16_t i = 0;i < count; i++)
    {
        float t = step * i;
        int16_t p1 = (int16_t)floor(t);
        int16_t p2 = (int16_t)ceil(t);
        dst_buf[i] = (int8_t)((int8_t)src_buf[p1] + ((int8_t)src_buf[p2] - (int8_t)src_buf[p1]) * (t - p1));
    }
    return 0;
}

void ics_delay_ms(int16_t ms)
{
    // TODO: Following code depends on platform.
    // Replace it with your platform specific delay implementation.
	vTaskDelay(pdMS_TO_TICKS(ms));
}
