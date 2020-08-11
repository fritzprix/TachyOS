/*
 * tch_boot.c
 *
 *  Created on: 2015. 8. 4.
 *      Author: innocentevil
 */

#include "tch.h"
#include "tch_halcfg.h"

#include "kernel/tch_boot.h"
#include "kernel/tch_err.h"

/************************* PLL Parameters *************************************/
#if defined(STM32F40_41xxx) || defined(STM32F427_437xx) || defined(STM32F429_439xx) || defined(STM32F401xx) || defined(STM32F469_479xx)
/* PLL_VCO = (HSE_VALUE or HSI_VALUE / PLL_M) * PLL_N */
#define PLL_M 25
#elif defined(STM32F412xG) || defined(STM32F413_423xx) || defined(STM32F446xx)
#define PLL_M 8
#elif defined(STM32F410xx) || defined(STM32F411xE)
#if defined(USE_HSE_BYPASS)
#define PLL_M 8
#else /* !USE_HSE_BYPASS */
#define PLL_M 16
#endif /* USE_HSE_BYPASS */
#else
#endif /* STM32F40_41xxx || STM32F427_437xx || STM32F429_439xx || STM32F401xx || STM32F469_479xx */

/* USB OTG FS, SDIO and RNG Clock =  PLL_VCO / PLLQ */
#define PLL_Q 7

#if defined(STM32F446xx)
/* PLL division factor for I2S, SAI, SYSTEM and SPDIF: Clock =  PLL_VCO / PLLR */
#define PLL_R 7
#elif defined(STM32F412xG) || defined(STM32F413_423xx)
#define PLL_R 2
#else
#endif /* STM32F446xx */

#if defined(STM32F427_437xx) || defined(STM32F429_439xx) || defined(STM32F446xx) || defined(STM32F469_479xx)
#define PLL_N 360
/* SYSCLK = PLL_VCO / PLL_P */
#define PLL_P 2
#endif /* STM32F427_437x || STM32F429_439xx || STM32F446xx || STM32F469_479xx */

#if defined(STM32F40_41xxx)
#define PLL_N 336
/* SYSCLK = PLL_VCO / PLL_P */
#define PLL_P 2
#endif /* STM32F40_41xxx */

#if defined(STM32F401xx)
#define PLL_N 336
/* SYSCLK = PLL_VCO / PLL_P */
#define PLL_P 4
#endif /* STM32F401xx */

#if defined(STM32F410xx) || defined(STM32F411xE) || defined(STM32F412xG) || defined(STM32F413_423xx)
#define PLL_N 400
/* SYSCLK = PLL_VCO / PLL_P */
#define PLL_P 4
#endif /* STM32F410xx || STM32F411xE || STM32F412xG || STM32F413_423xx */

/******************************************************************************/

#if defined(STM32F40_41xxx)
uint32_t SystemCoreClock = 168000000;
#endif /* STM32F40_41xxx */

#if defined(STM32F427_437xx) || defined(STM32F429_439xx)
uint32_t SystemCoreClock = 180000000;
#endif /* STM32F427_437x || STM32F429_439xx */

#if defined(STM32F401xx)
uint32_t SystemCoreClock = 84000000;
#endif /* STM32F401xx */

__I uint8_t AHBPrescTable[16] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 6, 7, 8, 9};

void tch_boot_setSystemClock()
{
/* FPU settings ------------------------------------------------------------*/
#if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
	SCB->CPACR |= ((3UL << 10 * 2) | (3UL << 11 * 2)); /* set CP10 and CP11 Full Access */
#endif
	/* Reset the RCC clock configuration to the default reset state ------------*/
	/* Set HSION bit */
	RCC->CR |= (uint32_t)0x00000001;

	/* Reset CFGR register */
	RCC->CFGR = 0x00000000;

	/* Reset HSEON, CSSON and PLLON bits */
	RCC->CR &= (uint32_t)0xFEF6FFFF;

	/* Reset PLLCFGR register */
	RCC->PLLCFGR = 0x24003010;

	/* Reset HSEBYP bit */
	RCC->CR &= (uint32_t)0xFFFBFFFF;

	/* Disable all interrupts */
	RCC->CIR = 0x00000000;

/******************************************************************************/
/*            PLL (clocked by HSE) used as System clock source                */
/******************************************************************************/
#if SYS_MCLK_TYPE == SYS_MCLK_TYPE_CRYSTAL || SYS_MCLK_TYPE == SYS_MCLK_TYPE_EXTOSC
	__IO uint32_t StartUpCounter = 0, HSEStatus = 0, HSIStatus = 0;

#if SYS_MCLK_TYPE == SYS_MCLK_TYPE_CRYSTAL
	/* Enable HSE */
	RCC->CR |= ((uint32_t)RCC_CR_HSEON);
#elif SYS_MCLK_TYPE == SYS_MCLK_TYPE_EXTOSC
	RCC->CR |= ((uint32_t)(RCC_CR_HSEON | RCC_CR_HSEBYP));
#endif

	/* Wait till HSE is ready and if Time out is reached exit */
	do
	{
		HSEStatus = RCC->CR & RCC_CR_HSERDY;
		StartUpCounter++;
	} while ((HSEStatus == 0) && (StartUpCounter != HSE_STARTUP_TIMEOUT));

	if (RCC->CR & RCC_CR_HSERDY)
	{
		HSEStatus = (uint32_t)0x01;
	}
	else
	{
		HSEStatus = (uint32_t)0x00;
	}
#else
	/** internal high speed clock **/
	uint32_t HSIStatus = 0;
	RCC->CR |= RCC_CR_HSION;
	do
	{
		HSIStatus = RCC->CR & RCC_CR_HSIRDY;
		StartUpCounter++;
	} while ((HSIStatus == 0) && (StartUpCounter != HSE_STARTUP_TIMEOUT));

	if ((RCC->CR & RCC_CR_HSIRDY) != RESET)
	{
		HSEStatus = (uint32_t)0x01;
	}
	else
	{
		HSEStatus = (uint32_t)0x00;
	}
#endif

	if (HSEStatus == (uint32_t)0x01)
	{
		/* Select regulator voltage output Scale 1 mode */
		RCC->APB1ENR |= RCC_APB1ENR_PWREN;
		PWR->CR |= PWR_CR_VOS;

		/* HCLK = SYSCLK / 1*/
		RCC->CFGR |= RCC_CFGR_HPRE_DIV1;

		/* PCLK2 = HCLK / 2*/
		RCC->CFGR |= RCC_CFGR_PPRE2_DIV1;

		/* PCLK1 = HCLK / 4*/
		RCC->CFGR |= RCC_CFGR_PPRE1_DIV2;

		/* Configure the main PLL */
		RCC->PLLCFGR = PLL_M | (PLL_N << 6) | (((PLL_P >> 1) - 1) << 16) |
					   (RCC_PLLCFGR_PLLSRC_HSE) | (PLL_Q << 24);

		/* Enable the main PLL */
		RCC->CR |= RCC_CR_PLLON;

		/* Wait till the main PLL is ready */
		while ((RCC->CR & RCC_CR_PLLRDY) == 0)
		{
		}

		/* Configure Flash prefetch, Instruction cache, Data cache and wait state */
		FLASH->ACR = FLASH_ACR_PRFTEN | FLASH_ACR_ICEN | FLASH_ACR_DCEN | FLASH_ACR_LATENCY_2WS;

		/* Select the main PLL as system clock source */
		RCC->CFGR &= (uint32_t)((uint32_t) ~(RCC_CFGR_SW));
		RCC->CFGR |= RCC_CFGR_SW_PLL;

		/* Wait till the main PLL is used as system clock source */
		while ((RCC->CFGR & (uint32_t)RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL)
			;
		{
		}
	}
	else
	{ /* If HSE fails to start-up, the application will have wrong clock
	           configuration. User can add here some code to deal with this error */
		KERNEL_PANIC("clock init failed");
	}
}

void tch_boot_setSystemMem()
{
#if defined(DATA_IN_ExtSRAM) && defined(DATA_IN_ExtSDRAM)
#if defined(STM32F427xx) || defined(STM32F437xx) || defined(STM32F429xx) || defined(STM32F439xx) || \
	defined(STM32F469xx) || defined(STM32F479xx)
	__IO uint32_t tmp = 0x00;

	register uint32_t tmpreg = 0, timeout = 0xFFFF;
	register uint32_t index;

	/* Enable GPIOC, GPIOD, GPIOE, GPIOF, GPIOG, GPIOH and GPIOI interface clock */
	RCC->AHB1ENR |= 0x000001F8;

	/* Delay after an RCC peripheral clock enabling */
	tmp = READ_BIT(RCC->AHB1ENR, RCC_AHB1ENR_GPIOCEN);

	/* Connect PDx pins to FMC Alternate function */
	GPIOD->AFR[0] = 0x00CCC0CC;
	GPIOD->AFR[1] = 0xCCCCCCCC;
	/* Configure PDx pins in Alternate function mode */
	GPIOD->MODER = 0xAAAA0A8A;
	/* Configure PDx pins speed to 100 MHz */
	GPIOD->OSPEEDR = 0xFFFF0FCF;
	/* Configure PDx pins Output type to push-pull */
	GPIOD->OTYPER = 0x00000000;
	/* No pull-up, pull-down for PDx pins */
	GPIOD->PUPDR = 0x00000000;

	/* Connect PEx pins to FMC Alternate function */
	GPIOE->AFR[0] = 0xC00CC0CC;
	GPIOE->AFR[1] = 0xCCCCCCCC;
	/* Configure PEx pins in Alternate function mode */
	GPIOE->MODER = 0xAAAA828A;
	/* Configure PEx pins speed to 100 MHz */
	GPIOE->OSPEEDR = 0xFFFFC3CF;
	/* Configure PEx pins Output type to push-pull */
	GPIOE->OTYPER = 0x00000000;
	/* No pull-up, pull-down for PEx pins */
	GPIOE->PUPDR = 0x00000000;

	/* Connect PFx pins to FMC Alternate function */
	GPIOF->AFR[0] = 0xCCCCCCCC;
	GPIOF->AFR[1] = 0xCCCCCCCC;
	/* Configure PFx pins in Alternate function mode */
	GPIOF->MODER = 0xAA800AAA;
	/* Configure PFx pins speed to 50 MHz */
	GPIOF->OSPEEDR = 0xAA800AAA;
	/* Configure PFx pins Output type to push-pull */
	GPIOF->OTYPER = 0x00000000;
	/* No pull-up, pull-down for PFx pins */
	GPIOF->PUPDR = 0x00000000;

	/* Connect PGx pins to FMC Alternate function */
	GPIOG->AFR[0] = 0xCCCCCCCC;
	GPIOG->AFR[1] = 0xCCCCCCCC;
	/* Configure PGx pins in Alternate function mode */
	GPIOG->MODER = 0xAAAAAAAA;
	/* Configure PGx pins speed to 50 MHz */
	GPIOG->OSPEEDR = 0xAAAAAAAA;
	/* Configure PGx pins Output type to push-pull */
	GPIOG->OTYPER = 0x00000000;
	/* No pull-up, pull-down for PGx pins */
	GPIOG->PUPDR = 0x00000000;

	/* Connect PHx pins to FMC Alternate function */
	GPIOH->AFR[0] = 0x00C0CC00;
	GPIOH->AFR[1] = 0xCCCCCCCC;
	/* Configure PHx pins in Alternate function mode */
	GPIOH->MODER = 0xAAAA08A0;
	/* Configure PHx pins speed to 50 MHz */
	GPIOH->OSPEEDR = 0xAAAA08A0;
	/* Configure PHx pins Output type to push-pull */
	GPIOH->OTYPER = 0x00000000;
	/* No pull-up, pull-down for PHx pins */
	GPIOH->PUPDR = 0x00000000;

	/* Connect PIx pins to FMC Alternate function */
	GPIOI->AFR[0] = 0xCCCCCCCC;
	GPIOI->AFR[1] = 0x00000CC0;
	/* Configure PIx pins in Alternate function mode */
	GPIOI->MODER = 0x0028AAAA;
	/* Configure PIx pins speed to 50 MHz */
	GPIOI->OSPEEDR = 0x0028AAAA;
	/* Configure PIx pins Output type to push-pull */
	GPIOI->OTYPER = 0x00000000;
	/* No pull-up, pull-down for PIx pins */
	GPIOI->PUPDR = 0x00000000;

	/*-- FMC Configuration -------------------------------------------------------*/
	/* Enable the FMC interface clock */
	RCC->AHB3ENR |= 0x00000001;
	/* Delay after an RCC peripheral clock enabling */
	tmp = READ_BIT(RCC->AHB3ENR, RCC_AHB3ENR_FMCEN);

	FMC_Bank5_6->SDCR[0] = 0x000019E4;
	FMC_Bank5_6->SDTR[0] = 0x01115351;

	/* SDRAM initialization sequence */
	/* Clock enable command */
	FMC_Bank5_6->SDCMR = 0x00000011;
	tmpreg = FMC_Bank5_6->SDSR & 0x00000020;
	while ((tmpreg != 0) && (timeout-- > 0))
	{
		tmpreg = FMC_Bank5_6->SDSR & 0x00000020;
	}

	/* Delay */
	for (index = 0; index < 1000; index++)
		;

	/* PALL command */
	FMC_Bank5_6->SDCMR = 0x00000012;
	timeout = 0xFFFF;
	while ((tmpreg != 0) && (timeout-- > 0))
	{
		tmpreg = FMC_Bank5_6->SDSR & 0x00000020;
	}

	/* Auto refresh command */
	FMC_Bank5_6->SDCMR = 0x00000073;
	timeout = 0xFFFF;
	while ((tmpreg != 0) && (timeout-- > 0))
	{
		tmpreg = FMC_Bank5_6->SDSR & 0x00000020;
	}

	/* MRD register program */
	FMC_Bank5_6->SDCMR = 0x00046014;
	timeout = 0xFFFF;
	while ((tmpreg != 0) && (timeout-- > 0))
	{
		tmpreg = FMC_Bank5_6->SDSR & 0x00000020;
	}

	/* Set refresh count */
	tmpreg = FMC_Bank5_6->SDRTR;
	FMC_Bank5_6->SDRTR = (tmpreg | (0x0000027C << 1));

	/* Disable write protection */
	tmpreg = FMC_Bank5_6->SDCR[0];
	FMC_Bank5_6->SDCR[0] = (tmpreg & 0xFFFFFDFF);

#if defined(STM32F427xx) || defined(STM32F437xx) || defined(STM32F429xx) || defined(STM32F439xx)
	/* Configure and enable Bank1_SRAM2 */
	FMC_Bank1->BTCR[2] = 0x00001011;
	FMC_Bank1->BTCR[3] = 0x00000201;
	FMC_Bank1E->BWTR[2] = 0x0fffffff;
#endif /* STM32F427xx || STM32F437xx || STM32F429xx || STM32F439xx */
#if defined(STM32F469xx) || defined(STM32F479xx)
	/* Configure and enable Bank1_SRAM2 */
	FMC_Bank1->BTCR[2] = 0x00001091;
	FMC_Bank1->BTCR[3] = 0x00110212;
	FMC_Bank1E->BWTR[2] = 0x0fffffff;
#endif /* STM32F469xx || STM32F479xx */

	(void)(tmp);
#endif
#endif
}
