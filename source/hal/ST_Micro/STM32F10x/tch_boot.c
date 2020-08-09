/*
 * tch_boot.c
 *
 *  Copyright (C) 2014 doowoong,lee
 *  All rights reserved.
 *
 *  This software may be modified and distributed under the terms
 *  of the LGPL v3 license.  See the LICENSE file for details.
 *
 *
 *  Created on: 2015. 8. 4.
 *      Author: innocentevil
 */


#include "tch.h"
#include "tch_halcfg.h"

#include "kernel/tch_boot.h"
#include "kernel/tch_err.h"



/* PLL_VCO = (HSE_VALUE or HSI_VALUE / PLL_M) * PLL_N */
#define PLL_M      (SYS_MCLK_FREQ / 1000000)
#define PLL_N      240

/* SYSCLK = PLL_VCO / PLL_P */
#define PLL_P      2

/* USB OTG FS, SDIO and RNG Clock =  PLL_VCO / PLLQ */
#define PLL_Q      5

/**
  * @}
  */

/** @addtogroup STM32F2xx_System_Private_Macros
  * @{
  */

/**
  * @}
  */

/** @addtogroup STM32F2xx_System_Private_Variables
  * @{
  */

  uint32_t SystemCoreClock = SYS_CLK;

  __I uint8_t AHBPrescTable[16] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 6, 7, 8, 9};

/**
  * @}
  */

/** @addtogroup STM32F2xx_System_Private_FunctionPrototypes
  * @{
  */


void tch_boot_setSystemClock(){


	  /* Reset the RCC clock configuration to the default reset state ------------*/
	/* Set HSION bit */
	RCC->CR |= (uint32_t) 0x00000001;

	/* Reset CFGR register */
	RCC->CFGR = 0x00000000;

	/* Reset HSEON, CSSON and PLLON bits */
	RCC->CR &= (uint32_t) 0xFEF6FFFF;

	/* Reset PLLCFGR register */
	RCC->PLLCFGR = 0x24003010;

	/* Reset HSEBYP bit */
	RCC->CR &= (uint32_t) 0xFFFBFFFF;

	/* Disable all interrupts */
	RCC->CIR = 0x00000000;

	/******************************************************************************/
	/*            PLL (clocked by HSE) used as System clock source                */
	/******************************************************************************/
	__IO uint32_t StartUpCounter = 0, HSEStatus = 0;

	/* Enable HSE */
	RCC->CR |= ((uint32_t) RCC_CR_HSEON);

	/* Wait till HSE is ready and if Time out is reached exit */
	do {
		HSEStatus = RCC->CR & RCC_CR_HSERDY;
		StartUpCounter++;
	} while ((HSEStatus == 0) && (StartUpCounter != HSE_STARTUP_TIMEOUT));

	if ((RCC->CR & RCC_CR_HSERDY) != RESET) {
		HSEStatus = (uint32_t) 0x01;
	} else {
		HSEStatus = (uint32_t) 0x00;
	}

	if (HSEStatus == (uint32_t) 0x01) {
		/* HCLK = SYSCLK / 1*/
		RCC->CFGR |= RCC_CFGR_HPRE_DIV1;

		/* PCLK2 = HCLK / 2*/
		RCC->CFGR |= RCC_CFGR_PPRE2_DIV2;

		/* PCLK1 = HCLK / 4*/
		RCC->CFGR |= RCC_CFGR_PPRE1_DIV4;

		/* Configure the main PLL */
		RCC->PLLCFGR = PLL_M | (PLL_N << 6) | (((PLL_P >> 1) - 1) << 16)
				| (RCC_PLLCFGR_PLLSRC_HSE) | (PLL_Q << 24);

		/* Enable the main PLL */
		RCC->CR |= RCC_CR_PLLON;

		/* Wait till the main PLL is ready */
		while ((RCC->CR & RCC_CR_PLLRDY) == 0) {
		}

		/* Configure Flash prefetch, Instruction cache, Data cache and wait state */
		FLASH->ACR = FLASH_ACR_PRFTEN | FLASH_ACR_ICEN | FLASH_ACR_DCEN
				| FLASH_ACR_LATENCY_3WS;

		/* Select the main PLL as system clock source */
		RCC->CFGR &= (uint32_t) ((uint32_t) ~(RCC_CFGR_SW));
		RCC->CFGR |= RCC_CFGR_SW_PLL;

		/* Wait till the main PLL is used as system clock source */
		while ((RCC->CFGR & (uint32_t) RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL)
		{
			__NOP();
		}
	} else { /* If HSE fails to start-up, the application will have wrong clock
	 configuration. User can add here some code to deal with this error */
		KERNEL_PANIC("clock init failed");
	}

}

void tch_boot_setSystemMem(){
	/*-- GPIOs Configuration -----------------------------------------------------*/
	/*
	 +-------------------+--------------------+------------------+------------------+
	 +                       SRAM pins assignment                                  +
	 +-------------------+--------------------+------------------+------------------+
	 | PD0  <-> FSMC_D2  | PE0  <-> FSMC_NBL0 | PF0  <-> FSMC_A0 | PG0 <-> FSMC_A10 |
	 | PD1  <-> FSMC_D3  | PE1  <-> FSMC_NBL1 | PF1  <-> FSMC_A1 | PG1 <-> FSMC_A11 |
	 | PD4  <-> FSMC_NOE | PE7  <-> FSMC_D4   | PF2  <-> FSMC_A2 | PG2 <-> FSMC_A12 |
	 | PD5  <-> FSMC_NWE | PE8  <-> FSMC_D5   | PF3  <-> FSMC_A3 | PG3 <-> FSMC_A13 |
	 | PD8  <-> FSMC_D13 | PE9  <-> FSMC_D6   | PF4  <-> FSMC_A4 | PG4 <-> FSMC_A14 |
	 | PD9  <-> FSMC_D14 | PE10 <-> FSMC_D7   | PF5  <-> FSMC_A5 | PG5 <-> FSMC_A15 |
	 | PD10 <-> FSMC_D15 | PE11 <-> FSMC_D8   | PF12 <-> FSMC_A6 | PG9 <-> FSMC_NE2 |
	 | PD11 <-> FSMC_A16 | PE12 <-> FSMC_D9   | PF13 <-> FSMC_A7 |------------------+
	 | PD12 <-> FSMC_A17 | PE13 <-> FSMC_D10  | PF14 <-> FSMC_A8 |
	 | PD14 <-> FSMC_D0  | PE14 <-> FSMC_D11  | PF15 <-> FSMC_A9 |
	 | PD15 <-> FSMC_D1  | PE15 <-> FSMC_D12  |------------------+
	 +-------------------+--------------------+
	*/
	   /* Enable GPIOD, GPIOE, GPIOF and GPIOG interface clock */
	  RCC->AHB1ENR   = 0x00000078;

	  /* Connect PDx pins to FSMC Alternate function */
	  GPIOD->AFR[0]  = 0x00cc00cc;
	  GPIOD->AFR[1]  = 0xcc0ccccc;
	  /* Configure PDx pins in Alternate function mode */
	  GPIOD->MODER   = 0xa2aa0a0a;
	  /* Configure PDx pins speed to 100 MHz */
	  GPIOD->OSPEEDR = 0xf3ff0f0f;
	  /* Configure PDx pins Output type to push-pull */
	  GPIOD->OTYPER  = 0x00000000;
	  /* No pull-up, pull-down for PDx pins */
	  GPIOD->PUPDR   = 0x00000000;

	  /* Connect PEx pins to FSMC Alternate function */
	  GPIOE->AFR[0]  = 0xc00000cc;
	  GPIOE->AFR[1]  = 0xcccccccc;
	  /* Configure PEx pins in Alternate function mode */
	  GPIOE->MODER   = 0xaaaa800a;
	  /* Configure PEx pins speed to 100 MHz */
	  GPIOE->OSPEEDR = 0xffffc00f;
	  /* Configure PEx pins Output type to push-pull */
	  GPIOE->OTYPER  = 0x00000000;
	  /* No pull-up, pull-down for PEx pins */
	  GPIOE->PUPDR   = 0x00000000;

	  /* Connect PFx pins to FSMC Alternate function */
	  GPIOF->AFR[0]  = 0x00cccccc;
	  GPIOF->AFR[1]  = 0xcccc0000;
	  /* Configure PFx pins in Alternate function mode */
	  GPIOF->MODER   = 0xaa000aaa;
	  /* Configure PFx pins speed to 100 MHz */
	  GPIOF->OSPEEDR = 0xff000fff;
	  /* Configure PFx pins Output type to push-pull */
	  GPIOF->OTYPER  = 0x00000000;
	  /* No pull-up, pull-down for PFx pins */
	  GPIOF->PUPDR   = 0x00000000;

	  /* Connect PGx pins to FSMC Alternate function */
	  GPIOG->AFR[0]  = 0x00cccccc;
	  GPIOG->AFR[1]  = 0x000000c0;
	  /* Configure PGx pins in Alternate function mode */
	  GPIOG->MODER   = 0x00080aaa;
	  /* Configure PGx pins speed to 100 MHz */
	  GPIOG->OSPEEDR = 0x000c0fff;
	  /* Configure PGx pins Output type to push-pull */
	  GPIOG->OTYPER  = 0x00000000;
	  /* No pull-up, pull-down for PGx pins */
	  GPIOG->PUPDR   = 0x00000000;

	/*-- FSMC Configuration ------------------------------------------------------*/
	  /* Enable the FSMC interface clock */
	  RCC->AHB3ENR         = 0x00000001;

	  /* Configure and enable Bank1_SRAM2 */
	  FSMC_Bank1->BTCR[2]  = 0x00001015;
	  FSMC_Bank1->BTCR[3]  = 0x00010400;
	  FSMC_Bank1E->BWTR[2] = 0x0fffffff;
	/*
	  Bank1_SRAM2 is configured as follow:

	  p.FSMC_AddressSetupTime = 0;
	  p.FSMC_AddressHoldTime = 0;
	  p.FSMC_DataSetupTime = 4;
	  p.FSMC_BusTurnAroundDuration = 1;
	  p.FSMC_CLKDivision = 0;
	  p.FSMC_DataLatency = 0;
	  p.FSMC_AccessMode = FSMC_AccessMode_A;

	  FSMC_NORSRAMInitStructure.FSMC_Bank = FSMC_Bank1_NORSRAM2;
	  FSMC_NORSRAMInitStructure.FSMC_DataAddressMux = FSMC_DataAddressMux_Disable;
	  FSMC_NORSRAMInitStructure.FSMC_MemoryType = FSMC_MemoryType_PSRAM;
	  FSMC_NORSRAMInitStructure.FSMC_MemoryDataWidth = FSMC_MemoryDataWidth_16b;
	  FSMC_NORSRAMInitStructure.FSMC_BurstAccessMode = FSMC_BurstAccessMode_Disable;
	  FSMC_NORSRAMInitStructure.FSMC_AsynchronousWait = FSMC_AsynchronousWait_Disable;
	  FSMC_NORSRAMInitStructure.FSMC_WaitSignalPolarity = FSMC_WaitSignalPolarity_Low;
	  FSMC_NORSRAMInitStructure.FSMC_WrapMode = FSMC_WrapMode_Disable;
	  FSMC_NORSRAMInitStructure.FSMC_WaitSignalActive = FSMC_WaitSignalActive_BeforeWaitState;
	  FSMC_NORSRAMInitStructure.FSMC_WriteOperation = FSMC_WriteOperation_Enable;
	  FSMC_NORSRAMInitStructure.FSMC_WaitSignal = FSMC_WaitSignal_Disable;
	  FSMC_NORSRAMInitStructure.FSMC_ExtendedMode = FSMC_ExtendedMode_Disable;
	  FSMC_NORSRAMInitStructure.FSMC_WriteBurst = FSMC_WriteBurst_Disable;
	  FSMC_NORSRAMInitStructure.FSMC_ReadWriteTimingStruct = &p;
	  FSMC_NORSRAMInitStructure.FSMC_WriteTimingStruct = &p;
	*/
}
