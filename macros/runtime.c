#define GEPEK_SZAMA 4 // Gepek szama

#define REG_HOSZIV 1100 // Heatpump register

#include "macrotypedef.h"

/*************** Utility Functions ***************/
unsigned int mergeShorts(unsigned short high, unsigned short low) {
    return ((unsigned int)high << 16) | (unsigned int)low;
}

void splitInt(unsigned int value, unsigned short *high, unsigned short *low) {
    *high = (unsigned short)(value >> 16);
    *low = (unsigned short)value;
}

struct HeatPump {
    short enable; // Engedelyezes
    short HMV_feedback; // HMV tartaly futes visszajelzes
    short HMV_temperature; // Mert HMV tartaly homerseklet
    short error; // Hiba jelzes
    unsigned int runtime; // Hoszivattyu uzemido
    short normal_start; //Hutes/futes inditas
    short HMV_start; // HMV tartaly futes inditas
};

struct HeatPump hoszivattyuk[GEPEK_SZAMA];

// Load the heatpump data from registers
void load_hps(short* buf) {
    short i = 0;
    for (i = 0; i < GEPEK_SZAMA; i++) {
        short machine_idx = i * 8;
        hoszivattyuk[i].enable = buf[machine_idx];
        hoszivattyuk[i].HMV_feedback = buf[machine_idx + 1];
        hoszivattyuk[i].HMV_temperature = buf[machine_idx + 2];
        hoszivattyuk[i].error = buf[machine_idx + 3];
        hoszivattyuk[i].runtime = mergeShorts(buf[machine_idx + 5], buf[machine_idx + 4]);
        hoszivattyuk[i].normal_start = buf[machine_idx + 6];
        hoszivattyuk[i].HMV_start = buf[machine_idx + 7];
    }
}

// Save the heatpump data to registers
void save_hps(short* buf) {
    short i = 0;
    for (i = 0; i < GEPEK_SZAMA; i++) {
        short machine_idx = i * 8;
        splitInt(hoszivattyuk[i].runtime, &buf[machine_idx + 5], &buf[machine_idx + 4]);
    }
}

void increase_runtime() {
    short i;
    for (i = 0; i < GEPEK_SZAMA; i++) {
        if ((hoszivattyuk[i].enable == 1 && hoszivattyuk[i].error == 0) && ((hoszivattyuk[i].normal_start == 1 && hoszivattyuk[i].HMV_start == 0) || hoszivattyuk[i].HMV_feedback == 1)) {
            hoszivattyuk[i].runtime++;
        }
    }
}

int MacroEntry() {
    short buf[GEPEK_SZAMA * 8];
    ReadLocal("LW", REG_HOSZIV, GEPEK_SZAMA * 8, (void*)buf, 0);
    load_hps(buf);
    increase_runtime();
    save_hps(buf);
    WriteLocal("LW", REG_HOSZIV, GEPEK_SZAMA * 8, (void*)buf, 0);
}