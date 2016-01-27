INC-y+=$(ROOT_DIR)/include/hal/ST_Micro/STM32F40_41xxx
SRC-y+=$(ROOT_DIR)/source/hal/ST_Micro/STM32F40_41xxx
DEF-y+=STM32F40_41xxx
LDSCRIPT=$(ROOT_DIR)/source/hal/ST_Micro/STM32F40_41xxx/ld/flash.ld

OBJ-y+=startup.sko
OBJ-y+=tch_rtc.ko
OBJ-y+=tch_boot.ko
OBJ-y+=tch_gpio.ko
OBJ-y+=tch_dma.ko
OBJ-y+=tch_hal.ko
OBJ-$(CONFIG_UART)+=tch_usart.ko
OBJ-$(CONFIG_ADC)+=tch_adc.ko
OBJ-$(CONFIG_SPI)+=tch_spi.ko
OBJ-$(CONFIG_IIC)+=tch_i2c.ko
OBJ-$(CONFIG_TIMER)+=tch_timer.ko
OBJ-$(CONFIG_SDIO)+=tch_sdio.ko
