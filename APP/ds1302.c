#include "ds1302.h"

// 位操作宏
#define DS1302_CLK_HIGH HAL_GPIO_WritePin(DS1302_CLK_GPIO_Port, DS1302_CLK_Pin, GPIO_PIN_SET)
#define DS1302_CLK_LOW  HAL_GPIO_WritePin(DS1302_CLK_GPIO_Port, DS1302_CLK_Pin, GPIO_PIN_RESET)
#define DS1302_RST_HIGH HAL_GPIO_WritePin(DS1302_RST_GPIO_Port, DS1302_RST_Pin, GPIO_PIN_SET)
#define DS1302_RST_LOW  HAL_GPIO_WritePin(DS1302_RST_GPIO_Port, DS1302_RST_Pin, GPIO_PIN_RESET)
#define DS1302_DAT_HIGH HAL_GPIO_WritePin(DS1302_DAT_GPIO_Port, DS1302_DAT_Pin, GPIO_PIN_SET)
#define DS1302_DAT_LOW  HAL_GPIO_WritePin(DS1302_DAT_GPIO_Port, DS1302_DAT_Pin, GPIO_PIN_RESET)
#define DS1302_DAT_READ HAL_GPIO_ReadPin(DS1302_DAT_GPIO_Port, DS1302_DAT_Pin)

// 数据引脚模式切换函数
void DS1302_DAT_SetOutput(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = DS1302_DAT_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(DS1302_DAT_GPIO_Port, &GPIO_InitStruct);
    DS1302_DAT_HIGH;
}

void DS1302_DAT_SetInput(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = DS1302_DAT_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(DS1302_DAT_GPIO_Port, &GPIO_InitStruct);
}

// 延时函数
void DS1302_Delay(uint32_t us)
{
    // 简单的延时函数，确保足够的延时时间
    uint32_t i;
    for (i = 0; i < us * 10; i++)
    {
        __NOP();
    }
}

/**
  * @brief  初始化DS1302
  * @retval None
  */
void DS1302_Init(void)
{
    // 初始化GPIO
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    // 时钟引脚
    GPIO_InitStruct.Pin = DS1302_CLK_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(DS1302_CLK_GPIO_Port, &GPIO_InitStruct);
    
    // 数据引脚
    GPIO_InitStruct.Pin = DS1302_DAT_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    HAL_GPIO_Init(DS1302_DAT_GPIO_Port, &GPIO_InitStruct);
    
    // 复位引脚
    GPIO_InitStruct.Pin = DS1302_RST_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    HAL_GPIO_Init(DS1302_RST_GPIO_Port, &GPIO_InitStruct);
    
    // 初始状态
    DS1302_CLK_LOW;
    DS1302_RST_LOW;
    DS1302_DAT_LOW;
}

/**
  * @brief  向DS1302写入一个字节
  * @param  addr: 地址
  * @param  data: 数据
  * @retval None
  */
void DS1302_WriteByte(uint8_t addr, uint8_t data)
{
    uint8_t i;
    
    // 启动通信
    DS1302_RST_LOW;
    DS1302_Delay(1);
    DS1302_CLK_LOW;
    DS1302_Delay(1);
    DS1302_RST_HIGH;
    DS1302_Delay(1);
    
    // 写入地址
    for (i = 0; i < 8; i++)
    {
        if (addr & 0x01)
        {
            DS1302_DAT_HIGH;
        }
        else
        {
            DS1302_DAT_LOW;
        }
        DS1302_Delay(1);
        DS1302_CLK_HIGH;
        DS1302_Delay(1);
        DS1302_CLK_LOW;
        DS1302_Delay(1);
        addr >>= 1;
    }
    
    // 写入数据
    for (i = 0; i < 8; i++)
    {
        if (data & 0x01)
        {
            DS1302_DAT_HIGH;
        }
        else
        {
            DS1302_DAT_LOW;
        }
        DS1302_Delay(1);
        DS1302_CLK_HIGH;
        DS1302_Delay(1);
        DS1302_CLK_LOW;
        DS1302_Delay(1);
        data >>= 1;
    }
    
    // 结束通信
    DS1302_RST_LOW;
    DS1302_Delay(1);
}

/**
  * @brief  从DS1302读取一个字节
  * @param  addr: 地址
  * @retval 读取的数据
  */
uint8_t DS1302_ReadByte(uint8_t addr)
{
    uint8_t i, data = 0;
    
    // DS1302读地址需要在写地址基础上加1
    addr |= 0x01;
    
    // 启动通信
    DS1302_RST_LOW;
    DS1302_Delay(1);
    DS1302_CLK_LOW;
    DS1302_Delay(1);
    DS1302_RST_HIGH;
    DS1302_Delay(1);
    
    // 写入地址
    for (i = 0; i < 8; i++)
    {
        if (addr & 0x01)
        {
            DS1302_DAT_HIGH;
        }
        else
        {
            DS1302_DAT_LOW;
        }
        DS1302_Delay(1);
        DS1302_CLK_HIGH;
        DS1302_Delay(1);
        DS1302_CLK_LOW;
        DS1302_Delay(1);
        addr >>= 1;
    }
    
    // 切换到输入模式
    DS1302_DAT_SetInput();
    DS1302_Delay(1);
    
    // 读取数据
    for (i = 0; i < 8; i++)
    {
        data >>= 1;
        if (DS1302_DAT_READ)
        {
            data |= 0x80;
        }
        DS1302_Delay(1);
        DS1302_CLK_HIGH;
        DS1302_Delay(1);
        DS1302_CLK_LOW;
        DS1302_Delay(1);
    }
    
    // 切换回输出模式
    DS1302_DAT_SetOutput();
    DS1302_Delay(1);
    
    // 结束通信
    DS1302_RST_LOW;
    DS1302_Delay(1);
    
    return data;
}

/**
  * @brief  设置DS1302的时间
  * @param  time: 时间结构体指针
  * @retval None
  */
void DS1302_SetTime(DS1302_Time *time)
{
    // 关闭写保护
    DS1302_WriteByte(DS1302_WP_REG, 0x00);
    
    // 写入时间数据（BCD码）
    // 注意：秒寄存器的最高位是CH位（时钟停止），必须设置为0才能让时钟运行
    DS1302_WriteByte(DS1302_YEAR_REG, ((time->year / 10) << 4) | (time->year % 10));
    DS1302_WriteByte(DS1302_MONTH_REG, ((time->month / 10) << 4) | (time->month % 10));
    DS1302_WriteByte(DS1302_DATE_REG, ((time->date / 10) << 4) | (time->date % 10));
    DS1302_WriteByte(DS1302_DAY_REG, ((time->day / 10) << 4) | (time->day % 10));
    DS1302_WriteByte(DS1302_HOUR_REG, ((time->hour / 10) << 4) | (time->hour % 10));
    DS1302_WriteByte(DS1302_MINUTE_REG, ((time->minute / 10) << 4) | (time->minute % 10));
    DS1302_WriteByte(DS1302_SECOND_REG, (((time->second / 10) << 4) | (time->second % 10)) & 0x7F); // 确保CH位为0
    
    // 开启写保护
    DS1302_WriteByte(DS1302_WP_REG, 0x80);
}

/**
  * @brief  获取DS1302的时间
  * @param  time: 时间结构体指针
  * @retval None
  */
void DS1302_GetTime(DS1302_Time *time)
{
    uint8_t temp;
    
    // 读取时间数据（BCD码）
    temp = DS1302_ReadByte(DS1302_YEAR_REG);
    time->year = (temp >> 4) * 10 + (temp & 0x0F);
    
    temp = DS1302_ReadByte(DS1302_MONTH_REG);
    time->month = (temp >> 4) * 10 + (temp & 0x0F);
    
    temp = DS1302_ReadByte(DS1302_DATE_REG);
    time->date = (temp >> 4) * 10 + (temp & 0x0F);
    
    temp = DS1302_ReadByte(DS1302_DAY_REG);
    time->day = (temp >> 4) * 10 + (temp & 0x0F);
    
    temp = DS1302_ReadByte(DS1302_HOUR_REG);
    time->hour = (temp >> 4) * 10 + (temp & 0x0F);
    
    temp = DS1302_ReadByte(DS1302_MINUTE_REG);
    time->minute = (temp >> 4) * 10 + (temp & 0x0F);
    
    temp = DS1302_ReadByte(DS1302_SECOND_REG);
    time->second = (temp >> 4) * 10 + (temp & 0x0F);
}