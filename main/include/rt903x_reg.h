#ifndef __RT903X_REG_LIST_H__
#define __RT903X_REG_LIST_H__

/******************************************************************************
 * rt903x register address
******************************************************************************/

#define RT903_I2C_1_ADDRESS			    0x5E
#define RT903_I2C_2_ADDRESS			    0x5F
#define REG_DEV_ID                      0x00
#define REG_REV_ID                      0x01
#define REG_SOFT_RESET                  0x02
#define REG_SYS_STATUS1                 0x03
#define REG_SYS_STATUS2                 0x04
#define REG_INT_STATUS                  0x05
#define REG_INT_MASK                    0x06
#define REG_RAM_CFG                     0x07
#define REG_RAM_ADDR_L                  0x08
#define REG_RAM_ADDR_H                  0x09
#define REG_RAM_DATA                    0x0A
#define REG_STREAM_DATA                 0x0B
#define REG_FIFO_AE_L                   0x0C
#define REG_FIFO_AE_H                   0x0D
#define REG_FIFO_AF_L                   0x0E
#define REG_FIFO_AF_H                   0x0F
    
#define REG_PLAY_CFG                    0x10
#define REG_PLAY_MODE                   0x11
#define REG_PLAY_CTRL                   0x12
#define REG_GPIO1_POS_ENTRY             0x13
#define REG_GPIO1_NEG_ENTRY             0x14
#define REG_GPIO2_POS_ENTRY             0x15
#define REG_GPIO2_NEG_ENTRY             0x16
#define REG_GPIO3_POS_ENTRY             0x17
#define REG_GPIO3_NEG_ENTRY             0x18
#define REG_GPIO_STATUS                 0x19
#define REG_PRIORITY_CFG                0x1A
#define REG_WAVE_BASE_ADDR_L            0x1C
#define REG_WAVE_BASE_ADDR_H            0x1D
#define REG_LIST_BASE_ADDR_L            0x1E
#define REG_LIST_BASE_ADDR_H            0x1F
    
#define REG_GAIN_CFG                    0x20
#define REG_SYS_CFG                     0x23
#define REG_STATE_MACHINE_CFG           0x25
#define REG_PROTECTION_STATUS1          0x26
#define REG_PROTECTION_STATUS2          0x27
#define REG_PROTECTION_MASK1            0x28
#define REG_PROTECTION_MASK2            0x29
#define REG_ERROR_CODE                  0x2C 
#define REG_LRA_F0_CFG1                 0x2D
#define REG_LRA_F0_CFG2                 0x2E
#define REG_DETECT_F0_CFG               0x2F
    
#define REG_BEMF_CZ1_VAL1               0x30
#define REG_BEMF_CZ1_VAL2               0x31
#define REG_BEMF_CZ2_VAL1               0x32
#define REG_BEMF_CZ2_VAL2               0x33
#define REG_BEMF_CZ3_VAL1               0x34
#define REG_BEMF_CZ3_VAL2               0x35
#define REG_BEMF_CZ4_VAL1               0x36
#define REG_BEMF_CZ4_VAL2               0x37
#define REG_BEMF_CZ5_VAL1               0x38
#define REG_BEMF_CZ5_VAL2               0x39
#define REG_BRAKE_CFG1                  0x3A
#define REG_BRAKE_CFG2                  0x3B
#define REG_BRAKE_CFG3                  0x3C
#define REG_BRAKE_CFG4                  0x3D
#define REG_BRAKE_CFG5                  0x3E
#define REG_BRAKE_CFG6                  0x3F
    
#define REG_BRAKE_CFG7                  0x40
#define REG_BRAKE_CFG8                  0x41
#define REG_TRACK_CFG1                  0x44
#define REG_TRACK_CFG2                  0x45
#define REG_TRACK_CFG3                  0x46
#define REG_TRACK_F0_VAL1               0x47
#define REG_TRACK_F0_VAL2               0x48
#define REG_TIMING_CFG1                 0x49
#define REG_ADC_DATA1                   0x4A
#define REG_ADC_DATA2                   0x4B
    
#define REG_BEMF_CFG1                   0x50
#define REG_BEMF_CFG2                   0x51
#define REG_BEMF_CFG3                   0x52
#define REG_BEMF_CFG4                   0x53
#define REG_BOOST_CFG1                  0x55
#define REG_BOOST_CFG2                  0x56
#define REG_BOOST_CFG3                  0x57
#define REG_BOOST_CFG4                  0x58
#define REG_BOOST_CFG5                  0x59
#define REG_PA_CFG1                     0x5A
#define REG_PA_CFG2                     0x5B
#define REG_PMU_CFG1                    0x5C
#define REG_PMU_CFG2                    0x5D
#define REG_PMU_CFG3                    0x5E
#define REG_PMU_CFG4                    0x5F
    
#define REG_OSC_CFG1                    0x60
#define REG_OSC_CFG2                    0x61
#define REG_INT_CFG                     0x62
#define REG_GPIO_CFG1                   0x63
#define REG_GPIO_CFG2                   0x64
#define REG_GPIO_CFG3                   0x65
#define REG_EFS_WR_DATA                 0x66
#define REG_EFS_RD_DATA                 0x67
#define REG_EFS_ADDR_INDEX              0x68
#define REG_EFS_MODE_CTRL               0x69
#define REG_EFS_PGM_STROBE_WIDTH        0x6A
#define REG_EFS_READ_STROBE_WIDTH       0x6B
    
#define REG_RAM_DATA_READ               0xFF

/******************************************************************************
 * rt903x register bit detail
******************************************************************************/
// RT903X_REG_PLAY_MODE
#define BIT_PLAY_MODE_MASK           (7 << 0)
#define BIT_PLAY_MODE_RAM            (1 << 0)
#define BIT_PLAY_MODE_STREAM         (2 << 0)
#define BIT_PLAY_MODE_TRACK          (3 << 0)

// RT903X_REG_PLAY_CTRL
#define BIT_GO_MASK                  (1 << 0)
#define BIT_GO_START                 (1 << 0)
#define BIT_GO_STOP                  (0 << 0)

// RT903X_REG_INT_STATUS
#define BIT_INTS_PLAYDONE            (1 << 3)
#define BIT_INTS_FIFO_AE             (1 << 2)
#define BIT_INTS_FIFO_AF             (1 << 1)
#define BIT_INTS_PROTECTION          (1 << 0)

// RT903X_REG_EFS_MODE_CTRL
#define BIT_EFS_READ                 (1 << 1)
#define BIT_EFS_PGM                  (1 << 0)

// PMU_CFG3
#define BIT_OSC_LDO_TRIM_MASK        (3 << 6)
#define BIT_PMU_LDO_TRIM_MASK        (3 << 4)
#define BIT_PMU_VBAT_TRIM_MASK       (3 << 0)

// PMU_CFG4
#define BIT_BIAS_1P2V_TRIM_MASK      (0x0F << 4)
#define BIT_BIAS_I_TRIM_MASK         (0x0F << 0)

// BOOST_CFG3
#define BIT_BST_VOUT_TRIM_MASK       (0x0F << 4)

// OSC_CFG1
#define BIT_OSC_TRIM_MASK            (0xFF << 0)

// BEMF_CFG1
#define BIT_BEMF_STAGE_TRIM_MASK     (0X0F << 0)

/******************************************************************************
 * rt903x efuse layout
******************************************************************************/
#define EFS_BIAS_1P2V_TRIM_OFFSET       0x00
#define EFS_BIAS_I_TRIM_OFFSET          0x04
#define EFS_PMU_LDO_TRIM_OFFSET         0x08
#define EFS_OSC_LDO_TRIM_OFFSET         0x09
#define EFS_OSC_TRIM_OFFSET             0x0A
#define EFS_VBAT_DET_TRIM_OFFSET        0x12
#define EFS_RL_DET_TRIM_OFFSET          0x17
#define EFS_VERSION_OFFSET              0x1E

#define EFS_BIAS_1P2V_TRIM_MASK         0x0F
#define EFS_BIAS_I_TRIM_MASK            0xF0
#define EFS_PMU_LDO_TRIM_MASK           0x100
#define EFS_OSC_LDO_TRIM_MASK           0x200
#define EFS_OSC_TRIM_MASK               0x3FC00
#define EFS_VBAT_DET_TRIM_MASK          0x7C0000
#define EFS_RL_DET_TRIM_MASK            0x3F800000
#define EFS_VERSION_MASK                0xC0000000

#endif // __RT903X_REG_LIST_H__
