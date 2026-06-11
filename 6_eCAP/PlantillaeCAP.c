
#include "DSP2833x_Device.h"
#include "DSP2833x_Examples.h"

#define SYSCLK_HZ     150000000UL
#define SYSCLK_MHZ    150.0

volatile Uint32 period_cycles = 0;
volatile Uint32 duty_cycles = 0;
volatile float frequencia = 0.0;
volatile float period_seconds = 0.0;
volatile float duty = 0.0;

__interrupt void ECAP1_ISR(void)
{
    // Leer valores capturados
    Uint32 rising_edge1 = ECap1Regs.CAP1;
    Uint32 falling_edge1 = ECap1Regs.CAP2;
    Uint32 rising_edge2 = ECap1Regs.CAP3;
    Uint32 falling_edge2 = ECap1Regs.CAP4;

    // Guardar valores en ciclos de reloj
    period_cycles = rising_edge2;
    duty_cycles = falling_edge1;

    // CALCULAR FRECUENCIA Y PERIODO EN SEGUNDOS
    if(period_cycles > 0) {
        period_seconds = ((float)period_cycles / SYSCLK_MHZ);
        frequencia = (SYSCLK_MHZ / (float)period_cycles)*(1000000);
        duty = ((float)duty_cycles / (float)period_cycles) * 100.0;
    }


    ECap1Regs.ECCLR.bit.CEVT4 = 1;
    ECap1Regs.ECCLR.bit.INT = 1;

    PieCtrlRegs.PIEACK.all = PIEACK_GROUP4;
}

void main(void)
{
    InitSysCtrl();
    DINT;
    InitPieCtrl();
    IER = 0x0000;
    IFR = 0x0000;
    InitPieVectTable();

    // Inicializar eCAP1
    InitECAP1();

    // Configurar ISR para eCAP1
    EALLOW;
    PieVectTable.ECAP1_INT = &ECAP1_ISR;
    EDIS;

    // Habilitar interrupciones
    IER |= M_INT4;
    PieCtrlRegs.PIEIER4.bit.INTx1 = 1;

    EINT;
    ERTM;

    while(1)
    {
        // Aquí puedes usar las variables:
        // frequency_hz - Frecuencia en Hertz
        // period_seconds - Periodo en segundos
        // duty_percent - Duty cycle en porcentaje

        // Ejemplo: pequeño delay
        DELAY_US(10000);
    }
}

void InitECAP1(void)
{
    // 1. Configurar GPIO para eCAP1
    EALLOW;
    GpioCtrlRegs.GPAMUX2.bit.GPIO24 = 1;  // eCAP1 en GPIO24
    EDIS;

    // 2. Configurar eCAP1 - Modo Captura
    ECap1Regs.ECCTL1.bit.CAP1POL = 0;    // Rising edge
    ECap1Regs.ECCTL1.bit.CAP2POL = 1;    // Falling edge
    ECap1Regs.ECCTL1.bit.CAP3POL = 0;    // Rising edge
    ECap1Regs.ECCTL1.bit.CAP4POL = 1;    // Falling edge

    // Modo diferencia - reset counter en rising edges
    ECap1Regs.ECCTL1.bit.CTRRST1 = 1;    // Reset después de CAP1
    ECap1Regs.ECCTL1.bit.CTRRST2 = 0;
    ECap1Regs.ECCTL1.bit.CTRRST3 = 1;    // Reset después de CAP3
    ECap1Regs.ECCTL1.bit.CTRRST4 = 0;

    ECap1Regs.ECCTL1.bit.PRESCALE = 0;   // Sin preescalado
    ECap1Regs.ECCTL1.bit.CAPLDEN = 1;    // Habilitar carga
    ECap1Regs.ECCTL1.bit.FREE_SOFT = 2;  // Free run en debug

    // 3. Configurar modo de operación
    ECap1Regs.ECCTL2.bit.CONT_ONESHT = 0;     // Modo continuo
    ECap1Regs.ECCTL2.bit.STOP_WRAP = 3;       // Circular buffer de 4
    ECap1Regs.ECCTL2.bit.REARM = 1;           // Rearmar
    ECap1Regs.ECCTL2.bit.TSCTRSTOP = 1;       // Iniciar contador
    ECap1Regs.ECCTL2.bit.SYNCI_EN = 0;        // Sin sync entrada
    ECap1Regs.ECCTL2.bit.SYNCO_SEL = 2;       // Sync out deshabilitado
    ECap1Regs.ECCTL2.bit.CAP_APWM = 0;        // Modo CAPTURA

    // 4. Habilitar interrupciones
    ECap1Regs.ECEINT.bit.CEVT4 = 1;

    // 5. Limpiar flags
    ECap1Regs.ECCLR.all = 0xFFFF;
}
