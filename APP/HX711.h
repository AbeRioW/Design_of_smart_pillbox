#ifndef __HX711_H__
#define __HX711_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

typedef enum {
    CHAN_A_GAIN_128 = 1,
    CHAN_A_GAIN_64 = 3,
    CHAN_B_GAIN_32 = 2
} HX711_Gain_t;

typedef enum {
    UNIT_G = 0,
    UNIT_KG = 1,
    UNIT_LB = 2
} HX711_Unit_t;

typedef struct {
    float tareA_128;
    float tareA_64;
    float tareB_32;
} HX711_Tare_t;

typedef struct {
    float calFactorA_128;
    float calFactorA_64;
    float calFactorB_32;
} HX711_Cal_t;

void HX711_Init(void);
int32_t HX711_ReadChannelRaw(HX711_Gain_t gain);
int32_t HX711_ReadChannelBlocking(HX711_Gain_t gain);
void HX711_TareA(int32_t value);
void HX711_TareB(int32_t value);
float HX711_GetTareA128(void);
float HX711_GetTareA64(void);
float HX711_GetTareB32(void);
void HX711_SetCalFactorA128(float factor);
void HX711_SetCalFactorA64(float factor);
void HX711_SetCalFactorB32(float factor);
float HX711_GetCalFactorA128(void);
float HX711_GetCalFactorA64(void);
float HX711_GetCalFactorB32(void);
float HX711_GetWeight(HX711_Gain_t gain, HX711_Unit_t unit);
float HX711_ConvertUnit(float weight_g, HX711_Unit_t target_unit);

#ifdef __cplusplus
}
#endif

#endif
