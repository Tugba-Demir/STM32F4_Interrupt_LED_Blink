#include "main.h"

void SystemClock_Config(void);

void SystemClockConfigUpdate(void);
void GPIOConfig(void);
void NVIC_Config(void);
void EXTIConfig(void);

volatile uint8_t kontrol = 0;

int main(void)
{
  HAL_Init();
  //SystemClock_Config(); //Clock ayarlarını ben yapacağım
  SystemClockConfigUpdate();
  GPIOConfig();
  NVIC_Config();
  EXTIConfig();

  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

void EXTI0_IRQHandler(void){

	// Bu artık CPU nun çalıştıracağı ISR dir.

	// Edge detection -> EXTI PR bit set -> NVIC pending -> CPU exception entry


	if((EXTI->PR & 1)==1){ // EXTI0 interrupt oluştu mu kontrol et. genelde şu ifade kullanılır: if (EXTI->PR & (1 << 0))

		// Ledblink i gerçekleştir
		if(kontrol == 0){
			GPIOD->BSRR = (1<<12); // led yak
			kontrol=1;
		}
		else{
			GPIOD->BSRR = (1<<28); // led söndür
			kontrol = 0;
		}

		EXTI->PR |= (1<<0); // EXTI0 pending flag temizlenir (write-1-to-clear). Yeni bir edge oluşmadıkça interrupt tekrar oluşmaz.
	}
}

void EXTIConfig(){

	// PA0 → EXTI0 hattı → NVIC’e IRQ6 gönderilir
	// NVIC enable ve priority koşulları uygunsa CPU EXTI0_IRQHandler ISR'ını çalıştırır.

	// SYSCFG biriminin clock'unu aktif et.
	// EXTI hattının hangi GPIO portuna bağlanacağını belirleyen EXTICR register'ları SYSCFG içinde bulunduğu için bu clock gereklidir.
	RCC->APB2ENR |= (1<<14);

	// EXTI0 hattını PA0 pinine bağla.
	// EXTI line 0 için port seçimi EXTICR[0] registerının [3:0] bitleri ile yapılır.
	SYSCFG->EXTICR[0] &= ~(0xF << 0);  // EXTI0 için reset
	SYSCFG->EXTICR[0] |= (0x0 << 0);   // PA0 seçildi (0000 = PortA)

	EXTI->IMR |= (1<<0); // EXTI line 0 için interrupt mask kaldırıldı (interrupt enable), böylece interrupt NVIC'e iletilebilir

	// EXTI0 hattı için rising edge tetiklemesi aktif edildi
	// EXTI0 hattında yükselen kenar algılandığında interrupt oluşacak.
	EXTI->RTSR |= (1<<0);
}

void NVIC_Config(){

    // PA0 pininden gelen sinyal EXTI0 hattını tetikler.
    // EXTI0 hattı NVIC'e EXTI0_IRQn (IRQ6) interrupt isteği gönderir.
    // NVIC bu interrupt'ı enable edilmiş ve priority koşulları uygunsa CPU'ya iletir.
    // CPU da buna karşılık EXTI0_IRQHandler() ISR fonksiyonuna dallanır.

    // NVIC_ISER registerları interrupt enable etmek için kullanılır.
    // NVIC->ISER[0] registerı IRQ0–IRQ31 aralığındaki interruptları kontrol eder.
    // EXTI0 interrupt numarası = 6 olduğu için bit6 set edilmelidir.

    NVIC->ISER[0] = (1<<6);   // EXTI0_IRQn interruptını enable eder.

    // Not:
    // NVIC_ISER registerları "write-1-to-set" mantığıyla çalışır.
    // Bu yüzden |= kullanmak zorunlu değildir; doğrudan = ile yazmak yeterlidir.
    // Aynı biti tekrar set etmek bir sorun oluşturmaz.

    // NVIC'in bu interrupt için kullanacağı priority değerini ayarla.
    // Cortex-M4'te priority registerı 8 bittir ancak sadece üst 4 bit kullanılır.

    NVIC->IP[6] &= ~(0xF << 4);  // priority alanını temizle (EXTI0_IRQn = 6)
    NVIC->IP[6] |=  (2   << 4);  // priority = 2 olarak ayarla

    // IRQ (Interrupt Request):
    // Bir periferik tarafından NVIC'e gönderilen kesme isteğidir.
    // IRQ'nun bir numarası ve priority değeri vardır.

    // ISR (Interrupt Service Routine):
    // CPU'nun bu interrupt isteğine cevap olarak çalıştırdığı fonksiyondur.
    // Bu örnekte ISR fonksiyonu EXTI0_IRQHandler() fonksiyonudur.
}

void GPIOConfig(){

	RCC->AHB1ENR |= (1<<0) | (1<<3); // portA için ve portD için AHB1 hattını enable et. PortA_0. pin user buton,
									 // PortD_12. pin led olarak kullanılacak

	// PortA nın 0. pinini buton olarak kullanacağım.
	// Pini giriş pini olarak ayarla
	GPIOA->MODER &= ~(3<<0);

	// Bu pin buton tarafından tetiklenmezken pull down durumunda olsun. Bu pinin gürültü yapmasını önler.
	// Bu butona zaten donanımsal olarak da pull down direncine bağlı o yüzden yazılımsal olarak belirtmesek de olur.
	GPIOA->PUPDR &= ~(3<<0);
	GPIOA->PUPDR |= (1<<1);

	// PortD nin 12. pinini led olarak kullanacağım
	// Pini çıkış pini olarak ayarla
	GPIOD->MODER &= ~(3<<24);
	GPIOD->MODER |= (1<<24);

	// OTYPER
	GPIOD->OTYPER &= ~(1<<12);

	//OSPEEDR
	GPIOD->OSPEEDR &= ~(3<<24);
	GPIOD->OSPEEDR |= (2<<24); // Medium

	// Gerçek zamanlı işlemler için BSRR ile dijital çıkış vermek daha doğru.
	// Bu uygulamada şart değil ama örnek olarak kullandım
	GPIOD->BSRR = (1<<28);

}

void SystemClockConfigUpdate(){

	// Amaç: SYSCLK=168MHz çalıştırmak

	/* FLASH ayarları */
	FLASH->ACR |= (5<<0); // Bu satır LATENCY ayarını yapıyor, LATENCY = 5 → 5 wait states (STM32F4’de 168 MHz çalıştırmak için datasheet’e göre 5 wait state gerekiyor)
	FLASH->ACR |= (1<<8); // PRFTEN (Prefetch enable) bitini açar.
	FLASH->ACR |= (1<<9); // ICEN (Instruction cache enable) bitini açar.
	FLASH->ACR |= (1<<10); // DCEN (Data cache enable) bitini açar.


	RCC->CR |= (1<<16); // HSE enable edildi kullanılmak üzere
	while((RCC->CR&0x00020000)!=0x00020000); // HSE ready flag i 1 olup HSE nin çalışmaya hazır olduğunu söyleyene kadar bekle
	RCC->CR |= (1<<19); // HSE clock un çalışıp çalışmadığını izleyen controle detector ü enable et

	// PLL ayarları

	// PLL_M = 8
	RCC->PLLCFGR &= ~(0x3F<<0); // öncelikle ilgili bitleri temizledim.
	RCC->PLLCFGR |= (1<<3);
	// PLL_M çıkışı(PLL_N girişi) = 1MHz artık

	// PLL_N=336MHz olmalı
	RCC->PLLCFGR &= ~(0x1FF<<6); // ilgili bitleri temizle
	RCC->PLLCFGR |= (1<<14) | (1<<12) | (1<<10);

	// PLL_P=2
	RCC->PLLCFGR &= ~(3<<16); // Zaten bu hali ile PLL_P=2 olmuş olur

	RCC->PLLCFGR |= (1<<22);   // PLLSRC = HSE

	// PLL kullanacağımızı belirtmek için PLLON enable edilmeli
	RCC->CR |= (1<<24);

	while((RCC->CR & (1<<25)) == 0); // PLLRDY

	// PLLCLK yı SYSCLK kullanacağımı söylüyorum;
	RCC->CFGR &= ~(3<<0); // önce bitleri temizle
	RCC->CFGR |= (1<<1);

	while((RCC->CFGR & (3<<2)) != (2<<2)); //Switch’in tamamlandığını kontrol et

	// AHB Prescaler(HPRE biti) = 1 olmalı ki HCLK=168MHz olsun
	RCC->CFGR &= ~(0xF<<4);

	// SysTick frekans=168MHz olsun istediğim için HCLK/8 değil HCLK kaynağını direkt kullanacağım.
	// Bu nedenle STK_CTRL register ında clock source olarak HCLK yı seçmek istediğim için 2. biti "1" olarak ayarla
	SysTick->CTRL |= (1<<2);

	//  APB1=42, APB2=84 MHz de çalışsın diye PPRE1=4, PPRE2=2 olarak ayalarnmalı RCC_CFGR register ında
	RCC->CFGR &= ~(0x3F<<10); // bitleri temizle
	RCC->CFGR |= (1<<15) | (1<<12) | (1<<10);
}


/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
