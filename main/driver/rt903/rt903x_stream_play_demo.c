#include "i2c_adapter.h"
#include "rt903x_reg.h"
#include "rt903x.h"
#include "ics_util.h"
#include "string.h"
#include <stdint.h>

const int8_t stream_data[] =
{
 0,-3,-6,-2,0,-28,-24,58,51,-38,-89,-44,29,43,24,3,-21,-45,-33,6,32,26,0,-25,-31,-16,10,29,21,-6,
 -32,-34,-9,21,35,23,-5,-30,-35,-15,10,25,19,-2,-23,-28,-15,5,14,3,-17,-31,-29,-11,3,13,3,-20,-33,
 -26,-5,8,10,-2,-19,-30,-24,-7,6,9,1,-13,-23,-20,-3,12,15,1,-15,-23,-14,1,13,12,-1,-18,-24,-15,0,
 11,9,-4,-19,-23,-13,3,14,10,-5,-22,-25,-15,0,8,1,-14,-25,-25,-13,0,7,0,-15,-25,-22,-7,7,11,1,-13,
 -22,-17,-2,11,14,4,-8,-13,-6,2,4,4,9,6,-7,-14,-7,3,7,2,-5,-11,-12,-7,0,1,-1,-8,-13,-11,-7,-3,-2,
 -6,-12,-16,-15,-9,-4,-2,-7,-14,-18,-16,-11,-8,-9,-13,-16,-14,-11,-7,-5,-6,-11,-16,-15,-11,-5,-2,
 -5,-10,-13,-11,-5,0,2,0,-4,-6,-4,0,2,2,-2,-3,-1,2,1,2,4,4,-3,-8,-8,0,-18,-17,54,75,-26,-95,-55,10,
 42,35,17,-16,-49,-45,-1,40,44,10,-31,-54,-37,4,39,31,-8,-46,-52,-20,24,45,27,-18,-58,-63,-29,16,38,
 18,-22,-49,-42,-8,24,26,-6,-46,-57,-30,13,36,17,-25,-57,-49,-9,29,34,1,-41,-57,-33,5,26,13,-20,-42,
 -31,0,26,25,0,-31,-41,-20,14,33,22,-9,-32,-29,-1,26,31,9,-17,-23,-4,23,36,22,-3,-19,-11,13,33,30,5,
 -18,-20,0,27,39,24,-4,-20,-8,19,38,30,1,-21,-17,7,33,35,12,-15,-21,-1,28,39,19,-12,-27,-11,19,36,23,
 -8,-30,-21,8,32,28,-1,-28,-30,-4,24,29,5,-24,-30,-7,22,30,9,-20,-31,-14,14,27,12,-14,-26,-12,14,27,15,
 -9,-22,-11,12,24,15,-4,-13,-2,18,28,17,0,-4,9,26,29,15,0,0,14,28,26,9,-4,0,18,30,21,1,-6,5,22,26,12,-4,
 -6,9,25,23,3,-11,-6,13,24,14,-5,-12,1,20,23,6,-9,-5,13,25,15,-2,-7,6,21,17,0,-10,0,17,18,1,-12,-4,16,23,
 5,-14,-11,11,24,9,-13,-13,11,25,10,-13,-13,12,26,6,-18,-11,15,24,3,-19,-10,17,25,1,-18,-5,19,18,-4,-14,4,
 22,11,-9,-5,17,21,-2,-13,7,26,7,-16,-2,27,19,-13,-14,19,29,0,-14,12,27,2,-15,13,28,-1,-17,14,25,0,-18,
 21,21,-27,-4,35,-2,-33,18,32,-26,-23,36,14,-36,0,33,-14,-23,24,7,-28,5,21,-20,-14,24,-6,-22,16,2,-23,
 10,8,-20,6,10,-18,5,10,-18,5,9,-17,7,9,-16,8,7,-13,9,1,-7,10,-4,0,9,-7,7,4,-9,11,0,-2,12,-4,6,7,-4,11,
 2,2,11,0,9,5,2,12,-1,6,9,-3,12,1,0,16,-4,2,12,-5,9,8,-1,7,1,5,4,0,10,0,1,7,-1,3,1,1,6,-5,4,4,-8,8,2,-7,
 8,0,-5,5,0,0,0,1,3,-4,2,4,-4,1,0,-3,0,-1,0,0,-4,2,0,-4,3,1,-2,0,0,2,-3,0,3,-4,0,2,-3,1,0,1,1,-2,4,0,0,4,
 0,1,0,1,4,0,3,2,2,3,2,5,2,3,5,3,3,2,5,2,1,6,1,2,3,4,3,2,5,2,3,3,2,3,2,3,1,2,2,0,2,0,1,0,0,0,0,0,0,0,-1,0,
 -1,-1,0,-1,0,0,-1,-1,-1,-1,-1,-1,-2,-1,-2,-2,-2,-3,-3,-3,-2,-2,-3,-2,-2,-2,-1,-2,-1,-1,-2,-1,-1,-1,-1,-1,
 -1,-2,-1,-1,-1,-1,-1,-1,-1,-1,0,-1,-1,-1,-1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,1,1,0,1,0,0,0,0,1,0,
 0,0,0,0,0,0,0,0,0,-1,0,0,0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,0,-1,-1,-1,-1,-1,-1,-1,-1,-1,
 -1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

#define STREAM_DATA_LEN        sizeof(stream_data)
uint8_t stream_demo_flag = 0;
uint32_t data_demo_index = 0;

int32_t stream_play_demo_proc(DEF_RT903_INFO i2c_config)
{
    uint8_t reg_val = 0;
    while (1)
    {
        int16_t res = I2CReadReg(i2c_config.i2c_master_num, i2c_config.i2c_address, REG_INT_STATUS, &reg_val, 1);
        CHECK_ERROR_RETURN(res);
        if ((reg_val & BIT_INTS_PLAYDONE) > 0)
        {
            return 0;
        }
        if ((reg_val & BIT_INTS_FIFO_AF) > 0)
        {
            stream_demo_flag = 0;
        }
        if ((reg_val & BIT_INTS_FIFO_AE) > 0)
        {
            stream_demo_flag = 1;
        }
        if (stream_demo_flag == 1)
        {
            stream_demo_flag = 0;
            int32_t stream_size = ((rt903x_config.ram_param.ListBaseAddrH << 8) | rt903x_config.ram_param.ListBaseAddrL)
                - ((rt903x_config.ram_param.FifoAEH << 8) | rt903x_config.ram_param.FifoAEL);
            stream_size = min(STREAM_DATA_LEN - data_demo_index, stream_size);
            res = rt903x_stream_data(i2c_config, (const uint8_t*)stream_data + data_demo_index, stream_size);
            CHECK_ERROR_RETURN(res);
            data_demo_index += stream_size;
            if (data_demo_index >= STREAM_DATA_LEN)
            {
                break;
            }
        }
        ics_delay_ms(1);
    }
    return 0;
}

int32_t rt903x_stream_play_demo(DEF_RT903_INFO i2c_config)
{
    int32_t res = 0;
    uint8_t regvalue = 0x01;
    res = I2CWriteReg(i2c_config.i2c_master_num, i2c_config.i2c_address, REG_RAM_CFG, &regvalue, 1);
    // Clear all interruptions
    res = rt903x_clear_int(i2c_config);
    CHECK_ERROR_RETURN(res);
    res = rt903x_gain(i2c_config, 0x80);
    CHECK_ERROR_RETURN(res);
    res = rt903x_boost_voltage(i2c_config, BOOST_VOUT_850);
    CHECK_ERROR_RETURN(res);
    res = rt903x_play_mode(i2c_config, MODE_STREAM_PLAY);
    CHECK_ERROR_RETURN(res);
    res = rt903x_go(i2c_config, 1);
    CHECK_ERROR_RETURN(res);
    int32_t stream_size = (rt903x_config.ram_param.ListBaseAddrH << 8) | rt903x_config.ram_param.ListBaseAddrL;
    stream_size = min(STREAM_DATA_LEN, stream_size);
    res = rt903x_stream_data(i2c_config, (const uint8_t*)stream_data, stream_size);
    CHECK_ERROR_RETURN(res);
    data_demo_index += stream_size;
    res = stream_play_demo_proc(i2c_config);
    CHECK_ERROR_RETURN(res);
		data_demo_index = 0;
	
    return 0;
}
