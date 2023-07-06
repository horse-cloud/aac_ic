ESP-IDF app for aac chid rt903 

we use this test rt903 performance.
i use esp32s3 
on this project, i use gpio2 for SCL , and gpio1 for SDA. if you want change it ,you can open the file Kconfig.projbuild and change the num .

config I2C_MASTER_SCL
    int "SCL GPIO Num"
    default 6 if IDF_TARGET_ESP32C3 || IDF_TARGET_ESP32C2 || IDF_TARGET_ESP32H2
    default 2 if  IDF_TARGET_ESP32S3
    default 19 if IDF_TARGET_ESP32 || IDF_TARGET_ESP32S2
    help
        GPIO number for I2C Master clock line.

config I2C_MASTER_SDA
    int "SDA GPIO Num"
    default 5 if IDF_TARGET_ESP32C3 || IDF_TARGET_ESP32C2 || IDF_TARGET_ESP32H2
    default 1 if  IDF_TARGET_ESP32S3
    default 18 if IDF_TARGET_ESP32 || IDF_TARGET_ESP32S2
    help
        GPIO number for I2C Master data line.

