ESP-IDF app for aac chid rt903 

本人使用esp32s3主板
注意事项汇总清单：
1、SCL 是GPIO2, SDA 是GPIO1 ,  rt903 的AD脚 接GPIO48（需拉高），RT903 中断脚接GPIO5
2、5vin的口似乎会受GPIO配置影响，demo状态是高阻态（电压0.9v左右）rt903从5vin接电可能不稳定，建议用usb口供电，rt903从板子上的5v取电。同时需注意，如果使用充电宝，注意充电宝低电流自动关闭，以及小电流模式几个小时后会关闭
3、GPIO拉低触发振动，GPIO6对应I2C_0_0X5E , GPIO7 对应I2C_0_0X5F