#ifndef __ICS_UTIL_H__
#define __ICS_UTIL_H__

#include <stdint.h>

#define CHECK_ERROR_RETURN(res)                                 \
    if(res < 0)                                                 \
    {                                                           \
        return -1;                                              \
    }

#define CHECK_ERROR_CLEAN(res)                                    \
    if(res < 0)                                                 \
    {                                                           \
        goto err;                                               \
    }

#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

enum WAVEFORM_TYPE
{
    WAVEFORM_SINE = 0,
    WAVEFORM_SQUARE
};

enum LOWPASS_FILTER
{
    LPF_NONE = 0,
    LPF_400,
    LPF_500,
    LPF_600,
    LPF_700,
    LPF_800,
    LPF_900,
    LPF_1000
};

struct GENERATION_CONFIG
{
    enum WAVEFORM_TYPE waveform_type;
    float frequency;
    float start_phase;
    uint8_t amplitude;
    uint32_t sample_rate;
    enum LOWPASS_FILTER lpf;
};

struct RESAMPLE_CONFIG
{
    float src_f0;
    float dest_f0;
};

#pragma pack(1)
struct FirmwareVersion
{
    uint32_t major;
    uint32_t minor;
    uint32_t patch;
};
#pragma pack()

int16_t ics_generation_reset(const struct GENERATION_CONFIG* config);
int16_t ics_generation_waveform(uint8_t* buf, int16_t size);
int16_t ics_resample_reset(const struct RESAMPLE_CONFIG* config);
int16_t ics_resample_waveform(const uint8_t* src_buf, int16_t src_size, uint8_t* dst_buf, int16_t* dst_size);

void ics_delay_ms(int16_t ms);

#endif // __ICS_UTIL_H__
