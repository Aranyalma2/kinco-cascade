#define GEPEK_SZAMA 4 // Gepek szama



#define REG_UZEMMOD 1000                        // Hutes/Futes

#define REG_ALAPJEL 1001                        // Hutes/Futes alapjel

#define REG_HOMERSEKLET 1002                    // Aktualis homerseklet

#define REG_HOMERSEKLET_HISZTEREZIS 1003        // Homerseklet hiszterezis

#define REG_HMV_MINIMUM_HOMERSEKLET 1004        // HMV minimum tartaly homerseklet

#define REG_HMV_FUTES_FENNTARTAS_STATIKUS 1005  // HMV futesre statikusan fenntartott gepek szama

#define REG_HMV_FUTES_FENNTARTAS_DINAMIKUS 1006 // HMV futesre dinamikusan fenntartott gepek szama

#define REG_HMV_CEL_HOMERSEKLET 1007            // HMV tartaly cel homerseklet



#define REG_FIFO 1010         // LIFO t�rol� regiszterek kezdete

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



//#define DEBUG



#include "macrotypedef.h"



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



/****** FIFO START ******/

typedef struct Stack

{

    short data[GEPEK_SZAMA];

    short top;

    short (*is_empty)(struct Stack *);

    short (*is_full)(struct Stack *);

    short (*clear)(struct Stack *);

    short (*deleteValue)(struct Stack *, short);

    short (*push)(struct Stack *, short);

    short (*pop)(struct Stack *);

    void (*export)(struct Stack *, short *);

    void (*import)(struct Stack *, short *);

} Stack;



short is_empty(Stack *s)

{

    return (s->top == -1);

}



short is_full(Stack *s)

{

    return (s->top == GEPEK_SZAMA - 1);

}



short clear(Stack *s)

{

    s->top = -1;

    return 0;

}



//Delete a value any place from the FIFO, entire stack is shifted

short deleteValue(Stack *s, short value)

{

    short i;

    short found = 0;

    for (i = 0; i <= s->top; i++)

    {

        if (s->data[i] == value)

        {

            found = 1;

            break;

        }

    }

    if (found)

    {

        for (i = i; i < s->top; i++)

        {

            s->data[i] = s->data[i + 1];

        }

        s->top--;

    }

    return found;

}



short push(Stack *s, short value)

{

    if (is_full(s))

    {

        return -1;

    }

    s->top++;

    s->data[s->top] = value;

    return 0;

}



short pop(Stack *s)

{

    short value;

    short i;



    if (is_empty(s))

    {

        return -1;

    }

    value = s->data[0];

    for (i = 0; i < s->top; i++)

    {

        s->data[i] = s->data[i + 1];

    }

    s->top--;

    return value;

}



void export_stack(Stack *s, short *buffer)

{

    short i;

    buffer[0] = s->top + 1;

    for (i = 0; i < GEPEK_SZAMA; i++)

    {

        buffer[i + 1] = s->data[i];

    }

}



void import_stack(Stack *s, short *buffer)

{

    short i;

    s->top = buffer[0] - 1;

    for (i = 0; i < GEPEK_SZAMA; i++)

    {

        s->data[i] = buffer[i + 1];

    }

}



void init(Stack *s)

{

    s->top = -1;

    s->is_empty = is_empty;

    s->is_full = is_full;

    s->clear = clear;

    s->deleteValue = deleteValue;

    s->push = push;

    s->pop = pop;

    s->export = export_stack;

    s->import = import_stack;

}



/****** FIFO END ******/



short mode = 0;                           // 0 = hutes, 1 = futes

short setpoint = 0;                       // Homerseklet vezerlo alapjel

short temperature = 0;                    // aktualis homerseklet

short temperature_hysteresis = 0;         // Homerseklet hiszterezis => Bekapcsolando gepek szama a homerseklettol differencia alapjan

short HMV_avg_temperature = 0;            // HMV tartaly homerseklet mert atlag

short HMV_min_temperature = 0;            // HMV tartaly homerseklet minimum

short HMV_FUTES_FENNTARTAS_STATIKUS = 0;  // HMV futesre statikusan fenntartott gepek szama

short HMV_FUTES_FENNTARTAS_DINAMIKUS = 0; // HMV futesre dinamikusan fenntartott gepek szama

short HMV_target_temperature = 0;         // HMV tartaly cel homerseklet



Stack normal_on_fifo; // Gepek hutes/futes bekapcsolasi sorrend

Stack hmv_on_fifo;    // Gepek HMV futes bekapcsolasi sorrend



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



struct TurnOnHeatPumpNumbers

{

    short normal;

    short hmv;

};



struct HeatPump hoszivattyuk[GEPEK_SZAMA];



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

        buf[machine_idx + 6] = hoszivattyuk[i].normal_start;

        buf[machine_idx + 7] = hoszivattyuk[i].HMV_start;

    }

}



// Check if the HP is able to turn on: enabled, no error

short can_turn_on(short idx)

{

    return hoszivattyuk[idx].enable == 1 && hoszivattyuk[idx].error == 0;

}



// Get the number of heatpumps that able to turn on

short get_number_of_turnable_hp()

{

    short count = 0;

    short i = 0;

    for (i = 0; i < GEPEK_SZAMA; i++)

    {

        if (can_turn_on(i))

        {

            count++;

        }

    }

    return count;

}



// Calulate the number of heatpumps that are on in heating or cooling mode (not in HMV mode)

short get_turned_on_normalmode_hp()

{

    short on = 0;

    short i = 0;

    for (i = 0; i < GEPEK_SZAMA; i++)

    {

        if (can_turn_on(i) && hoszivattyuk[i].normal_start == 1 && hoszivattyuk[i].HMV_start == 0)

        {

            on++;

        }

    }

    return on;

}



// Calulate the number of heatpumps that are in HMV mode

short get_turned_on_hmvmode_hp()

{

    short on = 0;

    short i = 0;

    for (i = 0; i < GEPEK_SZAMA; i++)

    {

        if (can_turn_on(i) && hoszivattyuk[i].HMV_start == 1)

        {

            on++;

        }

    }

    return on;

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



// Allocate dynamic number of heatpumps to HMV mode if the AVG HMV temperature is lower than the minimum

short allocate_hmv()

{

    short avg = get_avg_hmv_temperature();

    if (HMV_FUTES_FENNTARTAS_DINAMIKUS < HMV_FUTES_FENNTARTAS_STATIKUS)

    {

        return HMV_FUTES_FENNTARTAS_STATIKUS;

    }

    if (avg < HMV_min_temperature || (avg < HMV_target_temperature && get_turned_on_hmvmode_hp() == HMV_FUTES_FENNTARTAS_DINAMIKUS))

    {

        return HMV_FUTES_FENNTARTAS_DINAMIKUS;

    }

    else

    {

        return HMV_FUTES_FENNTARTAS_STATIKUS;

    }

}



// Calculate required heatpump for reaching the setpoint and hmvtank temperature

struct TurnOnHeatPumpNumbers calc_required_hp()

{

    short current_on = get_turned_on_normalmode_hp();

    short max_hp = get_number_of_turnable_hp();

    short normal_start = 0;



    if ((mode == 0 && temperature > setpoint) || (mode == 1 && temperature < setpoint))

    {

        short temp_diff = (temperature > setpoint) ? (temperature - setpoint) : (setpoint - temperature);

        short turnon = (temperature_hysteresis > 0) ? ((short)(((float)(temp_diff) / temperature_hysteresis) - current_on) + current_on) : max_hp;

        normal_start = (turnon > max_hp) ? max_hp : turnon;

    }

    

    short hmv_start = allocate_hmv();

    normal_start = (normal_start + hmv_start > max_hp) ? (max_hp - hmv_start) : normal_start;

    

    struct TurnOnHeatPumpNumbers turn_on_hp = {normal_start, hmv_start};

    return turn_on_hp;

}



short get_lowest_runtime_not_on_hp()

{

    short i;

    short min = -1; // Start with an invalid index



    for (i = 0; i < GEPEK_SZAMA; i++)

    {

        if (can_turn_on(i) && hoszivattyuk[i].normal_start == 0 && hoszivattyuk[i].HMV_feedback == 0)

        {

            if (min == -1 || hoszivattyuk[i].runtime < hoszivattyuk[min].runtime)

            {

                min = i;

            }

        }

    }



    return min;

}



short get_lowest_runtime_not_on_hp2()

{

    short i;

    short min = -1; // Start with an invalid index



    for (i = 0; i < GEPEK_SZAMA; i++)

    {

        if (can_turn_on(i) && hoszivattyuk[i].normal_start == 0 && hoszivattyuk[i].HMV_start == 0)

        {

            if (min == -1 || hoszivattyuk[i].runtime < hoszivattyuk[min].runtime)

            {

                min = i;

            }

        }

    }



    return min;

}



#ifdef DEBUG

short normal_requested_hp = 0;

short hmv_requested_hp = 0;

short norm_diff = 0;

short hmv_diff = 0;

short norm_min = -2;

short hmv_min = -2;

short err = 0;

#endif



// Destribute turn on signals to heatpumps for least runtime if required higher than already on

void distribute_turnon()

{

    short i, normal_on_hp, hmv_on_hp, diff;

    struct TurnOnHeatPumpNumbers required_numbers = calc_required_hp();

    normal_on_hp = get_turned_on_normalmode_hp();

    hmv_on_hp = get_turned_on_hmvmode_hp();



#ifdef DEBUG

    normal_requested_hp = required_numbers.normal;

    hmv_requested_hp = required_numbers.hmv;

    norm_diff = required_numbers.normal - normal_on_hp;

    hmv_diff = required_numbers.hmv - hmv_on_hp;

#endif



    // Turn on normal mode heatpumps

    diff = required_numbers.normal - normal_on_hp;

    if (diff > 0)

    {

        for (i = 0; i < GEPEK_SZAMA; i++)

        {

            short turn_on = get_lowest_runtime_not_on_hp();

            #ifdef DEBUG

            norm_min = turn_on;

            #endif

            if (turn_on == -1)

            {

                break;

            }

            if (normal_on_fifo.push(&normal_on_fifo, turn_on) == -1)

            {

                break;

            }

            hoszivattyuk[turn_on].normal_start = 1;

            hoszivattyuk[turn_on].HMV_start = 0;

            diff--;

            if (diff == 0)

            {

                break;

            }

        }

    }

    else if (diff < 0)

    {

        for (i = 0; i < GEPEK_SZAMA; i++)

        {

            short turn_off = normal_on_fifo.pop(&normal_on_fifo);

            if (turn_off == -1)

            {

                break;

            }

            hoszivattyuk[turn_off].normal_start = 0;

            diff++;

            if (diff == 0)

            {

                break;

            }

        }

    }



    // Turn on HMV mode heatpumps

    diff = required_numbers.hmv - hmv_on_hp;

    if (diff > 0)

    {

        for (i = 0; i < GEPEK_SZAMA; i++)

        {

            short turn_on = get_lowest_runtime_not_on_hp2();

            #ifdef DEBUG

            hmv_min = turn_on;

            #endif

            if (turn_on == -1)

            {

                break;

            }

            if (hmv_on_fifo.push(&hmv_on_fifo, turn_on) == -1)

            {

                break;

            }

            hoszivattyuk[turn_on].normal_start = 0;

            hoszivattyuk[turn_on].HMV_start = 1;

            diff--;

            if (diff == 0)

            {

                break;

            }

        }

    }

    else if (diff < 0)

    {

        for (i = 0; i < GEPEK_SZAMA; i++)

        {

            short turn_off = hmv_on_fifo.pop(&hmv_on_fifo);

            if (turn_off == -1)

            {

                break;

            }

            hoszivattyuk[turn_off].HMV_start = 0;

            diff++;

            if (diff == 0)

            {

                break;

            }

        }

    }

}



void remove_start_on_error()

{

    short i = 0;

    for (i = 0; i < GEPEK_SZAMA; i++)

    {

        if (!can_turn_on(i))

        {

            hoszivattyuk[i].normal_start = 0;

            hoszivattyuk[i].HMV_start = 0;

            deleteValue(&normal_on_fifo, i);

            deleteValue(&hmv_on_fifo, i);

        }

    }

}



// If only 1 heatpump is on working condition, turn both NORMAL and HMV start on.

void emergency_state()

{

    // Clear FIFOs

    normal_on_fifo.clear(&normal_on_fifo);

    hmv_on_fifo.clear(&hmv_on_fifo);



    short i;

    for (i = 0; i < GEPEK_SZAMA; i++)

    {

        if (can_turn_on(i))

        {

            hoszivattyuk[i].normal_start = 1;

            hoszivattyuk[i].HMV_start = 1;

            normal_on_fifo.push(&normal_on_fifo, i);

            hmv_on_fifo.push(&hmv_on_fifo, i);

        }

    }

}



void remove_leftover_emergency_on()

{

    short i;

    for (i = 0; i < GEPEK_SZAMA; i++)

    {

        if(hoszivattyuk[i].normal_start == 1 && hoszivattyuk[i].HMV_start == 1)

        {

            if(hoszivattyuk[i].HMV_feedback == 1)

            {  

                hoszivattyuk[i].normal_start = 0;

                normal_on_fifo.pop(&normal_on_fifo);

                

            }

            else

            {

                hoszivattyuk[i].HMV_start = 0;

                hmv_on_fifo.pop(&hmv_on_fifo);

            }

        }

    }

}



int MacroEntry()

{

    // Init FIFOs

    init(&normal_on_fifo);

    init(&hmv_on_fifo);

    // Load FIFO data

    short buff_fifo[GEPEK_SZAMA + 1];

    ReadLocal("LW", REG_FIFO, GEPEK_SZAMA + 1, (void *)buff_fifo, 0);

    normal_on_fifo.import(&normal_on_fifo, buff_fifo);

    ReadLocal("LW", REG_FIFO + GEPEK_SZAMA + 1, GEPEK_SZAMA + 1, (void *)buff_fifo, 0);

    hmv_on_fifo.import(&hmv_on_fifo, buff_fifo);



    // Load controll data

    short buf_controll[8];

    ReadLocal("LW", REG_UZEMMOD, 8, (void *)buf_controll, 0);

    load_controll_data(buf_controll);



    // Load heatpumps data

    short buf_hp[GEPEK_SZAMA * 8];

    ReadLocal("LW", REG_HOSZIVATTYUK, GEPEK_SZAMA * 8, (void *)buf_hp, 0);

    load_hps(buf_hp);



    // Remove any start signal if the heatpump is disabled or has an error

    remove_start_on_error();



    // Check if only 1 heatpump is on working condition

    if (get_number_of_turnable_hp() <= 1)

    {

        emergency_state();

    }

    else

    {   

        remove_leftover_emergency_on();

        // Distribute turnon signals

        distribute_turnon();

    }



    // Save heatpumps data

    save_hps(buf_hp);

    WriteLocal("LW", REG_HOSZIVATTYUK, GEPEK_SZAMA * 8, (void *)buf_hp, 0);



    // Save FIFO data

    normal_on_fifo.export(&normal_on_fifo, buff_fifo);

    WriteLocal("LW", REG_FIFO, GEPEK_SZAMA + 1, (void *)buff_fifo, 0);

    hmv_on_fifo.export(&hmv_on_fifo, buff_fifo);

    WriteLocal("LW", REG_FIFO + GEPEK_SZAMA + 1, GEPEK_SZAMA + 1, (void *)buff_fifo, 0);



#ifdef DEBUG

    WriteLocal("LW", 10, 1, (void *)&normal_requested_hp, 0);

    WriteLocal("LW", 11, 1, (void *)&hmv_requested_hp, 0);

    WriteLocal("LW", 12, 1, (void *)&norm_diff, 0);

    WriteLocal("LW", 13, 1, (void *)&hmv_diff, 0);

    WriteLocal("LW", 14, 1, (void *)&norm_min, 0);

    WriteLocal("LW", 15, 1, (void *)&hmv_min, 0);

    WriteLocal("LW", 16, 1, (void *)&err, 0);

#endif

}

 