
#include "rt903x.h"
#include "rt903x_reg.h"
#include "ics_util.h"
#include <i2c_adapter.h>
#include <stdint.h>
#include <math.h>
#include <stdlib.h>
#include "esp_log.h"
#include "esp_err.h"



static const char *TAG = "rt903-driver";

struct FirmwareVersion g_firmwareVersion =
{
    .major = 1,
    .minor = 0,
    .patch = 3
};

uint32_t g_efuse_v = 0;

struct RT903X_CONFIG rt903x_config =
{
    0, 0, 0, 0, 0, 0,
    {0x00,0x02,0x20,0x02,0x80,0x00,0x80,0x01}
};

const int8_t f0_wave_data[] =
{
    0x02,0x24,0x00,0xE7,
    0,17,34,50,65,79,92,103,112,119,124,126,126,124,119,113,104,93,80,66,51,35,18,1,-15,
	-32,-49,-64,-78,-91,-102,-111,-119,-123,-126,-126,-124,-120,-113,-105,-94,-81,-68,-52,
	-36,-19,-2,14,31,47,63,77,90,101,111,118,123,126,126,124,120,114,105,95,82,69,54,37,21,
	3,-13,-30,-46,-62,-76,-89,-101,-110,-118,-123,-126,-126,-125,-121,-114,-106,-96,-83,-70,
	-55,-39,-22,-5,11,29,45,61,75,88,100,109,117,123,126,126,125,121,115,107,97,84,71,56,40,23,6,
	-10,-27,-44,-60,-74,-87,-99,-109,-117,-122,-125,-126,-125,-121,-116,-107,-97,-85,-72,-57,-41,-25
	,-7,9,26,43,58,73,86,98,108,116,122,125,127,125,122,116,108,98,86,73,58,43,26,9,-7,-25,-41,-57,
	-72,-85,-97,-107,-116,-121,-125,-126,-125,-122,-117,-109,-99,-87,-74,-60,-44,-27,-10,6,23,40,56,
	71,84,97,107,115,121,125,126,126,123,117,109,100,88,75,61,45,29,11,-5,-22,-39,-55,-70,-83,-96,-106,
	-114,-121,-125,-126,-126,-123,-118,-110,-101,-89,-76,-62,-46,-30,-13
};
const uint8_t f0_list_data[] = {1,0,1,0};

uint8_t rl_bst_coe[25] =
{
    0x07,0x08,0x09,0x0A,0x0A,0x0B,0x0B,0x0B,0x0B,0x0B,0x0C,0x0C,0x0C,0x0C,0x0C,
    0x0D,0x0D,0x0D,0x0D,0x0D,0x0D,0x0D,0x0D,0x0D,0x0D
};
const int8_t rl_wave_data[] =
{
    0x02,0x24,0x00,0x0F,
    0,25,50,73,92,108,119,125,126,122,113,99,80,59,35
};
const uint8_t rl_list_data[] = {1,0,1,0};

typedef struct wave_description
{
        int8_t* wave;
        uint32_t len;
}wave_des;
// please fill the wavedata in this section
int8_t reserved_wave_data0[] = {0,17,34,50,65,79,92,103,112,119,124,126,126,124,119,113,104,93,80,66,51,35,18,1,-15,
	-32,-49,-64,-78,-91,-102,-111,-119,-123,-126,-126,-124,-120,-113,-105,-94,-81,-68,-52,
	-36,-19,-2,14,31,47,63,77,90,101,111,118,123,126,126,124,120,114,105,95,82,69,54,37,21,
	3,-13,-30,-46,-62,-76,-89,-101,-110,-118,-123,-126,-126,-125,-121,-114,-106,-96,-83,-70,
	-55,-39,-22,-5,11,29,45,61,75,88,100,109,117,123,126,126,125,121,115,107,97,84,71,56,40,23,6,
	-10,-27,-44,-60,-74,-87,-99,-109,-117,-122,-125,-126,-125,-121,-116,-107,-97,-85,-72,-57,-41,-25
	,-7,9,26,43,58,73,86,98,108,116,122,125,127,125,122,116,108,98,86,73,58,43,26,9,-7,-25,-41,-57,
	-72,-85,-97,-107,-116,-121,-125,-126,-125,-122,-117,-109,-99,-87,-74,-60,-44,-27,-10,6,23,40,56,
	71,84,97,107,115,121,125,126,126,123,117,109,100,88,75,61,45,29,11,-5,-22,-39,-55,-70,-83,-96,-106,
	-114,-121,-125,-126,-126,-123,-118,-110,-101,-89,-76,-62,-46,-30,-13};
int8_t reserved_wave_data1[] = {0};
int8_t reserved_wave_data2[] = {0};
int8_t reserved_wave_data3[] = {0};
int8_t reserved_wave_data4[] = {0};
// wave data list
wave_des wave_data_list[10]=
    {
        {reserved_wave_data0,sizeof(reserved_wave_data0)},
        {reserved_wave_data1,sizeof(reserved_wave_data1)},
        {reserved_wave_data2,sizeof(reserved_wave_data2)},
        {reserved_wave_data3,sizeof(reserved_wave_data3)},
        {reserved_wave_data4,sizeof(reserved_wave_data4)}
    };

#define F0_WAVE_DATA_LEN        sizeof(f0_wave_data)
#define F0_LIST_DATA_LEN        sizeof(f0_list_data)
#define RL_WAVE_DATA_LEN        sizeof(rl_wave_data)
#define RL_LIST_DATA_LEN        sizeof(rl_list_data)
#define MAX_RAM_SIZE            0x600    // 1.5K Bytes
#define EFS_BYTE_NUM            4

int32_t rt903x_soft_reset()
{
    uint8_t reg_val = 0x01;
    return I2CWriteReg(I2C_ADDRESS, REG_SOFT_RESET, &reg_val, 1);
}

int32_t rt903x_install()
{
	ESP_ERROR_CHECK(i2c_master_init());
	ESP_LOGI(TAG, "I2C initialized successfully");
	return 0;
}
int32_t rt903x_init()
{
    int32_t res = 0;
    uint8_t reg_val;
    struct RAM_PARAM *ram_param;

    rt903x_apply_trim();
    ram_param = (struct RAM_PARAM*)&rt903x_config.ram_param;
    reg_val = ram_param->ListBaseAddrL;
    res = I2CWriteReg(I2C_ADDRESS, REG_LIST_BASE_ADDR_L, &reg_val, 1);
    CHECK_ERROR_RETURN(res);
    reg_val = ram_param->ListBaseAddrH;
    res = I2CWriteReg(I2C_ADDRESS, REG_LIST_BASE_ADDR_H, &reg_val, 1);
    CHECK_ERROR_RETURN(res);
    reg_val = ram_param->WaveBaseAddrL;
    res = I2CWriteReg(I2C_ADDRESS, REG_WAVE_BASE_ADDR_L, &reg_val, 1);
    CHECK_ERROR_RETURN(res);
    reg_val = ram_param->WaveBaseAddrH;
    res = I2CWriteReg(I2C_ADDRESS, REG_WAVE_BASE_ADDR_H, &reg_val, 1);
    CHECK_ERROR_RETURN(res);
    reg_val = ram_param->FifoAEL;
    res = I2CWriteReg(I2C_ADDRESS, REG_FIFO_AE_L, &reg_val, 1);
    CHECK_ERROR_RETURN(res);
    reg_val = ram_param->FifoAEH;
    res = I2CWriteReg(I2C_ADDRESS, REG_FIFO_AE_H, &reg_val, 1);
    CHECK_ERROR_RETURN(res);
    reg_val = ram_param->FifoAFL;
    res = I2CWriteReg(I2C_ADDRESS, REG_FIFO_AF_L, &reg_val, 1);
    CHECK_ERROR_RETURN(res);
    reg_val = ram_param->FifoAFH;
    res = I2CWriteReg(I2C_ADDRESS, REG_FIFO_AF_H, &reg_val, 1);
    CHECK_ERROR_RETURN(res);
    reg_val = 0x08;
    res = I2CWriteReg(I2C_ADDRESS, REG_RAM_CFG, &reg_val, 1);
    CHECK_ERROR_RETURN(res);
    reg_val = 0x2B;
    res = I2CWriteReg(I2C_ADDRESS, REG_LRA_F0_CFG1, &reg_val, 1);
    CHECK_ERROR_RETURN(res);
    reg_val = 0x05;
    res = I2CWriteReg(I2C_ADDRESS, REG_LRA_F0_CFG2, &reg_val, 1);
    CHECK_ERROR_RETURN(res);

    ics_delay_ms(1);

    return 0;
}

int32_t rt903x_playlist_data(const uint8_t* buf, int32_t size)
{
    int32_t res = 0;
    uint8_t reg_val;
    reg_val = 0x02;
    res = I2CWriteReg(I2C_ADDRESS, REG_RAM_CFG, &reg_val, 1);
    struct RAM_PARAM *ram_param;
    ram_param = (struct RAM_PARAM*)&rt903x_config.ram_param;
    res = I2CWriteReg(I2C_ADDRESS, REG_RAM_ADDR_L, &ram_param->ListBaseAddrL, 1);
    CHECK_ERROR_RETURN(res);
    res = I2CWriteReg(I2C_ADDRESS, REG_RAM_ADDR_H, &ram_param->ListBaseAddrH, 1);
    CHECK_ERROR_RETURN(res);
    uint32_t copySize = min(size,MAX_RAM_SIZE);
    res = I2CWriteReg(I2C_ADDRESS, REG_RAM_DATA, (uint8_t*)buf, copySize);
    CHECK_ERROR_RETURN(res);
    return 0;
}

int32_t rt903x_waveform_data(const uint8_t* buf, int32_t size)
{
    int32_t res = 0;
    uint8_t reg_val;
    reg_val = 0x04;
    res = I2CWriteReg(I2C_ADDRESS, REG_RAM_CFG, &reg_val, 1);
    CHECK_ERROR_RETURN(res);
    struct RAM_PARAM *ram_param;
    ram_param = (struct RAM_PARAM*)&rt903x_config.ram_param;
    res = I2CWriteReg(I2C_ADDRESS, REG_RAM_ADDR_L, &ram_param->WaveBaseAddrL, 1);
    CHECK_ERROR_RETURN(res);
    res = I2CWriteReg(I2C_ADDRESS, REG_RAM_ADDR_H, &ram_param->WaveBaseAddrH, 1);
    CHECK_ERROR_RETURN(res);
    uint32_t copySize = min(size,MAX_RAM_SIZE);
    res = I2CWriteReg(I2C_ADDRESS, REG_RAM_DATA, (uint8_t*)buf, copySize);
    CHECK_ERROR_RETURN(res);
    return 0;
}

int32_t rt903x_stream_data(const uint8_t* buf, int32_t size)
{
    return I2CWriteReg(I2C_ADDRESS, REG_STREAM_DATA, (uint8_t*)buf, size);
}

int32_t rt903x_chip_id(void)
{
    int32_t res = 0;
    uint8_t reg_val;
    res = I2CReadReg(I2C_ADDRESS, 0, &reg_val, 1);
    CHECK_ERROR_RETURN(res);
    rt903x_config.chip_id = reg_val;
    if(CHIP_ID != reg_val)
    {
        return -2;
    }
    return 0;
}

int32_t rt903x_clear_int(void)
{
    uint8_t reg_val;
    return I2CReadReg(I2C_ADDRESS, REG_INT_STATUS, &reg_val, 1);
}

int32_t rt903x_clear_protection(void)
{
    int32_t res = 0;
    uint8_t reg_val = 0;

    res = I2CReadReg(I2C_ADDRESS, REG_BEMF_CFG2, &reg_val, 1);
    CHECK_ERROR_RETURN(res);
    reg_val |= 0x02;
    res = I2CWriteReg(I2C_ADDRESS, REG_BEMF_CFG2, &reg_val, 1);
    CHECK_ERROR_RETURN(res);
    reg_val &= 0xfd;
    res = I2CWriteReg(I2C_ADDRESS, REG_BEMF_CFG2, &reg_val, 1);
    CHECK_ERROR_RETURN(res);

    return 0;
}

int32_t rt903x_boost_voltage(RT903X_BOOST_VOLTAGE vout)
{
    int32_t res = 0;
    uint8_t reg_val;
    res = I2CReadReg(I2C_ADDRESS, REG_BOOST_CFG3, &reg_val, 1);
    CHECK_ERROR_RETURN(res);
    reg_val = (reg_val & 0xF0) | ((uint8_t)vout & 0x0F);
    return I2CWriteReg(I2C_ADDRESS, REG_BOOST_CFG3, &reg_val, 1);
}

int32_t rt903x_gain(uint8_t gain)
{
    return I2CWriteReg(I2C_ADDRESS, REG_GAIN_CFG, &gain, 1);
}

int32_t rt903x_play_mode(RT903X_PLAY_MODE mode)
{
    uint8_t reg_val = (uint8_t)mode;
    return I2CWriteReg(I2C_ADDRESS, REG_PLAY_MODE, &reg_val, 1);
}

static int32_t rt903x_efuse_read(uint8_t index, uint8_t *data)
{
    int32_t res = 0;
    uint8_t reg_val;
    reg_val = index;
    res = I2CWriteReg(I2C_ADDRESS, REG_EFS_ADDR_INDEX, &reg_val, 1);
    CHECK_ERROR_RETURN(res);
    reg_val = BIT_EFS_READ;
    res = I2CWriteReg(I2C_ADDRESS, REG_EFS_MODE_CTRL, &reg_val, 1);
    CHECK_ERROR_RETURN(res);
    ics_delay_ms(1);
    res = I2CReadReg(I2C_ADDRESS, REG_EFS_MODE_CTRL, &reg_val, 1);
    CHECK_ERROR_RETURN(res);
    if ((reg_val & BIT_EFS_READ) == 0)
    {
        res = I2CReadReg(I2C_ADDRESS, REG_EFS_RD_DATA, &reg_val, 1);
        CHECK_ERROR_RETURN(res);
        *data = (uint8_t)reg_val;
        return 1;
    }
    return 0;
}

int32_t rt903x_apply_trim()
{
    int32_t res = 0;
    uint8_t reg_val;
    uint32_t efs_data;
    uint8_t trim_val, *efs_data_p;
    /* soft reset */
    reg_val = 0x01;
    res = I2CWriteReg(I2C_ADDRESS, REG_SOFT_RESET, &reg_val, 1);
    CHECK_ERROR_RETURN(res);
    /* read out trim data from efuse */
    efs_data_p = (uint8_t*)&efs_data;
    for(int32_t i = 0; i < 4; ++i)
    {
        res = rt903x_efuse_read(i, efs_data_p + i);
        CHECK_ERROR_RETURN(res);
    }

    ESP_LOGI(TAG, "rt903x_apply_trim,efs_data:%ld",efs_data);
    /* apply trim data */
    trim_val = (efs_data & EFS_OSC_LDO_TRIM_MASK) >> EFS_OSC_LDO_TRIM_OFFSET;
    reg_val = trim_val << 6;
    trim_val = (efs_data & EFS_PMU_LDO_TRIM_MASK) >> EFS_PMU_LDO_TRIM_OFFSET;
    reg_val |= (trim_val << 4);
    res = I2CWriteReg(I2C_ADDRESS, REG_PMU_CFG3, &reg_val, 1);
    CHECK_ERROR_RETURN(res);
    reg_val=0;
    I2CReadReg(I2C_ADDRESS, REG_PMU_CFG3, &reg_val, 1);
    ESP_LOGI(TAG, "rt903x_apply_trim,REG_PMU_CFG3:%d",reg_val);
    trim_val = (efs_data & EFS_BIAS_1P2V_TRIM_MASK) >> EFS_BIAS_1P2V_TRIM_OFFSET;
    reg_val = trim_val << 4;
    trim_val = (efs_data & EFS_BIAS_I_TRIM_MASK) >> EFS_BIAS_I_TRIM_OFFSET;
    reg_val |= trim_val;
    res = I2CWriteReg(I2C_ADDRESS, REG_PMU_CFG4, &reg_val, 1);
    CHECK_ERROR_RETURN(res);
    reg_val=0;
    I2CReadReg(I2C_ADDRESS, REG_PMU_CFG4, &reg_val, 1);
    ESP_LOGI(TAG, "rt903x_apply_trim,REG_PMU_CFG4:%d",reg_val);

    trim_val = (efs_data & EFS_OSC_TRIM_MASK) >> EFS_OSC_TRIM_OFFSET;
    trim_val ^= 0x80;
    reg_val = trim_val;
    res = I2CWriteReg(I2C_ADDRESS, REG_OSC_CFG1, &reg_val, 1);
    CHECK_ERROR_RETURN(res);
    reg_val=0;
    I2CReadReg(I2C_ADDRESS, REG_OSC_CFG1, &reg_val, 1);
    ESP_LOGI(TAG, "rt903x_apply_trim,REG_OSC_CFG1:%d",reg_val);

    reg_val=0;
    res=I2CReadReg(I2C_ADDRESS, REG_OSC_CFG1, &reg_val, 1);
    CHECK_ERROR_RETURN(res);
    ESP_LOGI(TAG, "rt903x_apply_trim,REG_OSC_CFG1_0:%d",reg_val);

    reg_val=0;
    res=I2CReadReg(I2C_ADDRESS, REG_PMU_CFG3, &reg_val, 1);
    CHECK_ERROR_RETURN(res);
    ESP_LOGI(TAG, "rt903x_apply_trim,REG_PMU_CFG3_0:%d",reg_val);
    reg_val=0;
    res=I2CReadReg(I2C_ADDRESS, REG_PMU_CFG4, &reg_val, 1);
    CHECK_ERROR_RETURN(res);
    ESP_LOGI(TAG, "rt903x_apply_trim,REG_PMU_CFG4_0:%d",reg_val);



    reg_val = 0x0F;
    res = I2CWriteReg(I2C_ADDRESS, 0x2A, &reg_val, 1);
    CHECK_ERROR_RETURN(res);
    reg_val = 0x00;
    res = I2CWriteReg(I2C_ADDRESS, REG_BEMF_CFG1, &reg_val, 1);
    CHECK_ERROR_RETURN(res);
    reg_val = 0x01;
    res = I2CWriteReg(I2C_ADDRESS, REG_BEMF_CFG2, &reg_val, 1);
    CHECK_ERROR_RETURN(res);
    reg_val = 0x01;
    res = I2CWriteReg(I2C_ADDRESS, REG_BOOST_CFG1, &reg_val, 1);
    CHECK_ERROR_RETURN(res);
    reg_val = 0x05;
    res = I2CWriteReg(I2C_ADDRESS, REG_BOOST_CFG2, &reg_val, 1);
    CHECK_ERROR_RETURN(res);
    reg_val = 0x0A;
    res = I2CWriteReg(I2C_ADDRESS, REG_BOOST_CFG3, &reg_val, 1);
    CHECK_ERROR_RETURN(res);
    reg_val = 0x50;
    res = I2CWriteReg(I2C_ADDRESS, REG_BOOST_CFG4, &reg_val, 1);
    CHECK_ERROR_RETURN(res);
    reg_val = 0x1C;
    res = I2CWriteReg(I2C_ADDRESS, REG_BOOST_CFG5, &reg_val, 1);
    CHECK_ERROR_RETURN(res);
    reg_val = 0x2C;
    res = I2CWriteReg(I2C_ADDRESS, REG_PA_CFG1, &reg_val, 1);
    CHECK_ERROR_RETURN(res);
    reg_val = 0x03;
    res = I2CWriteReg(I2C_ADDRESS, REG_PA_CFG2, &reg_val, 1);
    CHECK_ERROR_RETURN(res);
    reg_val = 0x1C;
    res = I2CWriteReg(I2C_ADDRESS, REG_PMU_CFG2, &reg_val, 1);
    CHECK_ERROR_RETURN(res);
//    trim_val = (efs_data & EFS_VBAT_DET_TRIM_MASK) >> EFS_VBAT_DET_TRIM_OFFSET;
//    int32_t offset_val = (trim_val & 0x0F) * 313;
//    if ((trim_val & 0x10) != 0)
//    {
//        offset_val = 0 - offset_val;
//    }
//    rt903x_config.vbat_det_trim = offset_val - 1740;
//    rt903x_config.rl_det_trim = (efs_data & EFS_RL_DET_TRIM_MASK) >> EFS_RL_DET_TRIM_OFFSET;


    reg_val=0;
    res=I2CReadReg(I2C_ADDRESS, REG_OSC_CFG1, &reg_val, 1);
    CHECK_ERROR_RETURN(res);
    ESP_LOGI(TAG, "rt903x_apply_trim,REG_OSC_CFG1_1:%d",reg_val);
    reg_val=0;
    res=I2CReadReg(I2C_ADDRESS, REG_PMU_CFG3, &reg_val, 1);
    CHECK_ERROR_RETURN(res);
    ESP_LOGI(TAG, "rt903x_apply_trim,REG_PMU_CFG3_1:%d",reg_val);
    reg_val=0;
    res=I2CReadReg(I2C_ADDRESS, REG_PMU_CFG4, &reg_val, 1);
    CHECK_ERROR_RETURN(res);
    ESP_LOGI(TAG, "rt903x_apply_trim,REG_PMU_CFG4_1:%d",reg_val);


    return 0;
}

int32_t rt903x_go(uint8_t val)
{
    return I2CWriteReg(I2C_ADDRESS, REG_PLAY_CTRL, &val, 1);
}

int32_t rt903x_calc_f0(void)
{
    int32_t res = 0;
    uint8_t reg_val1, reg_val2;
    uint16_t cz_val1 = 0, cz_val2 = 0, cz_val3 = 0, cz_val4 = 0, cz_val5 = 0;
    res = I2CReadReg(I2C_ADDRESS, REG_BEMF_CZ1_VAL1, &reg_val1, 1);
    CHECK_ERROR_RETURN(res);
    res = I2CReadReg(I2C_ADDRESS, REG_BEMF_CZ1_VAL2, &reg_val2, 1);
    CHECK_ERROR_RETURN(res);
    cz_val1 = (uint16_t)(reg_val2 & 0x3F);
    cz_val1 = (uint16_t)((cz_val1 << 8) | reg_val1);
    res = I2CReadReg(I2C_ADDRESS, REG_BEMF_CZ2_VAL1, &reg_val1, 1);
    CHECK_ERROR_RETURN(res);
    res = I2CReadReg(I2C_ADDRESS, REG_BEMF_CZ2_VAL2, &reg_val2, 1);
    CHECK_ERROR_RETURN(res);
    cz_val2 = (uint16_t)(reg_val2 & 0x3F);
    cz_val2 = (uint16_t)((cz_val2 << 8) | reg_val1);
    res = I2CReadReg(I2C_ADDRESS, REG_BEMF_CZ3_VAL1, &reg_val1, 1);
    CHECK_ERROR_RETURN(res);
    res = I2CReadReg(I2C_ADDRESS, REG_BEMF_CZ3_VAL2, &reg_val2, 1);
    CHECK_ERROR_RETURN(res);
    cz_val3 = (uint16_t)(reg_val2 & 0x3F);
    cz_val3 = (uint16_t)((cz_val3 << 8) | reg_val1);
    res = I2CReadReg(I2C_ADDRESS, REG_BEMF_CZ4_VAL1, &reg_val1, 1);
    CHECK_ERROR_RETURN(res);
    res = I2CReadReg(I2C_ADDRESS, REG_BEMF_CZ4_VAL2, &reg_val2, 1);
    CHECK_ERROR_RETURN(res);
    cz_val4 = (uint16_t)(reg_val2 & 0x3F);
    cz_val4 = (uint16_t)((cz_val4 << 8) | reg_val1);
    res = I2CReadReg(I2C_ADDRESS, REG_BEMF_CZ5_VAL1, &reg_val1, 1);
    CHECK_ERROR_RETURN(res);
    res = I2CReadReg(I2C_ADDRESS, REG_BEMF_CZ5_VAL2, &reg_val2, 1);
    CHECK_ERROR_RETURN(res);
    cz_val5 = (uint16_t)(reg_val2 & 0x3F);
    cz_val5 = (uint16_t)((cz_val5 << 8) | reg_val1);
    rt903x_config.f0 = ((cz_val4 - cz_val2) + (cz_val5 - cz_val3)) >> 1;
    rt903x_config.f0 = 192000 / rt903x_config.f0;

    return res;
}

int32_t rt903x_detect_f0()
{
    int32_t res = 0;
    uint8_t reg_val;
    // Clear all interruptions
    res = I2CReadReg(I2C_ADDRESS, REG_INT_STATUS, &reg_val, 1);
    CHECK_ERROR_RETURN(res);
    // Fill the list data and waveform data
    res = rt903x_playlist_data(f0_list_data,F0_LIST_DATA_LEN);
    CHECK_ERROR_RETURN(res)
    res = rt903x_waveform_data((const uint8_t*)f0_wave_data,F0_WAVE_DATA_LEN);
    CHECK_ERROR_RETURN(res)
    reg_val = 0x20;
    res = I2CWriteReg(I2C_ADDRESS, REG_GAIN_CFG, &reg_val, 1);
    CHECK_ERROR_RETURN(res);
    reg_val = 0x00;
    res = I2CWriteReg(I2C_ADDRESS, REG_BRAKE_CFG1, &reg_val, 1);
    CHECK_ERROR_RETURN(res);
    reg_val = 0x1;
    res = I2CWriteReg(I2C_ADDRESS, REG_DETECT_F0_CFG, &reg_val, 1);
    CHECK_ERROR_RETURN(res);
    reg_val = 0x26;
    res = I2CWriteReg(I2C_ADDRESS, REG_BEMF_CFG3, &reg_val, 1);
    CHECK_ERROR_RETURN(res);
    reg_val = 0x20;
    res = I2CWriteReg(I2C_ADDRESS, REG_BEMF_CFG4, &reg_val, 1);
    CHECK_ERROR_RETURN(res);
    reg_val = 0x01;
    res = I2CWriteReg(I2C_ADDRESS, REG_PLAY_MODE, &reg_val, 1);
    CHECK_ERROR_RETURN(res);
    res = rt903x_go(1);
    CHECK_ERROR_RETURN(res);
    while (1)
    {
        res = I2CReadReg(I2C_ADDRESS, REG_PLAY_CTRL, &reg_val, 1);
        CHECK_ERROR_RETURN(res);
        if (reg_val == 0)
        {
            break;
        }
        ics_delay_ms(1);
    }

    ics_delay_ms(20);
    res = rt903x_calc_f0();
    CHECK_ERROR_RETURN(res);

    return 0;
}

static int32_t check_stream_play_status(void)
{
    uint8_t reg_val = 0;
    while (1)
    {
        int16_t res = I2CReadReg(I2C_ADDRESS, REG_INT_STATUS, &reg_val, 1);
        CHECK_ERROR_RETURN(res);
        if ((reg_val & BIT_INTS_PLAYDONE) > 0)
        {
            return 1;
        }

        if ((reg_val & BIT_INTS_FIFO_AE) > 0)
        {
            return 0;
        }

        if ((reg_val & BIT_INTS_PROTECTION) > 0)
        {
              return -1;
        }

        res = I2CReadReg(I2C_ADDRESS, REG_PLAY_CTRL, &reg_val, 1);
        CHECK_ERROR_RETURN(res);
        if ((reg_val & BIT_GO_MASK) == 0)
        {
            return 1;
        }

        ics_delay_ms(1);
    }
}

int32_t rt903x_play_long(uint16_t index, uint8_t gain, uint16_t duration)
{
    struct GENERATION_CONFIG gen_config =
    {
        WAVEFORM_SINE,
        rt903x_config.f0,
        0,
        64,
        6000,
        LPF_NONE
    };
    ics_generation_reset(&gen_config);

    int32_t total_size = duration * 6; //6k sample rate
    int32_t total_index = 0;
    int32_t fifo_size = (rt903x_config.ram_param.ListBaseAddrH << 8) | rt903x_config.ram_param.ListBaseAddrL;
    uint8_t *sin_gen_buf = (uint8_t *)malloc(fifo_size); //buf size depend on the fifo size

    int32_t res = 0;
    uint8_t reg_val = 0x01;
    res = I2CWriteReg(I2C_ADDRESS, REG_RAM_CFG, &reg_val, 1);
    // Clear all interruptions
    res = rt903x_clear_int();
    CHECK_ERROR_CLEAN(res);
    res = rt903x_gain(gain);
    CHECK_ERROR_CLEAN(res);
    res = rt903x_play_mode(MODE_STREAM_PLAY);
    CHECK_ERROR_CLEAN(res);
    res = rt903x_go(1);
    CHECK_ERROR_CLEAN(res);

    while(total_size > total_index)
    {
        int32_t gen_size = min(fifo_size, total_size - total_index);
        ics_generation_waveform(sin_gen_buf, gen_size);
        res = rt903x_stream_data((const uint8_t*)sin_gen_buf, gen_size);
        CHECK_ERROR_CLEAN(res);
        total_index += gen_size;
            
        res = check_stream_play_status();
        CHECK_ERROR_CLEAN(res);
        if (res == 0)     // FIFO AE
        {
            fifo_size = ((rt903x_config.ram_param.ListBaseAddrH << 8) | rt903x_config.ram_param.ListBaseAddrL)
                            - ((rt903x_config.ram_param.FifoAEH << 8) | rt903x_config.ram_param.FifoAEL);
        }
        else if (res == 1)  // Play Done or Stop
        {
            break;
        }
    }

err:
    free(sin_gen_buf);
    return res;
}

int32_t rt903x_play_transient(uint16_t index, uint8_t gain, uint16_t loop)
{
    struct RESAMPLE_CONFIG resample_config =
    {
        130.0f,
        rt903x_config.f0
    };
    ics_resample_reset(&resample_config);

    int16_t resample_size = wave_data_list[index].len;
    float g = resample_config.src_f0 / resample_config.dest_f0;
    resample_size = (int16_t)floor((resample_size - 1) * g) + 1;
    uint8_t *resample_buf = (uint8_t *)malloc(sizeof(uint8_t) * resample_size); //buf size depend on the resampled wave size
    ics_resample_waveform((const uint8_t*)wave_data_list[index].wave, wave_data_list[index].len, resample_buf, &resample_size);

    int32_t total_size = resample_size * loop;
    int32_t total_index = 0;
    int32_t fifo_size = (rt903x_config.ram_param.ListBaseAddrH << 8) | rt903x_config.ram_param.ListBaseAddrL;

    int32_t res = 0;
    uint8_t reg_val = 0x01;
    res = I2CWriteReg(I2C_ADDRESS, REG_RAM_CFG, &reg_val, 1);
    // Clear all interruptions
    res = rt903x_clear_int();
    CHECK_ERROR_CLEAN(res);
    res = rt903x_gain(gain);
    CHECK_ERROR_CLEAN(res);
    res = rt903x_play_mode(MODE_STREAM_PLAY);
    CHECK_ERROR_CLEAN(res);
    res = rt903x_go(1);
    CHECK_ERROR_CLEAN(res);

    while(total_size > total_index)
    {
        int32_t stream_size = min(fifo_size, total_size - total_index);
        while (stream_size > 0)
        {
            int32_t buf_offset = total_index % resample_size;
            int32_t batch_size = min(stream_size, resample_size - buf_offset);                    
            res = rt903x_stream_data((const uint8_t*)resample_buf + buf_offset, batch_size);
            CHECK_ERROR_CLEAN(res);
            stream_size -= batch_size;
            total_index += batch_size;
        }

        res = check_stream_play_status();
        CHECK_ERROR_CLEAN(res);
        if (res == 0)     // FIFO AE
        {
            fifo_size = ((rt903x_config.ram_param.ListBaseAddrH << 8) | rt903x_config.ram_param.ListBaseAddrL)
                            - ((rt903x_config.ram_param.FifoAEH << 8) | rt903x_config.ram_param.FifoAEL);
        }
        else if (res == 1)  // Play Done or Stop
        {
            break;
        }
    }

err:
    free(resample_buf);
    return res;
}

