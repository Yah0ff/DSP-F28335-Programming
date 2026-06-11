//
// Medición a alta velocidad, con valores -/+ de vel y pos
//
#include "DSP2833x_Device.h"
#include "DSP2833x_Examples.h"

#define SYSCLK_HZ     150000000UL   // 150 MHz
#define ENCODER_PPR   1000          // Pulsos por revolución
#define SAMPLE_PERIOD 150000000        // 1 ms (150 MHz / 150000 = 1 kHz)

volatile int32 positionCounts = 0;   // Posición acumulada con signo
volatile int32 posPrev = 0;
volatile float motorSpeed_rps = 0.0; // Velocidad en rev/s

__interrupt void EQEP1_isr(void)
{
    EQep1Regs.QCLR.bit.UTO = 0;
    int32 posNow = EQep1Regs.QPOSCNT;
    int32 delta = posNow - posPrev;

    // Manejo de rollover
    if(delta > (ENCODER_PPR*2)) {
        delta -= (ENCODER_PPR*4);
    } else if(delta < -(ENCODER_PPR*2)) {
        delta += (ENCODER_PPR*4);
    }

    positionCounts += delta;   // Posición extendida
    //positionCounts = positionCounts+1;
    posPrev = posNow;

    // Velocidad en rev/s = entas / (cuentas_por_rev * Ts)
    float Ts = 0.001f; // 1 ms
    motorSpeed_rps = ((float)delta / (ENCODER_PPR*4.0f)) / Ts;

    // Limpia bandera de unit timer overflow
    //EQep1Regs.QCLR.bit.UTO = 0xFFFF;
    EQep1Regs.QCLR.all = 0xFFFF;
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP5;
}

void main(void)
{
    InitSysCtrl();
    DINT;
    InitPieCtrl();
    IER = 0x0000;
    IFR = 0x0000;
    InitPieVectTable();

    // Configura GPIOs para eQEP1
    EALLOW;
    GpioCtrlRegs.GPAMUX2.bit.GPIO20 = 1; // EQEP1A
    GpioCtrlRegs.GPAMUX2.bit.GPIO21 = 1; // EQEP1B
    GpioCtrlRegs.GPAMUX2.bit.GPIO23 = 1; // EQEP1I
    EDIS;

    // Configuración básica de eQEP1
    EQep1Regs.QDECCTL.all = 0x0000;                  // Modo cuadratura
    EQep1Regs.QPOSMAX = (ENCODER_PPR*4 - 1);         // 4000 cuentas para 1000 PPR
    EQep1Regs.QPOSCNT = 0;                           // Inicializa en cero
    EQep1Regs.QEPCTL.bit.FREE_SOFT = 2;              // Free run en debug
    EQep1Regs.QEPCTL.bit.QPEN = 1;                   // Habilita QEP

    // Configuración del unit timer
    EQep1Regs.QUPRD = SAMPLE_PERIOD;     // Periodo del unit timer (1 ms)
    EQep1Regs.QEPCTL.bit.UTE = 1;        // Habilita unit timer
    EQep1Regs.QEPCTL.bit.QCLM = 1;       // Latch en unit time out
    EQep1Regs.QEINT.bit.UTO = 1;         // Habilita interrupción por unit timer

    // Configura ISR
    EALLOW;
    PieVectTable.EQEP1_INT = &EQEP1_isr;
    EDIS;
    IER |= M_INT5;
    PieCtrlRegs.PIEIER5.bit.INTx1 = 1;
    EINT;   // Habilita interrupciones globales
    ERTM;

    while(1)
    {
        // nada
    }
}

/**************************
 * ADC con SCI
 **************************/

/*
#include "DSP28x_Project.h"

// Variable global para ADC
volatile Uint16 adcResult = 0;

// Prototipos
void InitSciaGpio(void);
void InitScia(void);
void scia_xmit(char a);
void scia_msg(char *msg);
void scia_sendUint16(Uint16 value);

void main(void)
{
    InitSysCtrl();              // Inicializa reloj, PLL, watchdog
    DINT;                       // Deshabilita interrupciones globales
    InitPieCtrl();
    IER = 0x0000;
    IFR = 0x0000;
    InitPieVectTable();

    // === Configuración ADC ===
    InitAdc();
    AdcRegs.ADCTRL1.bit.ACQ_PS = 6;   // Tiempo de muestreo
    AdcRegs.ADCTRL3.bit.ADCCLKPS = 3; // Prescaler
    AdcRegs.ADCTRL1.bit.SEQ_CASC = 1; // Modo cascada
    AdcRegs.ADCCHSELSEQ1.bit.CONV00 = 0; // Canal ADCINA0

    // === Configuración SCI-A ===
    InitSciaGpio();
    InitScia();

    scia_msg("Inicio del programa...\r\n");

    while(1)
    {
        // Forzar conversión
        AdcRegs.ADCTRL2.bit.SOC_SEQ1 = 1;
        while(AdcRegs.ADCST.bit.INT_SEQ1 == 0);
        AdcRegs.ADCST.bit.INT_SEQ1_CLR = 1;

        adcResult = AdcRegs.ADCRESULT0 >> 4; // 12 bits (0–4095)

        // Enviar por SCI
        scia_msg("ADC Value: ");
        scia_sendUint16(adcResult);
        scia_msg("\r\n");

        DELAY_US(200000); // ~200ms
    }
}

// =============================
// Configuración SCI-A
// =============================
void InitSciaGpio(void)
{
    EALLOW;
    GpioCtrlRegs.GPAPUD.bit.GPIO28 = 0;   // Pull-up en GPIO28 (SCIRXDA)
    GpioCtrlRegs.GPAPUD.bit.GPIO29 = 0;   // Pull-up en GPIO29 (SCITXDA)
    GpioCtrlRegs.GPAQSEL2.bit.GPIO28 = 3; // Asíncrono para RX
    GpioCtrlRegs.GPAMUX2.bit.GPIO28 = 1;  // GPIO28 = SCIRXDA
    GpioCtrlRegs.GPAMUX2.bit.GPIO29 = 1;  // GPIO29 = SCITXDA
    EDIS;
}

void InitScia(void)
{
    SciaRegs.SCICCR.all = 0x0007;   // 1 stop bit, sin paridad, 8 bits
    SciaRegs.SCICTL1.all = 0x0003;  // Habilita TX, RX
    SciaRegs.SCICTL2.bit.TXINTENA = 0;
    SciaRegs.SCICTL2.bit.RXBKINTENA = 0;

    // Baudrate = LSPCLK/(BRR+1)*8
    // Suponiendo LSPCLK = 37.5 MHz  para 9600 baudios:
    // BRR  487  0x01E7
    SciaRegs.SCIHBAUD = 0x01;
    SciaRegs.SCILBAUD = 0xE7;

    SciaRegs.SCICTL1.all = 0x0023;  // Habilita SCI y libera reset
}

// =============================
// Funciones de transmisión
// =============================
void scia_xmit(char a)
{
    while (SciaRegs.SCICTL2.bit.TXRDY == 0); // Espera buffer libre
    SciaRegs.SCITXBUF = a;
}

void scia_msg(char *msg)
{
    int i;
    for(i=0; msg[i] != '\0'; i++)
    {
        scia_xmit(msg[i]);
    }
}

// Convierte un entero de 0–4095 a ASCII y lo envía
void scia_sendUint16(Uint16 value)
{
    char digits[5];
    int i = 0;

    if(value == 0)
    {
        scia_xmit('0');
        return;
    }

    while(value > 0)
    {
        digits[i++] = (value % 10) + '0';
        value /= 10;
    }

    // Enviar en orden correcto
    while(i > 0)
    {
        scia_xmit(digits[--i]);
    }
}

*/





/**************************
 * ADC con printf (no recomendable)
 **************************/

/*
 *
#include "DSP28x_Project.h"     // Cabecera principal del F28335
#include <stdio.h>

// Variable global para ADC
volatile Uint16 adcResult = 0;

// Prototipos
void InitSciaGpio(void);
void InitScia(void);
void scia_xmit(int a);
void scia_msg(char *msg);

void main(void)
{
    InitSysCtrl();              // Inicializa reloj, PLL, watchdog
    DINT;                       // Deshabilita interrupciones globales
    InitPieCtrl();
    IER = 0x0000;
    IFR = 0x0000;
    InitPieVectTable();

    // === Configuración ADC ===
    InitAdc();
    AdcRegs.ADCTRL1.bit.ACQ_PS = 6;   // Tiempo de muestreo
    AdcRegs.ADCTRL3.bit.ADCCLKPS = 3; // Prescaler
    AdcRegs.ADCTRL1.bit.SEQ_CASC = 1; // Modo cascada
    AdcRegs.ADCCHSELSEQ1.bit.CONV00 = 0; // Canal ADCINA0

    // === Configuración SCI-A ===
    InitSciaGpio();
    InitScia();

    scia_msg("Inicio del programa...\r\n");

    while(1)
    {
        // Forzar conversión
        AdcRegs.ADCTRL2.bit.SOC_SEQ1 = 1;
        while(AdcRegs.ADCST.bit.INT_SEQ1 == 0);
        AdcRegs.ADCST.bit.INT_SEQ1_CLR = 1;

        adcResult = AdcRegs.ADCRESULT0 >> 4; // 12 bits

        // Enviar por SCI
        char buffer[32];
        sprintf(buffer, "ADC Value: %u\r\n", adcResult);
        scia_msg(buffer);

        DELAY_US(200000); // ~200ms
    }
}

// =============================
// Configuración SCI-A
// =============================
void InitSciaGpio(void)
{
    EALLOW;
    GpioCtrlRegs.GPAPUD.bit.GPIO28 = 0;   // Habilita pull-up en GPIO28 (SCIRXDA)
    GpioCtrlRegs.GPAPUD.bit.GPIO29 = 0;   // Habilita pull-up en GPIO29 (SCITXDA)
    GpioCtrlRegs.GPAQSEL2.bit.GPIO28 = 3; // Asíncrono para RX
    GpioCtrlRegs.GPAMUX2.bit.GPIO28 = 1;  // GPIO28 = SCIRXDA
    GpioCtrlRegs.GPAMUX2.bit.GPIO29 = 1;  // GPIO29 = SCITXDA
    EDIS;
}

void InitScia(void)
{
    SciaRegs.SCICCR.all = 0x0007;   // 1 stop bit, sin paridad, 8 bits
    SciaRegs.SCICTL1.all = 0x0003;  // Habilita TX, RX
    SciaRegs.SCICTL2.bit.TXINTENA = 0;
    SciaRegs.SCICTL2.bit.RXBKINTENA = 0;

    // Baudrate = LSPCLK/(BRR+1)*8
    // Suponiendo LSPCLK = 37.5 MHz  para 9600 baudios:
    // BRR = (LSPCLK/(Baud*8)) - 1 aprox 487
    SciaRegs.SCIHBAUD = 0x01;
    SciaRegs.SCILBAUD = 0xE7;

    SciaRegs.SCICTL1.all = 0x0023;  // Habilita SCI y libera reset
}

// =============================
// Funciones de transmisión
// =============================
void scia_xmit(int a)
{
    while (SciaRegs.SCICTL2.bit.TXRDY == 0);
    SciaRegs.SCITXBUF = a;
}

void scia_msg(char *msg)
{
    int i;
    for(i=0; msg[i] != '\0'; i++)
    {
        scia_xmit(msg[i]);
    }
}
*/




