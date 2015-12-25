LDSCRIPT=$(ROOT_DIR)/source/hal/ST_Micro/STM32F427_437xx/ld/flash.ld
INC-y+=$(ROOT_DIR)/include/hal/ST_Micro/STM32F427_437xx
SRC-y+=$(ROOT_DIR)/source/hal/ST_Micro/STM32F427_437xx

OBJ-y+=startup.sko
OBJ-y+=tch_rtc.ko
OBJ-y+=tch_boot.ko
OBJ-y+=tch_gpio.ko
OBJ-y+=tch_dma.ko
OBJ-$(CONFIG_UART)+=tch_uart.ko
OBJ-$(CONFIG_ADC)+=tch_adc.ko
OBJ-$(CONFIG_SPI)+=tch_spi.ko
OBJ-$(CONFIG_IIC)+=tch_i2c.ko
OBJ-$(CONFIG_TIMER)+=tch_timer.ko

OBJ-y+=tch_hal.ko
