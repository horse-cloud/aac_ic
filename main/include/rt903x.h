#ifndef __RT903X_H
#define __RT903X_H
#include <stdint.h>
#include <stdbool.h>
#define    CHIP_ID    0x6B
#define TRUE 1
#define FALSE 0

#define EFFECT_NUMBER_MAX 2//4
#define SOUND_EFFECT_SUPPORT FALSE   
#pragma pack(1)
struct RAM_PARAM
{
    uint8_t ListBaseAddrL;
    uint8_t ListBaseAddrH;
    uint8_t WaveBaseAddrL;
    uint8_t WaveBaseAddrH;
    uint8_t FifoAEL;
    uint8_t FifoAEH;
    uint8_t FifoAFL;
    uint8_t FifoAFH;
};
#pragma pack()


typedef struct {
    bool is_online;       /*!< record chip is online or not>*/
    uint8_t i2c_master_num;/*!< I2C master number , 0  or 1> */
    uint16_t i2c_address;   /*!< I2C address, 0x5E or 0x5F>*/
} DEF_RT903_INFO;

struct RT903X_CONFIG {
    uint8_t chip_id;
    uint16_t f0;

    int32_t vbat_det_trim;
    uint8_t rl_det_trim;
    float vbat;
    float rl;

    struct RAM_PARAM ram_param;
};

typedef enum
{
    MODE_RAM_PLAY         = 1,
    MODE_STREAM_PLAY,
    MODE_AUTO_TRACK
} RT903X_PLAY_MODE;

typedef enum
{
    BOOST_VOUT_600    = 0,
    BOOST_VOUT_625    = 1,
    BOOST_VOUT_650    = 2,
    BOOST_VOUT_675    = 3,
    BOOST_VOUT_700    = 4,
    BOOST_VOUT_725    = 5,
    BOOST_VOUT_750    = 6,
    BOOST_VOUT_775    = 7,
    BOOST_VOUT_800    = 8,
    BOOST_VOUT_825    = 9,
    BOOST_VOUT_850    = 10,
    BOOST_VOUT_875    = 11,
    BOOST_VOUT_900    = 12,
    BOOST_VOUT_925    = 13,
    BOOST_VOUT_100    = 14,
    BOOST_VOUT_110    = 15
} RT903X_BOOST_VOLTAGE;

int32_t rt903x_soft_reset(DEF_RT903_INFO i2c_config);
int32_t rt903x_apply_trim(DEF_RT903_INFO i2c_config);
int32_t rt903x_init(DEF_RT903_INFO i2c_config);
int32_t rt903x_chip_id(DEF_RT903_INFO i2c_config);
int32_t rt903x_clear_int(DEF_RT903_INFO i2c_config);
int32_t rt903x_detect_f0(DEF_RT903_INFO i2c_config);

int32_t rt903x_play_long(DEF_RT903_INFO i2c_config, uint16_t index, uint8_t gain, uint16_t duration);
int32_t rt903x_play_transient(DEF_RT903_INFO i2c_config, uint16_t index, uint8_t gain, uint16_t loop);

int32_t rt903x_stream_data(DEF_RT903_INFO i2c_config, const uint8_t* buf, int32_t size);
int32_t rt903x_boost_voltage(DEF_RT903_INFO i2c_config, RT903X_BOOST_VOLTAGE vout);
int32_t rt903x_gain(DEF_RT903_INFO i2c_config, uint8_t gain);
int32_t rt903x_go(DEF_RT903_INFO i2c_config, uint8_t val);
int32_t rt903x_play_mode(DEF_RT903_INFO i2c_config, RT903X_PLAY_MODE mode);
int32_t rt903x_playlist_data(DEF_RT903_INFO i2c_config, const uint8_t* buf, int32_t size);
int32_t rt903x_waveform_data(DEF_RT903_INFO i2c_config, const uint8_t* buf, int32_t size);

int16_t rt903x_Ram_prepare(DEF_RT903_INFO i2c_config, uint8_t gain, uint8_t number, uint8_t area);
int16_t rt903x_Ram_play(DEF_RT903_INFO i2c_config);

int32_t rt903x_stream_play_demo(DEF_RT903_INFO i2c_config);

extern struct RT903X_CONFIG rt903x_config;

#endif // __RT903X