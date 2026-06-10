#include "DSP28x_Project.h"

//void main(void){

//    InitSysCtrl();                                               // Inicializar reloj, watchdog y prefifericos

//    EALLOW;                                                      // Habilitar la configuración de registros
//    GpioCtrlRegs.GPBMUX1.bit.GPIO34= 0;                          // Modo GPIO
//    GpioCtrlRegs.GPBDIR.bit.GPIO34 = 1;                          //
//    GpioCtrlRegs.GPAMUX2.bit.GPIO31= 0;                          // Modo GPIO
//    GpioCtrlRegs.GPADIR.bit.GPIO31 = 1;                          //
//    EDIS;                                                        // Deshabilitar la configuración de registros

//    for(;;){
//        GpioDataRegs.GPBSET.bit.GPIO34 = 1;                      // Led On
//        DELAY_US(1000000);                                        // Espera (0.1s)
//        GpioDataRegs.GPBCLEAR.bit.GPIO34=1;                      // Led Off
//        DELAY_US(1000000);                                        // Espera (0.5s)
//        GpioDataRegs.GPASET.bit.GPIO31 = 1;                      // Led On
//        DELAY_US(1000000);                                        // Espera (0.1s)
//        GpioDataRegs.GPACLEAR.bit.GPIO31=1;                      // Led Off
//        DELAY_US(1000000);
//    }
//}



__interrupt void cpu_timer0_isr(void);

void main(void){

    InitSysCtrl();                                               // Inicializar reloj, watchdog y prefifericos

    EALLOW;                                                      // Habilitar la configuración de registros
                         //
    GpioCtrlRegs.GPAMUX2.bit.GPIO31= 0;                          // Modo GPIO
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
    ConfigCpuTimer(&CpuTimer0,150,500000);                    // Seleccion de Timer , Clock DSP , Tiempo deseado

    CpuTimer0Regs.TCR.all = 0x4001;                           // Habilitar interrupciones y arrancar timer


    //CpuTimer0Regs.TCR.bit.TSS = 0;                          // Arrancar Timer
    //CpuTimer0Regs.TCR.bit.TRB = 1;                          // Recarga desde PRD de "Tiempo Deseado"
    //CpuTimer0Regs.TCR.bit.TIE = 0;                          // Habilitar interrupción

    IER |= M_INT1;                                            // Grupo 1 (Timer 0)
    PieCtrlRegs.PIEIER1.bit.INTx7 = 1;                        // INT1.7 = Timer0
    EINT;                                                     // Habilitar interrupciones globales
    ERTM;                                                     // Habilitar interrpciones en tiempo real

    for(;;){
        asm("NOP");
    }
}

__interrupt void cpu_timer0_isr(void){
    GpioDataRegs.GPATOGGLE.bit.GPIO31 = 1;
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP1;
}

