/*******************************************************************************
  Main Source File

  Company:
    Microchip Technology Inc.

  File Name:
    main.c

  Summary:
    This file contains the "main" function for a project.

  Description:
    This file contains the "main" function for a project.  The
    "main" function calls the "SYS_Initialize" function to initialize the state
    machines of all modules in the system
 *******************************************************************************/

// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************

#include <stddef.h>                     // Defines NULL
#include <stdbool.h>                    // Defines true
#include <stdlib.h>                     // Defines EXIT_FAILURE
#include "definitions.h"                // SYS function prototypes
#include <stdio.h>
#include <string.h>

#define DEBUG 1

// *****************************************************************************
// *****************************************************************************
// Section: Main Entry Point
// *****************************************************************************
// *****************************************************************************
volatile uint32_t global_tick = 0; // 1 tick = 10ms
volatile int state = 0;
void (*turnon_sequence[4])();
void (*turnoff_sequence[2])();

void mn_igbt_relay_on(){
    MN_IGBT_Set();
#if DEBUG==1
    GPIO_RH0_Set();
#endif
    state = 1; // state 1 : m- igbt on
    MN_Relay_Set();
#if DEBUG==1
    GPIO_RB12_Set();
#endif
    state = 2; // state 2 : m- igbt, m- relay on
}

void pc_on(){
    PC_IGBT_Set();
#if DEBUG==1
    GPIO_RH1_Set();
#endif
    state = 3; // state 3 : m- igbt, m- relay, pc igbt on
}

void mp_igbt_relay_on(){
    MP_IGBT_Set();
#if DEBUG==1
    GPIO_RH2_Set();
#endif
    state = 4; // state 4 : m- igbt, m- relay, pc igbt, m+ igbt on
    MP_Relay_Set();
#if DEBUG==1
    GPIO_RB13_Set();
#endif
    state = 5; // state 5 : m- igbt, m- relay, pc igbt, m+ igbt, m+ relay on
}

void pc_off(){
    PC_IGBT_Clear();
#if DEBUG==1
    GPIO_RH1_Clear();
#endif
    state = 6; // state 6 : m- igbt, m- relay, m+ igbt, m+ relay on
}

void mn_mp_relay_off(){
    MN_Relay_Clear();
#if DEBUG==1
    GPIO_RB12_Clear();
#endif
    MP_Relay_Clear();
#if DEBUG==1
    GPIO_RB13_Clear();
#endif
    state = 7; // state 7 : m- igbt, m- relay, m+ igbt, m+ relay on
}
void mn_mp_igbt_off(){
    MN_IGBT_Clear();
#if DEBUG==1
    GPIO_RH0_Clear();
#endif
    MP_IGBT_Clear();
#if DEBUG==1
    GPIO_RH2_Clear();
#endif
    state = 0; // state 0
}

/* This function is called after period matches in Timer 2 (32-bit timer) */
void TIMER2_InterruptSvcRoutine(uint32_t status, uintptr_t context)
{   
    global_tick++;
}

int main ( void )
{
    /* Initialize all modules */
    uint32_t prev_tick = 0; // record previous step's tick
    bool signal = 0;
#if DEBUG==1
    bool signal = 1;
#endif
    int interval[4] = {150, 200, 150, 100};
    turnon_sequence[0] = mn_igbt_relay_on;
    turnon_sequence[1] = pc_on;
    turnon_sequence[2] = mp_igbt_relay_on;
    turnon_sequence[3] = pc_off;
    
    turnoff_sequence[0] = mn_mp_relay_off;
    turnoff_sequence[1] = mn_mp_igbt_off;
    
    SYS_Initialize ( NULL );
    
    TMR2_CallbackRegister(TIMER2_InterruptSvcRoutine, (uintptr_t) NULL);
    TMR2_Start();

    while ( true )
    {
        /* Maintain state machines of all polled MPLAB Harmony modules. */
        SYS_Tasks ( );
        
        if (signal == 1){ // auto start
            if (state == 0){
                turnon_sequence[0]();
                prev_tick = global_tick;
            }
            else if (state == 2 && (global_tick - prev_tick >= interval[0])){
                turnon_sequence[1]();
                prev_tick = global_tick;
            }
            else if (state == 3 && (global_tick - prev_tick >= interval[1])){
                turnon_sequence[2]();
                prev_tick = global_tick;
            }
            else if (state == 5 && (global_tick - prev_tick >= interval[2])){
                turnon_sequence[3]();
                prev_tick = global_tick;
#if DEBUG==1
                signal = 0;
#endif
            }
        }
        else{ // stop
            if (state == 6 && (global_tick - prev_tick >= 200)){ // temporal value
                turnoff_sequence[0]();
                prev_tick = global_tick;
            }
            else if (state == 7 && (global_tick - prev_tick >= interval[3])){
                turnoff_sequence[1]();
                global_tick = 0;
                prev_tick = global_tick;
#if DEBUG==1
                signal = 1;
#endif
            }
        }
    }

    /* Execution should not come here during normal operation */

    return ( EXIT_FAILURE );
}


/*******************************************************************************
 End of File
*/

