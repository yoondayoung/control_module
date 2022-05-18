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

/* Global variables */
uint8_t txData[]  = "SELF LOOPBACK DEMO FOR SPI!";
uint8_t rxData[sizeof(txData)];
volatile bool transferStatus=false;

/* This function will be called by SPI PLIB when transfer is completed */
void SPI1_Callback(uintptr_t context )
{
    transferStatus = true;
}

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
    GPIO_RH0_Set(); 
    state = 1; // state 1 : m- igbt on
    GPIO_RB12_Set(); 
    state = 2; // state 2 : m- igbt, m- relay on
}

void pc_on(){
    GPIO_RH1_Set();
    state = 3; // state 3 : m- igbt, m- relay, pc igbt on
}

void mp_igbt_relay_on(){
    GPIO_RH2_Set();
    state = 4; // state 4 : m- igbt, m- relay, pc igbt, m+ igbt on
    GPIO_RB13_Set();
    state = 5; // state 5 : m- igbt, m- relay, pc igbt, m+ igbt, m+ relay on
}

void pc_off(){
    GPIO_RH1_Clear();
    state = 6; // state 6 : m- igbt, m- relay, m+ igbt, m+ relay on
}

void mn_mp_relay_off(){
    GPIO_RB12_Clear(); 
    GPIO_RB13_Clear();
    state = 7; // state 7 : m- igbt, m- relay, m+ igbt, m+ relay on
}
void mn_mp_igbt_off(){
    GPIO_RH0_Toggle(); 
    GPIO_RH2_Toggle();
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
    bool signal = 1;
    int interval[4] = {150, 200, 150, 100};
    turnon_sequence[0] = mn_igbt_relay_on;
    turnon_sequence[1] = pc_on;
    turnon_sequence[2] = mp_igbt_relay_on;
    turnon_sequence[3] = pc_off;
    
    turnoff_sequence[0] = mn_mp_relay_off;
    turnoff_sequence[1] = mn_mp_igbt_off;
    
    SYS_Initialize ( NULL );
    
    /* Register callback function   */
    SPI1_CallbackRegister(SPI1_Callback, 0);

    /* SPI Write Read */
    SPI1_WriteRead(&txData, sizeof(txData), &rxData, sizeof(rxData));
    
    TMR2_CallbackRegister(TIMER2_InterruptSvcRoutine, (uintptr_t) NULL);
    TMR2_Start();
    turnon_sequence[0]();
    while ( true )
    {
        /* Maintain state machines of all polled MPLAB Harmony modules. */
        SYS_Tasks ( );
       
        if(transferStatus == true)
        {
            transferStatus = false;

            /* Compare received data with the transmitted data */
            if(memcmp(txData, rxData, sizeof(txData)) == 0)
            {
                /* Pass: Received data is same as transmitted data */
                printf("success\r\n");
            }
            else
            {
				/* Fail: Received data is not same as transmitted data */
                printf("fail\r\n");
            }
        }
        
        if (signal == 1){ // auto start
            if (state == 2 && (global_tick - prev_tick >= interval[0])){
                turnon_sequence[1]();
                prev_tick = global_tick;
            }
            else if (state == 3 && (global_tick - prev_tick >= interval[1])){
                turnon_sequence[2]();
                prev_tick = global_tick;
            }
            else if (state == 5 && (global_tick - prev_tick >= interval[2])){
                turnon_sequence[3]();
//                TMR2_Stop();
//                // initialize ticks
//                global_tick = 0;
                prev_tick = global_tick;
                signal = 0;
            }
        }
        else{ // stop
            if (state == 6 && (global_tick - prev_tick >= 200)){ // temporal value
                turnoff_sequence[0]();
                prev_tick = global_tick;
            }
            else if (state == 7 && (global_tick - prev_tick >= interval[3])){
                turnoff_sequence[1]();
                prev_tick = global_tick;
                TMR2_Stop();
            }
        }
    }

    /* Execution should not come here during normal operation */

    return ( EXIT_FAILURE );
}


/*******************************************************************************
 End of File
*/

