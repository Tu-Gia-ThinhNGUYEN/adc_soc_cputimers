//#############################################################################
//
// FILE:   adc_soc_cputimers.c
//
// TITLE:  ADC triggering via CPU timer for f28p65x.
//
//! \addtogroup bitfield_example_list
//! <h1> ADC CPU Timer Triggering </h1>
//!
//! This example configures CPU Timer0 to periodically trigger the ADC.
//!
//! \b External \b Connections \n
//!  - A0 should be connected to signals to convert
//!
//! \b Watch \b Variables \n
//!  - \b AdcaResults \b: A sequence of analog-to-digital conversion samples from
//! pin A0. The time between samples is determined based on the period
//! of the ePWM timer.
//!
//
//#############################################################################
//
//
// $Copyright:
// Copyright (C) 2022 Texas Instruments Incorporated - http://www.ti.com
//
// Redistribution and use in source and binary forms, with or without 
// modification, are permitted provided that the following conditions 
// are met:
// 
//   Redistributions of source code must retain the above copyright 
//   notice, this list of conditions and the following disclaimer.
// 
//   Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimer in the 
//   documentation and/or other materials provided with the   
//   distribution.
// 
//   Neither the name of Texas Instruments Incorporated nor the names of
//   its contributors may be used to endorse or promote products derived
//   from this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// $
//#############################################################################

//
// Included Files
//
#include "f28x_project.h"

//
// Function Prototypes
//
__interrupt void cpuTimer0ISR(void);
void ConfigureADC(void);
void ConfigureTimer(void);
void SetupADCTimer(Uint16 channel);
interrupt void adca1_isr(void);

//
// Defines
//
#define RESULTS_BUFFER_SIZE 256

//
// Globals
//
Uint16 AdcaResults[RESULTS_BUFFER_SIZE];
Uint16 resultsIndex;
volatile Uint16 bufferFull;
volatile float outputVoltage;

//
// Main
//
void main(void)
{
    //
    // Initialize device clock and peripherals
    //
    InitSysCtrl();

    //
    // Initialize GPIO
    //
    InitGpio();
    GPIO_SetupPinMux(31U, GPIO_MUX_CPU1, 0);
    GPIO_SetupPinOptions(31U, GPIO_OUTPUT, GPIO_PUSHPULL);

    //
    // Disable CPU interrupts
    //
    DINT;

    //
    // Initialize the PIE control registers to their default state.
    // The default state is all PIE interrupts disabled and flags
    // are cleared.
    //
    InitPieCtrl();

    //
    // Disable CPU interrupts and clear all CPU interrupt flags
    //
    IER = 0x0000;
    IFR = 0x0000;

    //
    // Initialize the PIE vector table with pointers to the shell Interrupt
    // Service Routines (ISR)
    //
    InitPieVectTable();

    //
    // Map ISR functions
    //
    EALLOW;
    PieVectTable.ADCA1_INT = &adca1_isr; //function for ADCA interrupt 1
    PieVectTable.TIMER0_INT = &cpuTimer0ISR;
    EDIS;

    //
    // Configure the ADC and power it up
    //
    ConfigureADC();

    //
    // Configure the CPU Timer 0
    //
    ConfigureTimer();

    //
    // Setup the ADC for CPU Timer 0 triggered conversions on channel 0
    //
    SetupADCTimer(0);

    //
    // Enable global Interrupts and higher priority real-time debug events
    //
    IER |= M_INT1; //Enable group 1 interrupts
    EINT;   // Enable Global interrupt INTM
    ERTM;   // Enable Global realtime interrupt DBGM

    //
    // Initialize results buffer
    //
        for(resultsIndex = 0; resultsIndex < RESULTS_BUFFER_SIZE; resultsIndex++)
        {
            AdcaResults[resultsIndex] = 0;
        }
        resultsIndex = 0;
        bufferFull = 0;

    //
    // Enable PIE interrupt
    //
        PieCtrlRegs.PIEIER1.bit.INTx1 = 1;

    //
    // Sync ePWM
    //
        EALLOW;
        CpuSysRegs.PCLKCR0.bit.TBCLKSYNC = 1;

    //
    // Take conversions indefinitely in loop
    //
        do
        {
            //
            // Start Timer
            //
//            CpuTimer0Regs.TCR.bit.TRB = 1;      //reload
//            CpuTimer0Regs.TCR.bit.TIE = 0;      //no interrupt
//            CpuTimer0Regs.TCR.bit.TSS = 0;      //start timer

            //
            // Wait while ePWM causes ADC conversions, which then cause interrupts,
            // which fill the results buffer, eventually setting the bufferFull
            // flag
            //
            while(!bufferFull);
            bufferFull = 0; //clear the buffer full flag
        } while(1);
}

//
// Configure CPU-Timer 0 to interrupt every 1 micro-second:
// 200MHz CPU Freq, 1 micro-second Period (in uSeconds)
//
void ConfigureTimer(void)
{
    //
    // Initialize the Device Peripheral. For this example, only initialize the
    // Cpu Timers.
    //
    InitCpuTimers();

    //
    // Configure CPU-Timer 0, 1, and 2 to interrupt every second:
    // 200MHz CPU Freq, 1 second Period (in uSeconds)
    //
    ConfigCpuTimer(&CpuTimer0, 200, 10);

    //
    // To ensure precise timing, use write-only instructions to write to the
    // entire register. Therefore, if any of the configuration bits are changed
    // in ConfigCpuTimer and InitCpuTimers, the below settings must also be
    // be updated.
    //
    CpuTimer0Regs.TCR.all = 0x4000;

    //
    // Enable CPU int1 which is connected to CPU-Timer 0, CPU int13
    // which is connected to CPU-Timer 1, and CPU int 14, which is connected
    // to CPU-Timer 2
    //
    IER |= M_INT1;

    //
    // Enable TINT0 in the PIE: Group 1 interrupt 7
    //
    PieCtrlRegs.PIEIER1.bit.INTx7 = 1;
}


//
// ConfigureADC - Write ADC configurations and power up the ADC for both
//                ADC A and ADC B
//
void ConfigureADC(void)
{
    EALLOW;

    //
    // Write configurations
    //
    AdcaRegs.ADCCTL2.bit.PRESCALE = 6; //set ADCCLK divider to /4
    AdcSetMode(ADC_ADCA, ADC_RESOLUTION_12BIT, ADC_SIGNALMODE_SINGLE);

    //
    // Set pulse positions to late
    //
    AdcaRegs.ADCCTL1.bit.INTPULSEPOS = 1;

    //
    // Power up the ADC
    //
    AdcaRegs.ADCCTL1.bit.ADCPWDNZ = 1;

    //
    // Delay for 1ms to allow ADC time to power up
    //
    DELAY_US(1000);

    EDIS;
}

//
// SetupADCTimer - Setup ADC Timer acquisition window
//
void SetupADCTimer(Uint16 channel)
{
    Uint16 acqps;

    //
    // Determine minimum acquisition window (in SYSCLKS) based on resolution
    //
    if(ADC_RESOLUTION_12BIT == AdcaRegs.ADCCTL2.bit.RESOLUTION)
    {
        acqps = 14; //75ns
    }
    else //resolution is 16-bit
    {
        acqps = 63; //320ns
    }

    //
    // Select the channels to convert and end of conversion flag
    //
    EALLOW;
    AdcaRegs.ADCSOC0CTL.bit.CHSEL = channel;  //SOC0 will convert pin A0
    AdcaRegs.ADCSOC0CTL.bit.ACQPS = acqps; //sample window is 100 SYSCLK cycles
    AdcaRegs.ADCSOC0CTL.bit.TRIGSEL = 1; //trigger on ePWM1 SOCA/C
    AdcaRegs.ADCINTSEL1N2.bit.INT1SEL = 0; //end of SOC0 will set INT1 flag
    AdcaRegs.ADCINTSEL1N2.bit.INT1E = 1;   //enable INT1 flag
    AdcaRegs.ADCINTFLGCLR.bit.ADCINT1 = 1; //make sure INT1 flag is cleared
    EDIS;
}

//
// cpuTimer0ISR - CPU Timer0 ISR with interrupt counter
//
__interrupt void cpuTimer0ISR(void)
{
    CpuTimer0.InterruptCount++;

    //
    // Acknowledge this interrupt to receive more interrupts from group 1
    //
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP1;
}

//
// adca1_isr - Read ADC Buffer in ISR
//
interrupt void adca1_isr(void)
{
    AdcaResults[resultsIndex++] = AdcaResultRegs.ADCRESULT0;

    outputVoltage = (float)AdcaResultRegs.ADCRESULT0*3.3*0.00024420024420024420024; //conversion ratio 0-4095 --> 0-3.3V

    if(RESULTS_BUFFER_SIZE <= resultsIndex)
    {
        resultsIndex = 0;
        bufferFull = 1;
    }

    AdcaRegs.ADCINTFLGCLR.bit.ADCINT1 = 1; //clear INT1 flag

    //
    // Check if overflow has occurred
    //
    if(1 == AdcaRegs.ADCINTOVF.bit.ADCINT1)
    {
        AdcaRegs.ADCINTOVFCLR.bit.ADCINT1 = 1; //clear INT1 overflow flag
        AdcaRegs.ADCINTFLGCLR.bit.ADCINT1 = 1; //clear INT1 flag
    }

    PieCtrlRegs.PIEACK.all = PIEACK_GROUP1;
}

//
// End of file
//
