/*********************************************
 * Ver el ciclo de trabajo vía SCI
 *********************************************/
/*

#include "DSP2833x_Device.h"
#include "DSP2833x_Examples.h"
#include <stdlib.h>

// =============================
// Variables globales
// =============================

// Duty actual aplicado al PWM
volatile Uint16 duty = 50;

// Valor recibido por SCI (para depuración en CCS)
volatile Uint16 receivedValue = 0;

static char rxBuffer[8];
static int rxIndex = 0;

// =============================
// Prototipos
// =============================
void InitScia(void);
void InitEPwm1(void);
void scia_xmit(char a);
void scia_msg(char *msg);
void scia_sendUint16(Uint16 value);

__interrupt void scia_rx_isr(void);

// =============================
// Programa principal
// =============================
void main(void)
{
    InitSysCtrl();
    DINT;
    InitPieCtrl();
    IER = 0x0000;
    IFR = 0x0000;
    InitPieVectTable();

    // Mapear ISR SCI RX
    EALLOW;
    PieVectTable.SCIRXINTA = &scia_rx_isr;
    EDIS;

    // Inicializar periféricos
    InitScia();
    InitEPwm1();

    // Habilitar interrupción SCI RX
    PieCtrlRegs.PIEIER9.bit.INTx1 = 1;
    IER |= M_INT9;

    EINT;
    ERTM;

    scia_msg("SCI controla PWM\r\n");
    scia_msg("Ingrese duty (0-100) y presione Enter:\r\n");

    while(1)
    {
        // Calcular valor CMPA en 32 bits y asignar a 16 bits
        Uint32 temp = ((Uint32)EPwm1Regs.TBPRD * (Uint32)duty) / 100;
        EPwm1Regs.CMPA.half.CMPA = (Uint16)temp;
    }
}



// =============================
// ISR de recepción SCI
// =============================
__interrupt void scia_rx_isr(void)
{
    char rxChar;

    if(SciaRegs.SCIRXST.bit.RXRDY == 1)
    {
        rxChar = SciaRegs.SCIRXBUF.all;

        if(rxChar == '\r' || rxChar == '\n') // Enter  fin de número
        {
            rxBuffer[rxIndex] = '\0';
            receivedValue = atoi(rxBuffer);   // Guardar valor recibido
            if(receivedValue > 100) receivedValue = 100;

            duty = receivedValue;             // Actualizar duty
            rxIndex = 0;

            scia_msg("Nuevo duty: ");
            scia_sendUint16(duty);
            scia_msg("%\r\n");
        }
        else
        {
            if(rxIndex < 7)
            {
                rxBuffer[rxIndex++] = rxChar;
            }
        }
    }

    PieCtrlRegs.PIEACK.all = PIEACK_GROUP9;
}

// =============================
// Configuración SCI-A
// =============================
void InitScia(void)
{
    EALLOW;
    GpioCtrlRegs.GPAPUD.bit.GPIO28 = 0;
    GpioCtrlRegs.GPAPUD.bit.GPIO29 = 0;
    GpioCtrlRegs.GPAQSEL2.bit.GPIO28 = 3;
    GpioCtrlRegs.GPAMUX2.bit.GPIO28 = 1;
    GpioCtrlRegs.GPAMUX2.bit.GPIO29 = 1;
    EDIS;

    SciaRegs.SCICCR.all = 0x0007;
    SciaRegs.SCICTL1.all = 0x0003;
    SciaRegs.SCICTL2.bit.TXINTENA = 0;
    SciaRegs.SCICTL2.bit.RXBKINTENA = 1;

    // Baudrate = 9600 (LSPCLK=37.5MHz)
    SciaRegs.SCIHBAUD = 0x01;
    SciaRegs.SCILBAUD = 0xE7;

    SciaRegs.SCICTL1.all = 0x0023;
}

// =============================
// Configuración PWM1
// =============================
void InitEPwm1(void)
{
    EALLOW;
    GpioCtrlRegs.GPAMUX1.bit.GPIO0 = 1;   // GPIO0 = EPWM1A
    GpioCtrlRegs.GPADIR.bit.GPIO0 = 1;    // Dirección salida
    SysCtrlRegs.PCLKCR0.bit.TBCLKSYNC = 0;
    EDIS;

    EPwm1Regs.TBPRD = 1500;                // Periodo  ~10kHz
    EPwm1Regs.TBCTL.bit.CTRMODE = 0;       // Up-count
    EPwm1Regs.AQCTLA.bit.CAU = AQ_CLEAR;   // Clear en CMPA up
    EPwm1Regs.AQCTLA.bit.ZRO = AQ_SET;     // Set en Zero

    Uint32 temp = ((Uint32)EPwm1Regs.TBPRD * (Uint32)duty) / 100;
    EPwm1Regs.CMPA.half.CMPA = (Uint16)temp;

    EALLOW;
    SysCtrlRegs.PCLKCR0.bit.TBCLKSYNC = 1;
    EDIS;
}

// =============================
// Funciones de transmisión SCI
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
    char digits[6];
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





*/




/*********************************************
 * Modifica el ciclo de trabajo vía debuger
 *********************************************/

#include "DSP28x_Project.h"

// Variable global para el ciclo de trabajo (0 - 100 %)
volatile float dutyCycle = 50.0;   // % inicial

void main(void)
{
    InitSysCtrl();          // Inicializa reloj del sistema
    DINT;                   // Deshabilita interrupciones globales

    InitPieCtrl();
    IER = 0x0000;
    IFR = 0x0000;
    InitPieVectTable();

    // --- Configuración GPIO para PWM ---
    EALLOW;
    GpioCtrlRegs.GPAMUX1.bit.GPIO0 = 1;   // GPIO0 -> EPWM1A
    EDIS;

    // --- Configuración ePWM1 ---
    EALLOW;
    SysCtrlRegs.PCLKCR0.bit.TBCLKSYNC = 0; // Detiene los contadores para configurar
    EDIS;

    EPwm1Regs.TBPRD = 1500;                // Periodo (ejemplo: 20 kHz con SYSCLK=150 MHz)
    EPwm1Regs.TBCTL.bit.CTRMODE = 0;       // Modo up-count
    EPwm1Regs.TBCTL.bit.PHSEN = 0;         // Sin fase
    EPwm1Regs.TBCTL.bit.HSPCLKDIV = 0;     // Sin división
    EPwm1Regs.TBCTL.bit.CLKDIV = 0;

    EPwm1Regs.AQCTLA.bit.CAU = AQ_CLEAR;   // Clear en up-count
    EPwm1Regs.AQCTLA.bit.ZRO = AQ_SET;     // Set en zero

    // Inicializa duty
    EPwm1Regs.CMPA.half.CMPA = (Uint16)((dutyCycle/100.0) * EPwm1Regs.TBPRD);

    EALLOW;
    SysCtrlRegs.PCLKCR0.bit.TBCLKSYNC = 1; // Arranca los contadores
    EDIS;

    while(1)
    {
        // Aquí puedes modificar dutyCycle en tiempo real
        // dutyCycle = 75.0;  // ejemplo: cambiar a 75%

        EPwm1Regs.CMPA.half.CMPA = (Uint16)((dutyCycle/100.0) * EPwm1Regs.TBPRD);
        DELAY_US(1000); // Pequeño retardo
    }
}












