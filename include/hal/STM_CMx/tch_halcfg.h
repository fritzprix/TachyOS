/*
 * tch_halcfg.h
 *
 *  Created on: 2014. 6. 15.
 *      Author: innocentevil
 */

#ifndef TCH_HALCFG_H_
#define TCH_HALCFG_H_



#define TACHYOS_HAL_VENDOR               "ST_Micro"                      // vendor name field
#define TACHYOS_HAL_PLATFORM_NAME        "STM32F4xx"                     // hw platform name (product name of mcu / mpu ic)
#define TACHYOS_HAL_PLATFORM_SPECIFIER    1                              // hw platform specifier (mapped to sub product number ex > stm32f407zg , stm32f417...)
#define TACHYOS_HAL_MAIN_VERSION          0
#define TACHYOS_HAR_SUB_VERSION           0





#define SYS_MCLK_TYPE_CRYSTAL     0
#define SYS_MCLK_TYPE_EXTOSC      1
#define SYS_MCLK_TYPE_INTERNAL    2



#ifndef USE_EXTMEM
#define USE_EXTMEM                0
#endif

#define EXTMEM_TYPE_SDRAM         0
#define EXTMEM_TYPE_SRAM          1




#if (USE_EXTMEM == 1)
#ifndef EXTMEM_TYPE
 #error "Please define 'EXTMEM_TYPE' "
#endif
#endif

/***
 *  configure clock source type of target system  : default external crystal
 */
#ifndef SYS_MCLK_TYPE
#define SYS_MCLK_TYPE            SYS_MCLK_TYPE_CRYSTAL
#endif




/***
 *  configure clock source frequency
 */
#ifndef SYS_INTERNAL_CLK_FREQ
#define SYS_INTERNAL_CLK_FREQ      16000000
#endif


#ifndef SYS_MCLK_FREQ
#define SYS_MCLK_FREQ              8000000    ///default system main clock  : 8MHz
#endif



#define SYS_TARGET_CLK_FASTEST     ((uint32_t) 1)
#define SYS_TARGET_CLK_FAIR        ((uint32_t) 2)
#define SYS_TARGET_CLK_LOW         ((uint32_t) 4)


/****
 *    config system clock speed
 *     - Fastest  :  possible Highest Clock is set as system clock
 *                   typically it's more power consuming
 *     - fair     :  1/2 of highest clock
 *     - low      :  1/4 of highest clock
 */
#ifndef SYS_TARGET_CLK_SPEED
#define SYS_TARGET_CLK_SPEED       SYS_TARGET_CLK_FASTEST
#endif

#ifndef SYS_SLEEPCLK_ENABLE
#define SYS_SLEEPCLK                0                   /// default system sleep clock disable
#endif



/***
 *  configure stm32f4x vendor library header macro
 */


#if (SYS_MCLK_TYPE != SYS_MCLK_TYPE_INTERNAL)
#define HSE_VALUE                SYS_MCLK_FREQ
#else
#define HSE_VALUE                SYS_INTERNAL_CLK_FREQ
#endif







#endif /* TCH_HALCFG_H_ */
