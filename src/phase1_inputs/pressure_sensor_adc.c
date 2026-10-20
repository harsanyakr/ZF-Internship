/*
 * Phase 1 - Analog input: pressure sensor via SAR ADC
 * Target : SPC584B70E3 (SPC5Studio, RLA low-level drivers)
 * Reads ADC channel 67 continuously and converts the 12-bit result to volts.
 */
#include "components.h"
#include "saradc_lld_cfg.h"

#define ADC_CHANNEL_PRESSURE  67
#define ADC_MAX_VALUE         4095.0f   /* 12-bit */
#define ADC_VREF              5.0f      /* reference voltage in volts */

vuint16_t g_pressure_adc_value;
volatile float pressure_val;
volatile char adc_value_str[5];

int main(void) {
    componentsInit();
    saradc_lld_start(&SARADC12D1, &saradc_config_task_1); /* start ADC driver with the generated configuration */
    saradc_lld_start_conversion(&SARADC12D1);             /* begin continuous conversions */

    while (1) {
        g_pressure_adc_value = saradc_lld_readchannel(&SARADC12D1, ADC_CHANNEL_PRESSURE);

        /* split the ADC value into 4 ASCII digits for display */
        adc_value_str[0] = '0' + (g_pressure_adc_value / 1000) % 10;
        adc_value_str[1] = '0' + (g_pressure_adc_value / 100) % 10;
        adc_value_str[2] = '0' + (g_pressure_adc_value / 10) % 10;
        adc_value_str[3] = '0' + (g_pressure_adc_value % 10);
        adc_value_str[4] = '\0';

        /* Vin = ADC / ADC_max * Vref */
        pressure_val = (g_pressure_adc_value / ADC_MAX_VALUE) * ADC_VREF;
    }

    return 0;
}
