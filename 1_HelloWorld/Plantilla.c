#include "DSP28x_Project.h"


// On/off with delay ---------------------------------------------------------------------------------------

//void main(void){

//    InitSysCtrl();                                               // Initialize clock, watchdog, and peripherals

//    EALLOW;                                                      // Enable log configuration
//    GpioCtrlRegs.GPBMUX1.bit.GPIO34= 0;                          // GPIO Mode
//    GpioCtrlRegs.GPBDIR.bit.GPIO34 = 1;                          //
//    GpioCtrlRegs.GPAMUX2.bit.GPIO31= 0;                          // GPIO Mode
//    GpioCtrlRegs.GPADIR.bit.GPIO31 = 1;                          //
//    EDIS;                                                        // Disable logging settings

//    for(;;){
//        GpioDataRegs.GPBSET.bit.GPIO34 = 1;                      // Led On
//        DELAY_US(1000000);                                       // Wait (0.1s)
//        GpioDataRegs.GPBCLEAR.bit.GPIO34=1;                      // Led Off
//        DELAY_US(1000000);                                       // Wait (0.5s)
//        GpioDataRegs.GPASET.bit.GPIO31 = 1;                      // Led On
//        DELAY_US(1000000);                                       // Wait (0.1s)
//        GpioDataRegs.GPACLEAR.bit.GPIO31=1;                      // Led Off
//        DELAY_US(1000000);
//    }
//}



//On/off by timing and interruptions----------------------------------------------------------------------

__interrupt void cpu_timer0_isr(void);

void main(void){

    InitSysCtrl();                                               // Initialize clock, watchdog, and peripherals

    EALLOW;                                                      // Enable log configuration
                         //
    GpioCtrlRegs.GPAMUX2.bit.GPIO31= 0;                          // GPIO Mode
    GpioCtrlRegs.GPADIR.bit.GPIO31 = 1;                          //
    EDIS;

    DINT;
    InitPieCtrl();
    IER = 0x0000;
    IFR = 0x0000;
    InitPieVectTable();

    EALLOW;
    PieVectTable.TINT0 = &cpu_timer0_isr;
    EDIS;

    InitCpuTimers();
    ConfigCpuTimer(&CpuTimer0,150,500000);                    // Timer Selection, Clock DSP, Desired Time

    CpuTimer0Regs.TCR.all = 0x4001;                           // Enable interrupts and start timer


    //CpuTimer0Regs.TCR.bit.TSS = 0;                          // Start Timer
    //CpuTimer0Regs.TCR.bit.TRB = 1;                          // Top-up from PRD of "Desired Time"
    //CpuTimer0Regs.TCR.bit.TIE = 0;                          // Enable interruption

    IER |= M_INT1;                                            // Group 1 (Timer 0)
    PieCtrlRegs.PIEIER1.bit.INTx7 = 1;                        // INT1.7 = Timer0
    EINT;                                                     // Enable global interruptions
    ERTM;                                                     // Enable real-time interruptions

    for(;;){
        asm("NOP");
    }
}

__interrupt void cpu_timer0_isr(void){
    GpioDataRegs.GPATOGGLE.bit.GPIO31 = 1;
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP1;
}

