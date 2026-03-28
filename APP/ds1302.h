#ifndef __DS1302_H
#define __DS1302_H

#include "main.h"

// DS1302寄存器定义
#define DS1302_SECOND_REG 0x80
#define DS1302_MINUTE_REG 0x82
#define DS1302_HOUR_REG   0x84
#define DS1302_DATE_REG   0x86
#define DS1302_MONTH_REG  0x88
#define DS1302_DAY_REG    0x8A
#define DS1302_YEAR_REG   0x8C
#define DS1302_WP_REG     0x8E

// 时间结构体
typedef struct {
    uint8_t year;    // 年份 (00-99)
    uint8_t month;   // 月份 (01-12)
    uint8_t date;    // 日期 (01-31)
    uint8_t day;     // 星期 (01-07)
    uint8_t hour;    // 小时 (00-23)
    uint8_t minute;  // 分钟 (00-59)
    uint8_t second;  // 秒钟 (00-59)
} DS1302_Time;

// 函数原型
void DS1302_Init(void);
void DS1302_WriteByte(uint8_t addr, uint8_t data);
uint8_t DS1302_ReadByte(uint8_t addr);
void DS1302_SetTime(DS1302_Time *time);
void DS1302_GetTime(DS1302_Time *time);

#endif


