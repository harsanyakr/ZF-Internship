/*
 * Phase 1 - Digital input: external switch read using the ICU (Input Capture Unit) driver
 * Target : SPC584B70E3 (SPC5Studio, RLA low-level drivers)
 * The ICU is enabled on an eMIOS channel and the input level is polled
 * from the SIUL2 input data register.
 */
#include "components.h"
#include "icu_lld_cfg.h"

int main(void) {
    componentsInit();

    icu_lld_enable(&ICUD8);                        /* enable ICU capturing */
    icu_lld_start(&ICUD8, &icu_config_dig_inp);    /* start ICU driver for the eMIOS channel */

    while (1) {
        if (SIUL2.GPDI[9].R) {
            /* Input is HIGH */
        } else {
            /* Input is LOW */
        }
    }

    return 0;
}
