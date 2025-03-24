#define GEPEK_SZAMA 4 // Gepek szama

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

#define DEBUG

#include "macrotypedef.h"
#include "Math.h"
#include <time.h>

//-------------------- Simulation datas --------------------
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

// Check if the HP is able to turn on: enabled, no error
short can_turn_on(short idx)
{
    return hoszivattyuk[idx].enable == 1 && hoszivattyuk[idx].error == 0;
}

// Load controll data from registers
void load_controll_data(short *buf)
{
    mode = buf[0];
    setpoint = buf[1];
    temperature = buf[2];
    temperature_hysteresis = buf[3];
    HMV_min_temperature = buf[4];
    HMV_FUTES_FENNTARTAS_STATIKUS = buf[5];
    HMV_FUTES_FENNTARTAS_DINAMIKUS = buf[6];
    HMV_target_temperature = buf[7];
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

// Load the heatpump data from registers
void load_hps(short *buf)
{
    short i = 0;
    for (i = 0; i < GEPEK_SZAMA; i++)
    {
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

// Get AVG of the HMV temperatures from no error heatpumps
short get_avg_hmv_temperature()
{
    short sum = 0;
    short count = 0;
    short i = 0;
    for (i = 0; i < GEPEK_SZAMA; i++)
    {
        if (can_turn_on(i))
        {
            sum += hoszivattyuk[i].HMV_temperature;
            count++;
        }
    }
    return count > 0 ? sum / count : 0;
}

short any_HMV_on()
{
    short i;
    for (i = 0; i < GEPEK_SZAMA; i++)
    {
        if (hoszivattyuk[i].HMV_feedback == 1)
        {
            return 1;
        }
    }
    return 0;
}

//Simulation is randomly adjusting controll data and heatpumps enable, error, HMV feedback and HMV temperature
void make_simulation() {
    //Simulation probability persentage

    #define SIM_PROB_MODE_CHANGE 10
    #define SIM_PROB_SETPOINT_CHANGE 10
    #define SIM_PROB_TEMP_CHANGE 10
    #define SIM_PROB_TEMP_DRIFT 25
    #define SIM_PROB_HMV_MIN_CHANGE 10
    #define SIM_PROB_HYSTERESIS_CHANGE 10
    #define SIM_PROB_HMV_STATIC_CHANGE 10
    #define SIM_PROB_HMV_DYNAMIC_CHANGE 10
    #define SIM_PROB_HP_ENABLE 95
    #define SIM_PROB_HP_ERROR 5
    #define SIM_PROB_HP_TEMP_CHANGE 50

    /*-------------------- Simulation --------------------
    ************ Simulation priciples ************

    //take all simulation probability persentage from 0-100 and a easely settable variable to the top of the simualtion part
    
    Controll datas: - little adjustment on controll data
                    - random mode (0 or 1) but change probability is 10%
                    - random setpoint differencial (0/5), but change probability is 10%
                    - random temperature differencial (0-5), but is mode is 0 make it negative, if mode is 1 make it positive
                    - if temperature reach the setpoint, make 25% chance to it drasticly (20-100), but is mode is 0 make it positive change, if mode is 0 make it negative
                    - random HMV min temperature (0-5), but change probability is 10%, but if HMV min temperature is close to HMV target temperature (less then 100), subtract/add 50 to them
                    - random hysteresis (5/10/15), but change probability is 10%
                    - random HMV static futes fenntartas (0-2), but change probability is 10%
                    - random HMV dynamic futes fenntartas (0-2), but change probability is 10%

    Heatpumps datas: - little adjustment on heatpumps data
                          - random HeatPump enable (>90%)
                          - error (<10%)
                          - HMV feedback (Set to 1 if HMV start and HMV avg temperature is lower than HMV target temperature)
                          - HMV temperature differencial negative (0-5), but if HMV any feedback is 1, make it positive change
    */

    // Randomly adjust control data
    if (rand() % 100 < SIM_PROB_MODE_CHANGE) mode = rand() % 2;
    if (rand() % 100 < SIM_PROB_SETPOINT_CHANGE) setpoint += (rand() % 2) * 5;
    if (rand() % 100 < SIM_PROB_TEMP_CHANGE) temperature += (mode == 0 ? -1 : 1) * (rand() % 6);

    if (((mode == 0 && temperature - setpoint < 5) || (mode == 1 && temperature - setpoint > 5)) && rand() % 100 < SIM_PROB_TEMP_DRIFT) {
        temperature += (mode == 0 ? 1 : -1) * (20 + rand() % 81);
    }

    if (rand() % 100 < SIM_PROB_HMV_MIN_CHANGE) HMV_min_temperature += (rand() % 6);
    if (abs(HMV_min_temperature - HMV_target_temperature) < 100) {
        HMV_min_temperature += (rand() % 2 == 0 ? -50 : 50);
    }

    if (rand() % 100 < SIM_PROB_HYSTERESIS_CHANGE) temperature_hysteresis = (rand() % 3 + 1) * 5;
    if (rand() % 100 < SIM_PROB_HMV_STATIC_CHANGE) HMV_FUTES_FENNTARTAS_STATIKUS = rand() % 3;
    if (rand() % 100 < SIM_PROB_HMV_DYNAMIC_CHANGE) HMV_FUTES_FENNTARTAS_DINAMIKUS = rand() % 3;

    // Update heatpump data
    short i;
    for (i = 0; i < GEPEK_SZAMA; i++) {
        if (rand() % 100 < SIM_PROB_HP_ENABLE) hoszivattyuk[i].enable = 1;
        else hoszivattyuk[i].enable = 0;
        if (rand() % 100 < SIM_PROB_HP_ERROR) hoszivattyuk[i].error = 1;
        else hoszivattyuk[i].error = 0;

        if (hoszivattyuk[i].HMV_start && get_avg_hmv_temperature() < HMV_target_temperature) {
            hoszivattyuk[i].HMV_feedback = 1;
        } else {
            hoszivattyuk[i].HMV_feedback = 0;
        }

        if (rand() % 100 < SIM_PROB_HP_TEMP_CHANGE) {
            hoszivattyuk[i].HMV_temperature += any_HMV_on() ? (rand() % 6) : -(rand() % 6);
        }
    }

    sim_iteration++;

}

int MacroEntry()
{
    int t = time(NULL);
    srand(t);
    #ifdef DEBUG
    short r = rand();
    WriteLocal("LW", 17, 1, (void *)&t, 0);
    WriteLocal("LW", 18, 1, (void *)&r, 0);
    #endif

    // Load simulation data
    ReadLocal("LW", REG_SIM_ITERATION, 1, (void *)&sim_iteration, 0);

    // Load controll data
    short buf_controll[8];
    ReadLocal("LW", REG_UZEMMOD, 8, (void *)buf_controll, 0);
    load_controll_data(buf_controll);

    // Load heatpumps data
    short buf_hp[GEPEK_SZAMA * 8];
    ReadLocal("LW", REG_HOSZIVATTYUK, GEPEK_SZAMA * 8, (void *)buf_hp, 0);
    load_hps(buf_hp);

    make_simulation();

    // Save simulation data
    WriteLocal("LW", REG_SIM_ITERATION, 1, (void *)&sim_iteration, 0);

    // Save controll data
    save_controll_data(buf_controll);
    WriteLocal("LW", REG_UZEMMOD, 8, (void *)buf_controll, 0);

    // Save heatpumps data
    save_hps(buf_hp);
    WriteLocal("LW", REG_HOSZIVATTYUK, GEPEK_SZAMA * 8, (void *)buf_hp, 0);

    return 0;
}
