/*
 * Phase 1 - Digital input: external switch read through a plain GPIO pad (polling)
 * Target : SPC584B70E3 (SPC5Studio, RLA low-level drivers)
 */
#include "components.h"

int dig_inp_rd;

int main(void) {
    componentsInit();

    while (1) {
        /* Reads the state (HIGH/LOW) of pin 2 on PORT_B and stores it in dig_inp_rd */
        dig_inp_rd = pal_lld_readpad(PORT_B, 2);
    }

    return 0;
}
