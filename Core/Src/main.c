/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "oled.h"
#include "ds1302.h"
#include "HX711.h"
#include <stdio.h>  // 添加stdio.h头文件
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
uint8_t key1_pressed = 0;
uint8_t key2_pressed = 0;
uint8_t key3_pressed = 0;
uint32_t weight_threshold = 100000;  // 全局阈值变量
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
#define FLASH_USER_START_ADDR   0x0800FC00
#define FLASH_USER_END_ADDR     0x08010000

// 函数声明
uint32_t Flash_Read(uint32_t address);

void Flash_Write(uint32_t address, uint32_t data)
{
  HAL_FLASH_Unlock();
  
  FLASH_EraseInitTypeDef EraseInitStruct;
  uint32_t PageError = 0;
  
  EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
  EraseInitStruct.PageAddress = address;
  EraseInitStruct.NbPages = 1;
  
  if (HAL_FLASHEx_Erase(&EraseInitStruct, &PageError) == HAL_OK) {
    HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, address, data);
  }
  
  HAL_FLASH_Lock();
  
  // 验证写入是否成功
  uint32_t verify = Flash_Read(address);
  if (verify != data) {
    // 写入失败，使用默认值
    weight_threshold = 100000;
  }
}

uint32_t Flash_Read(uint32_t address)
{
  return *(__IO uint32_t*)address;
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
  MX_USART2_UART_Init();
  MX_TIM1_Init();
  /* USER CODE BEGIN 2 */
  OLED_Init();
  DS1302_Init();
  HX711_Init();
//          HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
//					        HAL_GPIO_WritePin(BEEP_GPIO_Port, BEEP_Pin, GPIO_PIN_RESET);
  HX711_SetCalFactorB32(0.01f);
  
  HX711_TareB(-832236);
  
  DS1302_Time time;
  time.year = 26;
  time.month = 3;
  time.day = 26;
  time.hour = 12;
  time.minute = 0;
  time.second = 0;
  DS1302_SetTime(&time);
  
  // 从Flash读取阈值
  weight_threshold = Flash_Read(FLASH_USER_START_ADDR);
  if (weight_threshold < 100000 || weight_threshold > 1000000) {
    weight_threshold = 100000;
  }
  
  uint8_t in_setting_mode = 0;
  
  uint8_t human_detect_count = 0;
  uint8_t human_detected = 0;
  
  static uint32_t last_display_time = 0;
  
  OLED_Refresh();
  
  // 启动定时器1中断，每秒更新一次时间显示
  HAL_TIM_Base_Start_IT(&htim1);
  
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    // 检查时间更新标志，每秒更新一次时间显示

    
    // Handle key interrupts
    if (key1_pressed) {
      key1_pressed = 0;
      in_setting_mode = !in_setting_mode;
      if (!in_setting_mode) {
        Flash_Write(FLASH_USER_START_ADDR, weight_threshold);
      }
      HAL_Delay(20);
    }
    
    if (in_setting_mode) {
      // 处理设置模式下的按键
      if (key2_pressed) {
        key2_pressed = 0;
        weight_threshold += 100000;
        if (weight_threshold > 1000000) weight_threshold = 1000000;
        Flash_Write(FLASH_USER_START_ADDR, weight_threshold);
        HAL_Delay(20);
      }
      
      if (key3_pressed) {
        key3_pressed = 0;
        if (weight_threshold >= 100000) weight_threshold -= 100000;
        Flash_Write(FLASH_USER_START_ADDR, weight_threshold);
        HAL_Delay(20);
      }
      
      // 显示设置界面
      OLED_Clear();
      OLED_ShowString(0, 0, (uint8_t*)"Setting Mode", 8, 1);
      OLED_ShowString(0, 16, (uint8_t*)"Threshold:", 8, 1);
      OLED_ShowNum(64, 16, weight_threshold, 6, 8, 1);  // 改为6位显示
      OLED_ShowString(0, 24, (uint8_t*)"KEY2:+ KEY3:-", 8, 1);
      OLED_ShowString(0, 32, (uint8_t*)"KEY1:Exit", 8, 1);
      OLED_Refresh();
      
      HAL_Delay(100);
    } else {
      // 只在非设置模式下处理人体检测和显示
      // 不要清除key2_pressed和key3_pressed，因为用户可能在进入设置模式前按下了这些按键
      
      // HCSR505 human detection
      uint8_t hcsr505_state = HAL_GPIO_ReadPin(HC_SR505_GPIO_Port, HC_SR505_Pin);
      if (hcsr505_state == GPIO_PIN_SET) {
        if (human_detect_count < 5) {
          human_detect_count++;
        }
        if (human_detect_count >= 5) {
          human_detected = 1;
        }
      } else {
        human_detect_count = 0;
        human_detected = 0;
      }
      
      int32_t weightA128 = HX711_ReadChannelBlocking(CHAN_A_GAIN_128);
      int32_t rawB32 = HX711_ReadChannelBlocking(CHAN_B_GAIN_32);
      
      static uint32_t alarm_start_time = 0;
      static uint8_t alarm_active = 0;
      
      // Check if weight is below threshold
      if (human_detected && rawB32 < weight_threshold) {
        if (!alarm_active) {
          // 发送报警信息
          char alarm_msg[100];
          sprintf(alarm_msg, "ALARM: Weight below threshold! Current: %ld, Threshold: %ld\r\n", rawB32, weight_threshold);
          HAL_UART_Transmit(&huart2, (uint8_t*)alarm_msg, strlen(alarm_msg), 1000);
          
          // 触发BEEP和LED
          HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
          HAL_GPIO_WritePin(BEEP_GPIO_Port, BEEP_Pin, GPIO_PIN_RESET);
          
          alarm_active = 1;
          alarm_start_time = HAL_GetTick();
        }
      } else {
        // 没有报警时关闭BEEP和LED
        if (alarm_active) {
          HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
          HAL_GPIO_WritePin(BEEP_GPIO_Port, BEEP_Pin, GPIO_PIN_SET);
          alarm_active = 0;
        }
      }
      
      // 检查报警是否持续了5秒
      if (alarm_active && (HAL_GetTick() - alarm_start_time >= 5000)) {
        HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(BEEP_GPIO_Port, BEEP_Pin, GPIO_PIN_SET);
        alarm_active = 0;
      }
      
      // 更新时间显示
      if (time_update_flag) {
        time_update_flag = 0;
        Update_Time_Display();
        
        OLED_ShowString(0, 16, (uint8_t*)"A128:", 8, 1);
        OLED_ShowNum(30, 16, weightA128, 8, 8, 1);
        
        OLED_ShowString(0, 24, (uint8_t*)"B32:", 8, 1);
        OLED_ShowNum(24, 24, rawB32, 8, 8, 1);
        
        OLED_ShowString(0, 32, (uint8_t*)"Th:", 8, 1);
        OLED_ShowNum(16, 32, weight_threshold, 6, 8, 1);
        
        if (human_detected) {
          OLED_ShowString(64, 32, (uint8_t*)"Human", 8, 1);
        }
      }
      
      HAL_Delay(10);
    }
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

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  // 直接设置标志，不在中断中使用HAL_Delay
  if (GPIO_Pin == KEY1_Pin) {
    key1_pressed = 1;
  } else if (GPIO_Pin == KEY2_Pin) {
    key2_pressed = 1;
  } else if (GPIO_Pin == KEY3_Pin) {
    key3_pressed = 1;
  }
}
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
