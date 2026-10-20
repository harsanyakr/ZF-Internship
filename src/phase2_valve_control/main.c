/*
 * Phase 2 - High-Side / Low-Side switch control with ADC current-sense feedback
 *           and fault detection for a solenoid valve.
 *
 * Target : SPC584B70E3 (SPC5Studio, RLA low-level drivers)
 * Author : Harsanya K R - ZF Commercial Vehicle Control Systems India Ltd (internship, May-Jun 2025)
 *
 * Behaviour
 *  - HSS is driven with a 1 kHz PWM on pin 88, LSS is switched ON/OFF through a GPIO.
 *  - IS (current sense) feedback of both switches is sampled with the 12-bit SAR ADC.
 *  - If a feedback value crosses a fault threshold, the HSS PWM is stopped, an error
 *    counter is incremented and a PIT timer is started to retry after valve_errDelay seconds.
 *  - After valve_errCount_max errors, both switches are disabled (shutdown until reset).
 */
#include "components.h"
#include "platform.h"
#include "saradc_lld_cfg.h"
#include "pwm_lld_cfg.h"
#include "pit_lld_cfg.h"

/* ---- pin / channel definitions in one place for readability ---- */
#define ADC_CHANNEL_LSS      82    /* feedback from LSS, pin 45, channel 82 */
#define ADC_CHANNEL_HSS      12    /* feedback from HSS, pin 14, channel 12 */
#define PWM_CHANNEL_PIN88    22    /* 1 kHz PWM of pin 88, channel 22, eMIOS 0, group 1 */
#define PIN91_PORT           PORT_E /* HSS enable (diagnostics enable), pin 91 */
#define PIN91_PAD            12
#define PIN84_PORT           PORT_E /* LSS control/enable, pin 84 */
#define PIN84_PAD            10
#define TIMER_CHANNEL        1     /* PIT channel used for the safety retry timer */

#define ADC_MAX_VALUE        4095.0f
#define ADC_VREF             5.0f

/* ---- status flags of the load ---- */
vuint8_t valve_noError_bit = 1;          /* 1 if no error occurred during operation, else 0 */
vuint8_t valve_operationStatus_bit = 1;  /* 1 if valve is operating, else 0 */

/* ---- tunable parameters ---- */
vuint32_t valve_errCount_max = 10;       /* max errors before total shutdown */
vuint32_t valve_errCount = 0;            /* running count of valve errors */
volatile double valve_errDelay = 2;      /* seconds between retries after an error */

/* ---- fault thresholds (12-bit ADC counts) ---- */
const int lss_fault_min = 3276;          /* LSS feedback >= 3276 (~4.0 V)  -> fault */
const int hss_fault_short_min = 4000;    /* HSS feedback >= 4000 (~4.9 V)  -> short circuit */
const int hss_fault_open_max  = 82;      /* HSS feedback <= 82   (~0.1 V)  -> open load */

/* ---- inputs ---- */
vuint16_t lss_fb_adc_value = 0;          /* feedback from pin 45 */
vuint16_t hss_fb_adc_value = 0;          /* feedback from pin 14 */
volatile float lss_fb_volt = 0;
volatile float hss_fb_volt = 0;

void safetyProtocol(void);

/* PIT callback: retry after the error delay */
void pit0_channel1_callback(void) {
    pit_lld_channel_stop(&PITD1, TIMER_CHANNEL);       /* shut down the timer */
    valve_errCount++;

    pwm_lld_start(&PWMD10, &pwm_config_pwm_pin88);     /* turn PWM back on for the HSS */
    valve_noError_bit = 1;
}

void setTimer(PITDriver *pitp, int channel, double delayTime) {
    double freq = 1 / delayTime;                       /* delay (s) -> timer frequency (Hz) */

    pit_lld_channel_set_freq(pitp, channel, freq);
    pit_lld_channel_start(pitp, channel);
}

void shutdownValve(void) {
    /* PWM to HSS is already stopped in safetyProtocol() */
    pal_lld_clearpad(PIN91_PORT, PIN91_PAD);           /* disable HSS (pin 91) */
    pal_lld_clearpad(PIN84_PORT, PIN84_PAD);           /* disable LSS (pin 84) */

    valve_operationStatus_bit = 0;
}

void componentsStartup(void) {
    pal_lld_setpad(PIN91_PORT, PIN91_PAD);             /* enable HSS in pin 91 */
    pal_lld_setpad(PIN84_PORT, PIN84_PAD);             /* enable LSS in pin 84 */

    /* analog inputs */
    saradc_lld_start(&SARADC12D1, &saradc_config_lss_hss_fb_ip);
    saradc_lld_start_conversion(&SARADC12D1);

    /* HSS PWM output: 1 kHz, 50 % duty */
    pwm_lld_enable_channel(&PWMD10, PWM_CHANNEL_PIN88,
                           PWM_PERCENTAGE_TO_WIDTH(&PWMD10, 5000), 0, 0, 0);
    pwm_lld_start(&PWMD10, &pwm_config_pwm_pin88);

    /* PIT timer used by the safety protocol */
    pit_lld_start(&PITD1, &pit0_config[TIMER_CHANNEL]);
}

void safetyProtocol(void) {
    pwm_lld_stop(&PWMD10);                             /* turn off PWM to HSS */
    valve_noError_bit = 0;

    if (valve_errCount >= valve_errCount_max) {
        shutdownValve();
    } else {
        setTimer(&PITD1, TIMER_CHANNEL, valve_errDelay); /* start timer to re-test the valve */
    }
}

int main(void) {
    componentsInit();
    componentsStartup();

    for (;;) {
        /* read current-sense feedback of LSS and HSS */
        lss_fb_adc_value = saradc_lld_readchannel(&SARADC12D1, ADC_CHANNEL_LSS);
        hss_fb_adc_value = saradc_lld_readchannel(&SARADC12D1, ADC_CHANNEL_HSS);

        /* convert to volts */
        lss_fb_volt = (lss_fb_adc_value / ADC_MAX_VALUE) * ADC_VREF;
        hss_fb_volt = (hss_fb_adc_value / ADC_MAX_VALUE) * ADC_VREF;

        /* check for faults only when there is no pre-existing error and the valve is operating */
        if (valve_noError_bit && valve_operationStatus_bit) {
            if (lss_fb_adc_value >= lss_fault_min) {
                safetyProtocol();                      /* general fault reported by LSS */
            } else if (hss_fb_adc_value >= hss_fault_short_min) {
                safetyProtocol();                      /* short-circuit reported by HSS */
            } else if (hss_fb_adc_value <= hss_fault_open_max) {
                safetyProtocol();                      /* open-circuit reported by HSS */
            }
        }
    }

    return 0;
}
