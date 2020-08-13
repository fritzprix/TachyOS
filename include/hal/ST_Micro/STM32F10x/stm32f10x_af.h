#ifndef __STM32F10X_AF_H
#define __STM32F10X_AF_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


///  alternate function register of STM32F0x series has different structure from STM32F4x series 
///  to keep API unchanged AF value should be encoded as below
///  | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
///  | value |   offset for value    |
///  => value << offset
/// if value is 0, then the AFIO remap register updated by masking corresponding offset value

extern const uint32_t AF_REMAP_MASK[];


#define AF_ADC2_ETRGREG              ((uint8_t) 20)
#define AF_ADC2_ETRGINJ              ((uint8_t) 19)
#define AF_ADC1_ETRGREG              ((uint8_t) 18)
#define AF_ADC1_ETRGINJ              ((uint8_t) 17)
#define AF_TIM5CH4_POS               ((uint8_t) 16)
#define AF_PD01                      ((uint8_t) 15)
#define AF_CAN                       ((uint8_t) 13)
#define AF_TIM4                      ((uint8_t) 12)
#define AF_TIM3                      ((uint8_t) 10)
#define AF_TIM2                      ((uint8_t) 8)
#define AF_TIM1                      ((uint8_t) 6)
#define AF_USART3                    ((uint8_t) 4)
#define AF_USART2                    ((uint8_t) 3)
#define AF_USART1                    ((uint8_t) 2)
#define AF_I2C1                      ((uint8_t) 1)
#define AF_SPI1                      ((uint8_t) 0)

#define AF_OFFSET_Msk                ((uint8_t) 0x3F)


#define encode_af(v, pos)      (uint8_t ((v << 6) | (0x3F & pos)))
#define apply_af(afio_mapr, afv)   \
do {                                                   \
    afio_mapr &= ~AF_REMAP_MASK[afv  & AF_OFFSET_Msk]; \
    afio_mapr |= (afv >> 6) << (afv  & AF_OFFSET_Msk); \
} while(0)



#ifdef __cplusplus
}
#endif

#endif