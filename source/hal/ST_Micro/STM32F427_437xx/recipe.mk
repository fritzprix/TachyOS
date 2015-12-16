LDSCRIPT=$(ROOT_DIR)/source/hal/ST_Micro/STM32F427_437xx/ld/flash.ld
INC-y+=$(ROOT_DIR)/include/hal/ST_Micro/STM32F427_437xx
SRC-y+=$(ROOT_DIR)/source/hal/ST_Micro/STM32F427_437xx

OBJ-y+=startup.sko
OBJ-y+=tch_rtc.ko
OBJ-y+=tch_boot.ko
OBJ-y+=tch_gpio.ko
OBJ-y+=tch_dma.ko
OBJ-$(UART)+=tch_uart.ko
OBJ-$(ADC)+=tch_adc.ko
OBJ-$(SPI)+=tch_spi.ko
OBJ-$(IIC)+=tch_i2c.ko
OBJ-$(TIMER)+=tch_timer.ko

y-OBJ+=tch_hal.ko
