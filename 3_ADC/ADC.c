
#include "DSP28x_Project.h"


// Variable global para almacenar el resultado del ADC
volatile Uint16 adcResult = 0;

// Prototipos
void InitSciaGpio(void);
void InitScia(void);
void scia_xmit(char a);
void scia_msg(char *msg);
void scia_sendUint16(Uint16 value);
__interrupt void adc_isr(void);

void main(void)
{
    InitSysCtrl();              // Inicializa reloj, PLL, watchdog

    DINT;                       // Deshabilita interrupciones globales
    InitPieCtrl();
    IER = 0x0000;
    IFR = 0x0000;
    InitPieVectTable();

    // === Configuración SCI-A ====
    InitSciaGpio();
    InitScia();
    scia_msg("Inicio del programa con interrupciones...\r\n");

    // === Configuración ADC ===
    InitAdc();
    AdcRegs.ADCTRL1.bit.ACQ_PS = 6;   // Tiempo de muestreo
    AdcRegs.ADCTRL3.bit.ADCCLKPS = 3; // Prescaler
    AdcRegs.ADCTRL1.bit.SEQ_CASC = 1; // Modo cascada
    AdcRegs.ADCCHSELSEQ1.bit.CONV00 = 0; // Canal ADCINA0

    // Configurar interrupción del ADC
    EALLOW;
    PieVectTable.ADCINT = &adc_isr;   // ISR de ADC
    EDIS;

    AdcRegs.ADCTRL2.bit.EPWM_SOCA_SEQ1 = 0; // No usar disparo de ePWM
    AdcRegs.ADCTRL2.bit.INT_ENA_SEQ1 = 1;   // Habilita interrupción SEQ1
    AdcRegs.ADCTRL2.bit.RST_SEQ1 = 1;       // Reinicia secuenciador

    // Habilitar interrupciones en PIE y CPU
    PieCtrlRegs.PIEIER1.bit.INTx6 = 1; // ADCINT en grupo 1, INT6
    IER |= M_INT1;                     // Habilita grupo 1
    EINT;                              // Habilita interrupciones globales
    ERTM;

    // === Bucle principal ===
    while(1)
    {
        // Forzar conversión manualmente
        AdcRegs.ADCTRL2.bit.SOC_SEQ1 = 1;
        DELAY_US(200000); // ~200ms entre conversiones
    }
}

// =============================
// ISR del ADC
// =============================
__interrupt void adc_isr(void)
{
    adcResult = AdcRegs.ADCRESULT0 >> 4; // 12 bits

    //scia_msg("ADC Value: ");
    scia_sendUint16(adcResult);
    scia_msg("\n");

    AdcRegs.ADCST.bit.INT_SEQ1_CLR = 1; // Limpia bandera ADC
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP1; // Reconoce interrupción
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

    // Baudrate = 9600 (LSPCLK=37.5MHz)
    SciaRegs.SCIHBAUD = 0x01;
    SciaRegs.SCILBAUD = 0xE7;

    SciaRegs.SCICTL1.all = 0x0023;  // Habilita SCI y libera reset
}

// =============================
// Funciones de transmisión
// =============================
void scia_xmit(char a)
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

    while(i > 0)
    {
        scia_xmit(digits[--i]);
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




