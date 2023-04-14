/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "dma.h"
#include "i2c.h"
#include "iwdg.h"
#include "rtc.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stdio.h"
#include "lps25hb.h"
#include "hagl.h"
#include "font6x9.h"
#include "font10x20.h"
#include "rgb565.h"
#include "wchar.h"
#include "font9x18.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */


//Add __WFI() -> sleep (wait for interrupt)
void HAL_Delay(uint32_t Delay)
{
  uint32_t tickstart = HAL_GetTick();
  uint32_t wait = Delay;

  /* Add a period to guaranty minimum wait */
  if (wait < HAL_MAX_DELAY)
  {
    wait += (uint32_t)uwTickFreq;
  }

  while ((HAL_GetTick() - tickstart) < wait)
  {
	  __WFI();
  }
}

int __io_putchar(int ch)
{
  if (ch == '\n') {
    __io_putchar('\r');
  }

  HAL_UART_Transmit(&huart2, (uint8_t*)&ch, 1, HAL_MAX_DELAY);

  return 1;
}

volatile uint8_t flag = 0;

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  SystemClock_Config ();
  HAL_ResumeTick();
  if (GPIO_Pin == USER_BUTTON_Pin) {
	  static uint32_t last_interrupt_time = 0;
	  uint32_t current_interrupt_time = HAL_GetTick();
	  // button filter ( 100ms)
	  if ((current_interrupt_time - last_interrupt_time) > 300) {
		  last_interrupt_time = current_interrupt_time;
		  printf("Enter interrupt!\n");
		  HAL_Delay(100);
		  if(flag==0) {
			  flag=1;
			  printf("Set flag 1\n");
			  HAL_Delay(200);
		  }
		  else if(flag==1) {
			  flag=2;
			  printf("Set flag 2\n");
			  HAL_Delay(200);
		  }
		  else if (flag==2){
			  flag=0;
	  }
	}
  }
}

void read_and_save_correct_time_after_sleep()
{
	  RTC_TimeTypeDef RTC_Time1;
	  RTC_Time1.Hours = HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR1);
	  RTC_Time1.Minutes = HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR2);
	  RTC_Time1.Seconds = HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR3);

	  RTC_TimeTypeDef RTC_Time2;
	  RTC_DateTypeDef RTC_Date2;
	  HAL_RTC_GetTime(&hrtc, &RTC_Time2, RTC_FORMAT_BIN);
	  HAL_RTC_GetDate(&hrtc, &RTC_Date2, RTC_FORMAT_BIN);

	  /*uint8_t seconds_slept = ((RTC_Time2.Seconds - RTC_Time1.Seconds) +
	                           ((RTC_Time2.Minutes - RTC_Time1.Minutes) * 60) +
	                           ((RTC_Time2.Hours - RTC_Time1.Hours) * 3600));*/
	  uint8_t seconds_slept = RTC_Time2.Seconds - RTC_Time1.Seconds;
	  RTC_Time1.Seconds=RTC_Time1.Seconds+seconds_slept;
	  if(RTC_Time1.Seconds>60){
		  RTC_Time1.Seconds=RTC_Time1.Seconds-60;
		  RTC_Time1.Minutes=RTC_Time1.Minutes+1;
		  if(RTC_Time1.Minutes>60){
			  RTC_Time1.Minutes=RTC_Time1.Minutes-60;
			  RTC_Time1.Hours=RTC_Time1.Hours+1;
			  if(RTC_Time1.Hours>23) RTC_Time1.Hours=0;
		  }
	  }
	  // Set the saved time back to RTC
	  HAL_RTC_SetTime(&hrtc, &RTC_Time1, RTC_FORMAT_BIN);
}

void save_time_to_sleep()
{
	RTC_TimeTypeDef currentTime;
	RTC_DateTypeDef currentdate;
	HAL_RTC_GetTime(&hrtc, &currentTime, RTC_FORMAT_BIN);
	HAL_RTC_GetDate(&hrtc, &currentdate, RTC_FORMAT_BIN);
	HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR1, currentTime.Hours);
	HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR2, currentTime.Minutes);
	HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR3, currentTime.Seconds);
}

void lps_temperature_pressure_measure()
{
	  printf("LPS measure init!\n");
	  printf("Searching...\n");
	  if (lps25hb_init() == HAL_OK) {
	    printf("OK: LPS25HB\n");
	    lps25hb_one_shot();
	    HAL_Delay(100);
	    printf("T = %.1f*C\n", lps25hb_read_temp()-2);
	    printf("p = %.1f hPa\n", lps25hb_read_pressure());

	    wchar_t buffer[20];
	    swprintf(buffer, 20, L"Temperatura: %.1f°C", lps25hb_read_temp()-2);
	    hagl_put_text(buffer, 20, 75, YELLOW, font6x9);
	    swprintf(buffer, 20, L"Cisnienie: %.0fhPa", lps25hb_read_pressure());
	    hagl_put_text(buffer, 20, 95, YELLOW, font6x9);
	    lcd_copy();

	    RTC_TimeTypeDef time;
	    RTC_DateTypeDef date;
		HAL_RTC_GetTime(&hrtc, &time, RTC_FORMAT_BIN);
		HAL_RTC_GetDate(&hrtc, &date, RTC_FORMAT_BIN);
		printf("Aktualny czas: %02d:%02d:%02d\n", time.Hours, time.Minutes, time.Seconds);
	  }
	  else {
	    printf("Error: LPS25HB not found\n");
	    Error_Handler();
	  }
}

void first_procedure_hour_change()
{
	  printf("Entering first procedure!\n");
	  int16_t prev_value=0;
	  uint8_t i=0;
	  RTC_TimeTypeDef time;
	  RTC_DateTypeDef date;
	  HAL_RTC_GetTime(&hrtc, &time, RTC_FORMAT_BIN);
	  HAL_RTC_GetDate(&hrtc, &date, RTC_FORMAT_BIN);
	  wchar_t buffer[20];
	  //draw rectangle at Hour
	  for (int i = 0; i < 2; i++) {
	    hagl_draw_rounded_rectangle(52+i, 30+i, 75-i, 55-i, 2-i, YELLOW);
	  }
	  swprintf(buffer, 20, L"%02d:%02d", time.Hours, time.Minutes);
	  hagl_put_text(buffer, 55, 35, YELLOW, font9x18);
	  lcd_copy();
	  while(flag==1){
		  int16_t value = __HAL_TIM_GET_COUNTER(&htim3);
		  if (value != prev_value) {
			  //printf("value = %d\n", value);
			  i++;
			  if(value==prev_value+1){
				  if(i==2)
				  {
					  HAL_IWDG_Refresh(&hiwdg);
					  time.Hours=time.Hours+1;
					  if(time.Hours==24) time.Hours=0;
					  HAL_RTC_SetTime(&hrtc, &time, RTC_FORMAT_BIN);
					  HAL_RTC_SetDate(&hrtc, &date, RTC_FORMAT_BIN);
					  HAL_RTC_GetTime(&hrtc, &time, RTC_FORMAT_BIN);
					  HAL_RTC_GetDate(&hrtc, &date, RTC_FORMAT_BIN);
					  printf("Hours: %d\n", time.Hours);
					  i=0;
					  swprintf(buffer, 20, L"%02d:%02d", time.Hours, time.Minutes);
					  hagl_put_text(buffer, 55, 35, YELLOW, font9x18);
					  lcd_copy();
				  }
			  }
			  if(value==prev_value-1){
				  if(i==2)
				  {
					  HAL_IWDG_Refresh(&hiwdg);
					  time.Hours=time.Hours-1;
					  if(time.Hours==255) time.Hours=23;
					  HAL_RTC_SetTime(&hrtc, &time, RTC_FORMAT_BIN);
					  printf("Hours: %d\n", time.Hours);
					  i=0;
					  swprintf(buffer, 20, L"%02d:%02d", time.Hours, time.Minutes);
					  hagl_put_text(buffer, 55, 35, YELLOW, font9x18);
					  lcd_copy();
				  }
			  }
			  prev_value = value;
		  }
	  }
	  for (int i = 0; i < 2; i++) {
	    hagl_draw_rounded_rectangle(52+i, 30+i, 75-i, 55-i, 2-i, BLACK);
	  }
}

void second_procedure_minutes_change()
{
	  printf("Entering second procedure!\n");
	  HAL_Delay(100);
	  int16_t prev_value=0;
	  uint8_t i=0;
	  RTC_TimeTypeDef time;
	  RTC_DateTypeDef date;
	  HAL_RTC_GetTime(&hrtc, &time, RTC_FORMAT_BIN);
	  HAL_RTC_GetDate(&hrtc, &date, RTC_FORMAT_BIN);
	  wchar_t buffer[20];
	  for (int i = 0; i < 2; i++) {
	    hagl_draw_rounded_rectangle(80+i, 30+i, 105-i, 55-i, 2-i, YELLOW);
	  }
	  swprintf(buffer, 20, L"%02d:%02d", time.Hours, time.Minutes);
	  hagl_put_text(buffer, 55, 35, YELLOW, font9x18);
	  lcd_copy();
	  while(flag==2){
		  int16_t value = __HAL_TIM_GET_COUNTER(&htim3);
		  if (value != prev_value) {
			  //printf("value = %d\n", value);
			  i++;
			  if(value==prev_value+1){
				  if(i==2)
				  {
					  HAL_IWDG_Refresh(&hiwdg);
					  time.Minutes=time.Minutes+1;
					  if(time.Minutes==60) time.Minutes=0;
					  HAL_RTC_SetTime(&hrtc, &time, RTC_FORMAT_BIN);
					  printf("Minutes: %d\n", time.Minutes);
					  i=0;
					  swprintf(buffer, 20, L"%02d:%02d", time.Hours, time.Minutes);
					  hagl_put_text(buffer, 55, 35, YELLOW, font9x18);
					  lcd_copy();
				  }
			  }
			  if(value==prev_value-1){
				  if(i==2)
				  {
					  HAL_IWDG_Refresh(&hiwdg);
					  time.Minutes=time.Minutes-1;
					  if(time.Minutes==255) time.Minutes=59;
					  HAL_RTC_SetTime(&hrtc, &time, RTC_FORMAT_BIN);
					  printf("Minutes: %d\n", time.Minutes);
					  i=0;
					  swprintf(buffer, 20, L"%02d:%02d", time.Hours, time.Minutes);
					  hagl_put_text(buffer, 55, 35, YELLOW, font9x18);
					  lcd_copy();
				  }
			  }
			  prev_value = value;
		  }
	  }
	  for (int i = 0; i < 2; i++) {
	    hagl_draw_rounded_rectangle(80+i, 30+i, 105-i, 55-i, 2-i, BLACK);
	  }
	  swprintf(buffer, 20, L"%02d:%02d", time.Hours, time.Minutes);
	  hagl_put_text(buffer, 55, 35, YELLOW, font9x18);
	  lcd_copy();

	for(int i=0;i<1;i++){
		RTC_TimeTypeDef time;
		RTC_DateTypeDef date;
	  HAL_RTC_GetTime(&hrtc, &time, RTC_FORMAT_BIN);
	  HAL_RTC_GetDate(&hrtc, &date, RTC_FORMAT_BIN);
	  printf("Aktualny czas: %02d:%02d:%02d\n", time.Hours, time.Minutes, time.Seconds);
	  HAL_Delay(100);
	}
}

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi) //When we get interrupt (If DMA finish send data to LCD)
{
	if (hspi == &hspi2)
	{
		lcd_transfer_done();
	}
}

void lcd_transfer_done(void)
{
	HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET);
}

bool lcd_is_busy(void)
{
	if (HAL_SPI_GetState(&hspi2) == HAL_SPI_STATE_BUSY)
		return true;
	else
		return false;
}

volatile uint8_t flag_lps=0;
volatile uint8_t flag_check_time=0;

void check_time()
{
	  printf("Time check init!\n");
	  RTC_TimeTypeDef time;
	  RTC_DateTypeDef date;
	  HAL_RTC_GetTime(&hrtc, &time, RTC_FORMAT_BIN);
	  HAL_RTC_GetDate(&hrtc, &date, RTC_FORMAT_BIN);
	  wchar_t buffer[20];
	  swprintf(buffer, 20, L"%02d:%02d", time.Hours, time.Minutes);
	  hagl_put_text(buffer, 55, 35, YELLOW, font9x18);
	  lcd_copy();
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim == &htim6) {
	  flag_check_time=1;
  }
  if (htim == &htim7) {
	  flag_lps=1;
  }
}


/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_RTC_Init();
  MX_USART2_UART_Init();
  MX_I2C1_Init();
  MX_SPI2_Init();
  MX_TIM3_Init();
  MX_IWDG_Init();
  MX_TIM6_Init();
  MX_TIM7_Init();
  /* USER CODE BEGIN 2 */

  lcd_init();
  printf("LCD initialization!\n");
  for (int i = 0; i < 8; i++) {
    hagl_draw_rounded_rectangle(2+i, 2+i, 158-i, 126-i, 8-i, rgb565(0, 0, i*16));
  }

  RTC_TimeTypeDef time;
  RTC_DateTypeDef date;
  HAL_RTC_GetTime(&hrtc, &time, RTC_FORMAT_BIN);
  HAL_RTC_GetDate(&hrtc, &date, RTC_FORMAT_BIN);
  wchar_t buffer[20];
  swprintf(buffer, 20, L"%02d:%02d", time.Hours, time.Minutes);
  hagl_put_text(buffer, 55, 35, YELLOW, font9x18);

  lps_temperature_pressure_measure();

  HAL_TIM_Base_Start_IT(&htim6);
  HAL_TIM_Base_Start_IT(&htim7);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  HAL_IWDG_Refresh(&hiwdg);
	  /*SET HOURS*/
	  if(flag==1)
	  {
		  printf("First procedure!!\n");
		  HAL_TIM_Encoder_Start(&htim3, TIM_CHANNEL_ALL);
		  first_procedure_hour_change();
	  }
	  if(flag==2)
	  {
		  printf("Second procedure!\n");
		  second_procedure_minutes_change();
		  HAL_TIM_Encoder_Stop(&htim3, TIM_CHANNEL_ALL);
	  }

	  if(__HAL_TIM_GET_FLAG(&htim6, TIM_FLAG_UPDATE) != RESET) // Sprawdzenie flagi timera
	  {
		__HAL_TIM_CLEAR_FLAG(&htim6, TIM_FLAG_UPDATE); // Wyczyszczenie flagi timera
		check_time();
	  }
	  /*
	  if(flag_check_time==1)
	  {
		  check_time();
		  flag_check_time=0;
	  }
	  */
	  if(flag_lps==1)
	  {
		  lps_temperature_pressure_measure();
		  flag_lps=0;
	  }

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
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
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure LSE Drive Capability
  */
  HAL_PWR_EnableBkUpAccess();
  __HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_LOW);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_LSE
                              |RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = 0;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_7;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_MSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }

  /** Enable MSI Auto calibration
  */
  HAL_RCCEx_EnableMSIPLLMode();
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

#ifdef  USE_FULL_ASSERT
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
