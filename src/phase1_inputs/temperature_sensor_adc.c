/*
 * Phase 1 - Analog input: on-chip temperature sensor via SAR ADC
 * Target : SPC584B70E3 (SPC5Studio, RLA low-level drivers)
 * Reads ADC channel 38 continuously.
 */
#include "components.h"
#include "saradc_lld_cfg.h"

#define ADC_CHANNEL_TEMPERATURE  38

vuint16_t g_temp_adc_value;

int main(void) {
    componentsInit();

    saradc_lld_start(&SARADC12D1, &saradc_config_t_sense);
    saradc_lld_start_conversion(&SARADC12D1);

    while (1) {
        g_temp_adc_value = saradc_lld_readchannel(&SARADC12D1, ADC_CHANNEL_TEMPERATURE);
    }

    return 0;
}
