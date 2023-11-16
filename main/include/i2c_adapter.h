#ifndef __I2C_ADAPTER_H__
#define __I2C_ADAPTER_H__

#include <stdint.h>
#include <esp_err.h>
#include "driver/gpio.h"

/* Definition for I2Cx clock resources */
#define I2Cx                            I2C4
#define I2Cx_CLK_ENABLE()               __HAL_RCC_I2C4_CLK_ENABLE()
#define I2Cx_SDA_GPIO_CLK_ENABLE()      __HAL_RCC_GPIOD_CLK_ENABLE()
#define I2Cx_SCL_GPIO_CLK_ENABLE()      __HAL_RCC_GPIOD_CLK_ENABLE()
#define I2Cx_DMA_CLK_ENABLE()           __HAL_RCC_BDMA_CLK_ENABLE()

#define I2Cx_FORCE_RESET()              __HAL_RCC_I2C4_FORCE_RESET()
#define I2Cx_RELEASE_RESET()            __HAL_RCC_I2C4_RELEASE_RESET()

/* Definition for I2Cx Pins */
#define I2Cx_SCL_PIN                    GPIO_PIN_12
#define I2Cx_SCL_GPIO_PORT              GPIOD
#define I2Cx_SDA_PIN                    GPIO_PIN_13
#define I2Cx_SDA_GPIO_PORT              GPIOD
#define I2Cx_SCL_SDA_AF                 GPIO_AF4_I2C4

/* Definition for I2Cx's NVIC */
#define I2Cx_EV_IRQn                    I2C4_EV_IRQn
#define I2Cx_ER_IRQn                    I2C4_ER_IRQn
#define I2Cx_EV_IRQHandler              I2C4_EV_IRQHandler
#define I2Cx_ER_IRQHandler              I2C4_ER_IRQHandler

/* Definition for I2Cx's DMA */
#define I2Cx_DMA                        BDMA   
#define I2Cx_DMA_INSTANCE_TX            BDMA_Channel1
#define I2Cx_DMA_INSTANCE_RX            BDMA_Channel0
#define I2Cx_DMA_REQUEST_TX             BDMA_REQUEST_I2C4_TX
#define I2Cx_DMA_REQUEST_RX             BDMA_REQUEST_I2C4_RX

/* Definition for I2Cx's DMA NVIC */
#define I2Cx_DMA_TX_IRQn                BDMA_Channel1_IRQn
#define I2Cx_DMA_RX_IRQn                BDMA_Channel0_IRQn
#define I2Cx_DMA_TX_IRQHandler          BDMA_Channel1_IRQHandler
#define I2Cx_DMA_RX_IRQHandler          BDMA_Channel0_IRQHandler


#define WRITE_BIT I2C_MASTER_WRITE  /*!< I2C master write */
#define READ_BIT I2C_MASTER_READ    /*!< I2C master read */
#define ACK_CHECK_EN 0x1            /*!< I2C master will check ack from slave*/
#define ACK_CHECK_DIS 0x0           /*!< I2C master will not check ack from slave */
#define ACK_VAL 0x0                 /*!< I2C ack value */
#define NACK_VAL 0x1                /*!< I2C nack value */

//
#define I2C_MASTER_NUM0              0          /*!< I2C master i2c port number, the number of i2c peripheral interfaces available will depend on the chip */
#define I2C_MASTER_NUM1              1          /*!< I2C master i2c port number, the number of i2c peripheral interfaces available will depend on the chip */
#define I2C_MASTER_0_SCL_IO           GPIO_NUM_38//CONFIG_I2C_MASTER_0_SCL  /*!< GPIO number used for I2C master clock */
#define I2C_MASTER_0_SDA_IO           GPIO_NUM_37//CONFIG_I2C_MASTER_0_SDA  /*!< GPIO number used for I2C master data  */
#define I2C_MASTER_1_SCL_IO           GPIO_NUM_36//CONFIG_I2C_MASTER_1_SCL  /*!< GPIO number used for I2C master clock */
#define I2C_MASTER_1_SDA_IO           GPIO_NUM_35//CONFIG_I2C_MASTER_1_SDA  /*!< GPIO number used for I2C master data  */
#define I2C_MASTER_NUM              0                          
#define I2C_MASTER_FREQ_HZ          400000                     /*!< I2C master clock frequency */
#define I2C_MASTER_TX_BUF_DISABLE   0                          /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE   0                          /*!< I2C master doesn't need buffer */
#define I2C_MASTER_TIMEOUT_MS       1000
#define I2C_0_ADDRESS			(0x5F)
#define I2C_1_ADDRESS			(0x5E)
//#define I2C_TIMING    0x00401040//0x00901954
//#define I2C_TIMING    0x20301850//300k
#define I2C_TIMING    0x00401032 //1m
//#define I2C_TIMING	0x40504e75
#define MAX_I2C_BUFFER_SIZE		0x1000

#define I2C_MASTER_SCL_IO           CONFIG_I2C_MASTER_SCL//GPIO_NUM_2//7//      /*!< GPIO number used for I2C master clock */
#define I2C_MASTER_SDA_IO           CONFIG_I2C_MASTER_SDA//GPIO_NUM_1//6//      /*!< GPIO number used for I2C master data  */
#define I2C_MASTER_NUM              0                          /*!< I2C master i2c port number, the number of i2c peripheral interfaces available will depend on the chip */
#define I2C_MASTER_TX_BUF_DISABLE   0                          /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE   0                          /*!< I2C master doesn't need buffer */
#define I2C_MASTER_TIMEOUT_MS       1000

#define INPUT_INT1    GPIO_NUM_1
#define INPUT_INT2    GPIO_NUM_2
#define INPUT_INT3    GPIO_NUM_6
#define INPUT_INT4    GPIO_NUM_45
#define INPUT_INT5    GPIO_NUM_48
#define INPUT_INT6    GPIO_NUM_47
#define INPUT_INT7    GPIO_NUM_21
#define INPUT_INT8    GPIO_NUM_14
enum INT_NUMBER {
    INT1,
    INT2,
    INT3,
    INT4,
    INT5,
    INT6,
    INT7,
    INT8,
    INT_DATA_MAX
};

#define SMART_SURFACE_SWITCH1    GPIO_NUM_18
#define SMART_SURFACE_SWITCH2    GPIO_NUM_8
#define SMART_SURFACE_SWITCH3    GPIO_NUM_3
#define SMART_SURFACE_SWITCH4    GPIO_NUM_46
#define SMART_SURFACE_SWITCH5    GPIO_NUM_9
enum SWITCH_NUMBER {
    SWITCH1,
    SWITCH2,
    SWITCH3,
    SWITCH4,
    SWITCH5,
    SWITCH_MAX,
};

typedef struct {
    uint8_t i2c_master_num;
    uint8_t i2c_master_sda_io;
    uint8_t i2c_master_scl_io; 
    uint32_t i2c_master_freq_hz;
} def_i2c_config_t;


esp_err_t i2c_master_init(def_i2c_config_t i2cConfig);
int16_t I2CWriteReg(uint8_t i2c_master_num, uint16_t devAddr, uint16_t WriteAddr, uint8_t *data_wr, uint16_t length);
int16_t I2CReadReg(uint8_t i2c_master_num, uint16_t devAddr, uint16_t ReadAddr, uint8_t *data_wr, uint16_t len);

#endif	// __I2C_ADAPTER_H__
