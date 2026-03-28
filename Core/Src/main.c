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

// 时间设置相关变量
uint8_t setting_mode = 0;  // 0: 阈值设置, 1: 时间设置
uint8_t time_setting_index = 0;  // 0:年, 1:月, 2:日, 3:时, 4:分, 5:秒
DS1302_Time setting_time;  // 设置中的时间

// 报警时间相关变量
uint8_t alarm_hour = 0;    // 报警小时
uint8_t alarm_minute = 0;  // 报警分钟
uint8_t alarm_second = 0;  // 报警秒
uint8_t alarm_active = 0;  // 报警状态
uint32_t alarm_start_time = 0;  // 报警开始时间
uint8_t alarm_setting_mode = 0;  // 0: 正常模式, 1: 报警时间设置模式
uint8_t alarm_setting_index = 0;  // 0:时, 1:分, 2:秒
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
  
  // 擦除页面
  if (HAL_FLASHEx_Erase(&EraseInitStruct, &PageError) == HAL_OK) {
    // 写入数据
    HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, address, data);
  }
  
  HAL_FLASH_Lock();
  
  // 验证写入是否成功
  uint32_t verify = Flash_Read(address);
  if (verify != data) {
    // 写入失败，使用默认值
    weight_threshold = 100000;
    // 再次尝试写入默认值
    HAL_FLASH_Unlock();
    if (HAL_FLASHEx_Erase(&EraseInitStruct, &PageError) == HAL_OK) {
      HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, address, weight_threshold);
    }
    HAL_FLASH_Lock();
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
  // 检查读取的值是否有效
  if (weight_threshold == 0xFFFFFFFF || weight_threshold < 100000 || weight_threshold > 1000000) {
    weight_threshold = 100000;
    // 如果是第一次运行或Flash值无效，写入默认值
    Flash_Write(FLASH_USER_START_ADDR, weight_threshold);
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
      // 添加按键消抖延迟
      HAL_Delay(100);
      // 再次检查按键状态，确保是有效的按键按下
      if (HAL_GPIO_ReadPin(KEY1_GPIO_Port, KEY1_Pin) == GPIO_PIN_RESET) {
        if (in_setting_mode) {
          // 在设置模式下，按KEY1切换设置模式或退出
          if (setting_mode == 0) {
            // 保存阈值设置
            Flash_Write(FLASH_USER_START_ADDR, weight_threshold);
            // 从阈值设置切换到时间设置
            setting_mode = 1;
            time_setting_index = 0;
            // 读取当前时间作为初始值
            DS1302_GetTime(&setting_time);
          } else if (setting_mode == 1) {
            // 退出设置模式
            // 保存时间设置
            DS1302_SetTime(&setting_time);
            // 清除OLED显示
            OLED_Clear();
            OLED_Refresh();
            in_setting_mode = 0;
          }
        } else {
          // 进入设置模式
          in_setting_mode = 1;
          setting_mode = 0;  // 默认进入阈值设置
          time_setting_index = 0;
          // 读取当前时间作为初始值
          DS1302_GetTime(&setting_time);
        }
      }
    }
    
    if (in_setting_mode) {
      // 处理设置模式下的按键
      if (key2_pressed) {
        key2_pressed = 0;
        if (setting_mode == 0) {
          // 阈值设置
          weight_threshold += 100000;
          if (weight_threshold > 1000000) weight_threshold = 1000000;
          Flash_Write(FLASH_USER_START_ADDR, weight_threshold);
        } else if (setting_mode == 1) {
          // 时间设置 - 增加当前项或切换设置项
          static uint32_t last_key2_time = 0;
          if (HAL_GetTick() - last_key2_time < 300) {
            // 短按：增加当前项
            switch (time_setting_index) {
              case 0:  // 年
                setting_time.year = (setting_time.year + 1) % 100;
                break;
              case 1:  // 月
                setting_time.month = (setting_time.month % 12) + 1;
                break;
              case 2:  // 日
                // 简单处理，实际应根据月份调整天数
                setting_time.day = (setting_time.day % 31) + 1;
                break;
              case 3:  // 时
                setting_time.hour = (setting_time.hour % 23) + 1;
                break;
              case 4:  // 分
                setting_time.minute = (setting_time.minute % 59) + 1;
                break;
              case 5:  // 秒
                setting_time.second = (setting_time.second % 59) + 1;
                break;
            }
          } else {
            // 长按：切换到下一个设置项
            time_setting_index = (time_setting_index + 1) % 6;
          }
          last_key2_time = HAL_GetTick();
        }
        HAL_Delay(20);
      }
      
      if (key3_pressed) {
        key3_pressed = 0;
        if (setting_mode == 0) {
          // 阈值设置 - 减少阈值
          if (weight_threshold >= 100000) {
            weight_threshold -= 100000;
            Flash_Write(FLASH_USER_START_ADDR, weight_threshold);
          }
        } else if (setting_mode == 1) {
          // 时间设置 - 减少当前项
          switch (time_setting_index) {
            case 0:  // 年
              setting_time.year = (setting_time.year + 99) % 100;
              break;
            case 1:  // 月
              setting_time.month = (setting_time.month + 10) % 12 + 1;
              break;
            case 2:  // 日
              setting_time.day = (setting_time.day + 29) % 31 + 1;
              break;
            case 3:  // 时
              setting_time.hour = (setting_time.hour + 23) % 24;
              break;
            case 4:  // 分
              setting_time.minute = (setting_time.minute + 59) % 60;
              break;
            case 5:  // 秒
              setting_time.second = (setting_time.second + 59) % 60;
              break;
          }
        }
        HAL_Delay(20);
      }
      
      // 显示设置界面
      OLED_Clear();
      if (setting_mode == 0) {
        // 阈值设置界面
        OLED_ShowString(0, 0, (uint8_t*)"Setting Mode", 8, 1);
        OLED_ShowString(0, 16, (uint8_t*)"Threshold:", 8, 1);
        OLED_ShowNum(64, 16, weight_threshold, 7, 8, 1);
        OLED_ShowString(0, 24, (uint8_t*)"KEY2:+ KEY3:-", 8, 1);
        OLED_ShowString(0, 32, (uint8_t*)"KEY1:Time", 8, 1);
      } else if (setting_mode == 1) {
        // 时间设置界面
        OLED_ShowString(0, 0, (uint8_t*)"Time Setting", 8, 1);
        
        // 显示当前选中的项
        const char* time_items[] = {"Year", "Month", "Day", "Hour", "Minute", "Second"};
        OLED_ShowString(0, 16, (uint8_t*)"Current:", 8, 1);
        OLED_ShowString(48, 16, (uint8_t*)time_items[time_setting_index], 8, 1);
        
        OLED_ShowString(0, 24, (uint8_t*)"KEY2:+ KEY3:-", 8, 1);
        OLED_ShowString(64, 24, (uint8_t*)"Long KEY2:Next", 8, 1);
        OLED_ShowString(0, 32, (uint8_t*)"KEY1:Exit", 8, 1);
        
        // 显示当前设置的时间（在Exit下一行）
        char time_str[30];
        sprintf(time_str, "20%02d-%02d-%02d %02d:%02d:%02d", 
                setting_time.year, setting_time.month, setting_time.day, 
                setting_time.hour, setting_time.minute, setting_time.second);
        OLED_ShowString(0, 40, (uint8_t*)time_str, 8, 1);
      }
      OLED_Refresh();
      
      HAL_Delay(100);
    } else {
      // 只在非设置模式下处理人体检测和显示
      // 不要清除key2_pressed和key3_pressed，因为用户可能在进入设置模式前按下了这些按键
      
      // 非设置模式下的按键处理（主页面）
      if (key2_pressed) {
        key2_pressed = 0;
        if (alarm_setting_mode) {
          // 报警时间设置模式 - 增加当前项
          switch (alarm_setting_index) {
            case 0:  // 时
              alarm_hour = (alarm_hour + 1) % 24;
              break;
            case 1:  // 分
              alarm_minute = (alarm_minute + 1) % 60;
              break;
            case 2:  // 秒
              alarm_second = (alarm_second + 1) % 60;
              break;
          }
        } else {
          // 进入报警时间设置模式
          alarm_setting_mode = 1;
          alarm_setting_index = 0;
        }
        HAL_Delay(20);
      }
      
      if (key3_pressed) {
        key3_pressed = 0;
        if (alarm_setting_mode) {
          // 报警时间设置模式 - 减少当前项或切换设置项
          static uint32_t last_key3_time = 0;
          if (HAL_GetTick() - last_key3_time < 300) {
            // 短按：减少当前项
            switch (alarm_setting_index) {
              case 0:  // 时
                alarm_hour = (alarm_hour + 23) % 24;
                break;
              case 1:  // 分
                alarm_minute = (alarm_minute + 59) % 60;
                break;
              case 2:  // 秒
                alarm_second = (alarm_second + 59) % 60;
                break;
            }
          } else {
            // 长按：切换到下一个设置项或退出设置模式
            if (alarm_setting_index < 2) {
              alarm_setting_index++;
            } else {
              // 退出报警时间设置模式
              alarm_setting_mode = 0;
            }
          }
          last_key3_time = HAL_GetTick();
        }
        HAL_Delay(20);
      }
      
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
      
      // 获取当前时间
      DS1302_Time current_time;
      DS1302_GetTime(&current_time);
      
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
      
      // 时间匹配报警逻辑
      if (current_time.hour == alarm_hour && 
          current_time.minute == alarm_minute && 
          current_time.second == alarm_second) {
        if (!alarm_active) {
          // 发送报警信息
          char alarm_msg[100];
          sprintf(alarm_msg, "ALARM: Time match! Current: %02d:%02d:%02d, Alarm: %02d:%02d:%02d\r\n", 
                  current_time.hour, current_time.minute, current_time.second,
                  alarm_hour, alarm_minute, alarm_second);
          HAL_UART_Transmit(&huart2, (uint8_t*)alarm_msg, strlen(alarm_msg), 1000);
          
          // 触发BEEP和LED
          HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
          HAL_GPIO_WritePin(BEEP_GPIO_Port, BEEP_Pin, GPIO_PIN_RESET);
          
          alarm_active = 1;
          alarm_start_time = HAL_GetTick();
        }
      }
      
      // 5秒后关闭报警
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
        OLED_ShowNum(16, 32, weight_threshold, 7, 8, 1);
        
        // 显示报警时间
        char alarm_str[20];
        sprintf(alarm_str, "Alarm: %02d:%02d:%02d", alarm_hour, alarm_minute, alarm_second);
        OLED_ShowString(0, 40, (uint8_t*)alarm_str, 8, 1);
        
        if (human_detected) {
          OLED_ShowString(64, 32, (uint8_t*)"Human", 8, 1);
        }
        
        // 显示报警设置提示（分两行显示）
        if (alarm_setting_mode) {
          const char* alarm_items[] = {"Hour", "Minute", "Second"};
          OLED_ShowString(0, 50, (uint8_t*)"Set:", 8, 1);
          OLED_ShowString(64, 50, (uint8_t*)alarm_items[alarm_setting_index], 8, 1);
        } else {
          // 主页面显示报警设置提示
          OLED_ShowString(0, 50, (uint8_t*)"KEY2:Alarm Set", 8, 1);
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
