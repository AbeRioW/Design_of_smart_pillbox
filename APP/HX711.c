#include "HX711.h"
#include "stm32f1xx_hal.h"

static HX711_Tare_t tareValues = {0};
static HX711_Cal_t calFactors = {1.0f, 1.0f, 1.0f};

void HX711_Init(void)
{
    HAL_GPIO_WritePin(HX711_SCK_GPIO_Port, HX711_SCK_Pin, GPIO_PIN_RESET);
}

static int32_t HX711_ReadRaw(void)
{
    int32_t value = 0;
    uint8_t i;

    while (HAL_GPIO_ReadPin(HX711_DT_GPIO_Port, HX711_DT_Pin) == GPIO_PIN_SET);

    for (i = 0; i < 24; i++) {
        HAL_GPIO_WritePin(HX711_SCK_GPIO_Port, HX711_SCK_Pin, GPIO_PIN_SET);
        value <<= 1;
        if (HAL_GPIO_ReadPin(HX711_DT_GPIO_Port, HX711_DT_Pin) == GPIO_PIN_SET) {
            value |= 0x01;
        }
        HAL_GPIO_WritePin(HX711_SCK_GPIO_Port, HX711_SCK_Pin, GPIO_PIN_RESET);
    }

    if (value & 0x00800000) {
        value |= 0xFF000000;
    }

    return value;
}

int32_t HX711_ReadChannelRaw(HX711_Gain_t gain)
{
    int32_t value;
    uint8_t i;

    value = HX711_ReadRaw();

    for (i = 0; i < (uint8_t)gain; i++) {
        HAL_GPIO_WritePin(HX711_SCK_GPIO_Port, HX711_SCK_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(HX711_SCK_GPIO_Port, HX711_SCK_Pin, GPIO_PIN_RESET);
    }

    return value;
}

int32_t HX711_ReadChannelBlocking(HX711_Gain_t gain)
{
    return HX711_ReadChannelRaw(gain);
}

void HX711_TareA(int32_t value)
{
    tareValues.tareA_128 = (float)value;
    tareValues.tareA_64 = (float)value;
}

void HX711_TareB(int32_t value)
{
    tareValues.tareB_32 = (float)value;
}

float HX711_GetTareA128(void)
{
    return tareValues.tareA_128;
}

float HX711_GetTareA64(void)
{
    return tareValues.tareA_64;
}

float HX711_GetTareB32(void)
{
    return tareValues.tareB_32;
}

void HX711_SetCalFactorA128(float factor)
{
    calFactors.calFactorA_128 = factor;
}

void HX711_SetCalFactorA64(float factor)
{
    calFactors.calFactorA_64 = factor;
}

void HX711_SetCalFactorB32(float factor)
{
    calFactors.calFactorB_32 = factor;
}

float HX711_GetCalFactorA128(void)
{
    return calFactors.calFactorA_128;
}

float HX711_GetCalFactorA64(void)
{
    return calFactors.calFactorA_64;
}

float HX711_GetCalFactorB32(void)
{
    return calFactors.calFactorB_32;
}

float HX711_GetWeight(HX711_Gain_t gain, HX711_Unit_t unit)
{
    int32_t raw = HX711_ReadChannelBlocking(gain);
    float tare = 0;
    float calFactor = 1.0f;
    float weight_g;

    switch (gain) {
        case CHAN_A_GAIN_128:
            tare = tareValues.tareA_128;
            calFactor = calFactors.calFactorA_128;
            break;
        case CHAN_A_GAIN_64:
            tare = tareValues.tareA_64;
            calFactor = calFactors.calFactorA_64;
            break;
        case CHAN_B_GAIN_32:
            tare = tareValues.tareB_32;
            calFactor = calFactors.calFactorB_32;
            break;
    }

    weight_g = (float)(raw - tare) * calFactor;

    return HX711_ConvertUnit(weight_g, unit);
}

float HX711_ConvertUnit(float weight_g, HX711_Unit_t target_unit)
{
    switch (target_unit) {
        case UNIT_G:
            return weight_g;
        case UNIT_KG:
            return weight_g / 1000.0f;
        case UNIT_LB:
            return weight_g * 0.00220462f;
        default:
            return weight_g;
    }
}
