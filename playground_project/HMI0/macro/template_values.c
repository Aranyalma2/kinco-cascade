#define GEPEK_SZAMA 4 // Gepek szama

#define REG_SIMULATION 100 // Szimulacio start
#define REG_SIM_ITERATION 101 // Szimulacio iteracio

#define REG_UZEMMOD 1000                        // Hutes/Futes
#define REG_ALAPJEL 1001                        // Hutes/Futes alapjel
#define REG_HOMERSEKLET 1002                    // Aktualis homerseklet
#define REG_HOMERSEKLET_HISZTEREZIS 1003        // Homerseklet hiszterezis
#define REG_HMV_MINIMUM_HOMERSEKLET 1004        // HMV minimum tartaly homerseklet
#define REG_HMV_FUTES_FENNTARTAS_STATIKUS 1005  // HMV futesre statikusan fenntartott gepek szama
#define REG_HMV_FUTES_FENNTARTAS_DINAMIKUS 1006 // HMV futesre dinamikusan fenntartott gepek szama
#define REG_HMV_CEL_HOMERSEKLET 1007            // HMV tartaly cel homerseklet

#define REG_FIFO 1010         // LIFO tároló regiszterek kezdete
#define REG_HOSZIVATTYUK 1100 // Hoszivattyu registerek kezdete
/* ----- REGISZTEREK per gep -----
0 - engedelyezes
1 - HMV futes visszajelzes
2 - HMV tartaly homerseklet
3 - hiba jelzes
4 - uzem ido high byte
5 - uzem ido low byte
6 - normal inditas
7 - HMV inditas
*/

#include "macrotypedef.h"
#include "Math.h"

//-------------------- Simulation datas --------------------
short sim_started = 0; // Szimulacio elindult-e
short sim_iteration = 0; // Szimulacio iteracio


//-------------------- Controll data --------------------
short mode = 0;                           // 0 = hutes, 1 = futes
short setpoint = 0;                       // Homerseklet vezerlo alapjel
short temperature = 0;                    // aktualis homerseklet
short temperature_hysteresis = 0;         // Homerseklet hiszterezis => Bekapcsolando gepek szama a homerseklettol differencia alapjan
short HMV_avg_temperature = 0;            // HMV tartaly homerseklet mert atlag
short HMV_min_temperature = 0;            // HMV tartaly homerseklet minimum
short HMV_FUTES_FENNTARTAS_STATIKUS = 0;  // HMV futesre statikusan fenntartott gepek szama
short HMV_FUTES_FENNTARTAS_DINAMIKUS = 0; // HMV futesre dinamikusan fenntartott gepek szama
short HMV_target_temperature = 0;         // HMV tartaly cel homerseklet

//-------------------- Heatpump data --------------------

struct HeatPump
{
    short enable;          // Engedelyezes
    short HMV_feedback;    // HMV tartaly futes visszajelzes
    short HMV_temperature; // Mert HMV tartaly homerseklet
    short error;           // Hiba jelzes
    unsigned int runtime;  // Hoszivattyu uzemido
    short normal_start;    // Hutes/futes inditas
    short HMV_start;       // HMV tartaly futes inditas
};

struct HeatPump hoszivattyuk[GEPEK_SZAMA];

/*************** Utility Functions ***************/
unsigned int mergeShorts(unsigned short high, unsigned short low)
{
    return ((unsigned int)high << 16) | (unsigned int)low;
}

void splitInt(unsigned int value, unsigned short *high, unsigned short *low)
{
    *high = (unsigned short)(value >> 16);
    *low = (unsigned short)value;
}

// Save controll data to registers
void save_controll_data(short *buf)
{
    buf[0] = mode;
    buf[1] = setpoint;
    buf[2] = temperature;
    buf[3] = temperature_hysteresis;
    buf[4] = HMV_min_temperature;
    buf[5] = HMV_FUTES_FENNTARTAS_STATIKUS;
    buf[6] = HMV_FUTES_FENNTARTAS_DINAMIKUS;
    buf[7] = HMV_target_temperature;
}

// Save nessesery the heatpump data to registers
void save_hps(short *buf)
{
    short i = 0;
    for (i = 0; i < GEPEK_SZAMA; i++)
    {
        short machine_idx = i * 8;
        buf[machine_idx] = hoszivattyuk[i].enable;
        buf[machine_idx + 1] = hoszivattyuk[i].HMV_feedback;
        buf[machine_idx + 2] = hoszivattyuk[i].HMV_temperature;
        buf[machine_idx + 3] = hoszivattyuk[i].error;
        splitInt(hoszivattyuk[i].runtime, &buf[machine_idx + 5], &buf[machine_idx + 4]);
        buf[machine_idx + 6] = hoszivattyuk[i].normal_start;
        buf[machine_idx + 7] = hoszivattyuk[i].HMV_start;
    }
}

int MacroEntry()
{
    mode = 0;
        setpoint = 200;
        temperature = 200;
        temperature_hysteresis = 10;
        HMV_min_temperature = 400;
        HMV_FUTES_FENNTARTAS_STATIKUS = 1;
        HMV_FUTES_FENNTARTAS_DINAMIKUS = 1;
        HMV_target_temperature = 500;

        short i;
        for (i = 0; i < GEPEK_SZAMA; i++)
        {
            hoszivattyuk[i].enable = 1;
            hoszivattyuk[i].HMV_feedback = 0;
            hoszivattyuk[i].HMV_temperature = 500;
            hoszivattyuk[i].error = 0;
            hoszivattyuk[i].runtime = 0;
            hoszivattyuk[i].normal_start = 0;
            hoszivattyuk[i].HMV_start = 0;
    }
    // Load controll data
    short buf_controll[8];

    // Load heatpumps data
    short buf_hp[GEPEK_SZAMA * 8];

    // Save controll data
    save_controll_data(buf_controll);
    WriteLocal("LW", REG_UZEMMOD, 8, (void *)buf_controll, 0);

    // Save heatpumps data
    save_hps(buf_hp);
    WriteLocal("LW", REG_HOSZIVATTYUK, GEPEK_SZAMA * 8, (void *)buf_hp, 0);

    // Reset FIFOs
    short buff_fifo[GEPEK_SZAMA + 1] = {0};
    WriteLocal("LW", REG_FIFO, GEPEK_SZAMA + 1, (void *)buff_fifo, 0);
    WriteLocal("LW", REG_FIFO + GEPEK_SZAMA + 1, GEPEK_SZAMA + 1, (void *)buff_fifo, 0);

    return 0;
}