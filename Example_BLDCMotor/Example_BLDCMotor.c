#include "DSP28x_Project.h"
#include "DSP2833x_Device.h"
#include "DSP2833x_Examples.h"
#include <stdlib.h>

// ===============================================================
//                        Global variables
// ===============================================================

volatile Uint16 duty = 50;                                                     // Current duty applied to PWM

volatile Uint16 receivedValue = 0;                                             // Value received by SCI (for debugging in CCS)

volatile Uint32 A1 = 0;                                                        // Variables to turn MOSFETs on or off
volatile Uint32 B1 = 0;
volatile Uint32 A2 = 0;
volatile Uint32 B2 = 0;
volatile Uint32 A3 = 0;
volatile Uint32 B3 = 0;

static char rxBuffer[8];                                                       // Data transmission and SCI
static int rxIndex = 0;

volatile Uint16 adcResult = 0;                                                 // Global variable for ADC and sign handling
volatile float Mdes = 0.0f;
volatile float Mdess = 0.0f;

// ===============================================================
//                             Prototypes
// ===============================================================
void InitScia(void);
void InitEPwm1(void);
void scia_xmit(char a);
void scia_msg(char *msg);
void scia_sendUint16(Uint16 value);
void scia_sendFloat(float value, int decimalPlaces);

__interrupt void scia_rx_isr(void);

void manejarCasosHall(Uint16 ha, Uint16 hb, Uint16 hc) {                         // Function to handle variables

    Uint16 caso = (hc << 2) | (hb << 1) | ha;                                    // Combine in HC-HB-HA order

    switch(caso) {                                                               // 000 - HA=0, HB=0, HC=0 All INACTIVE
        case 0:
            if(Mdes>0){
                A1 = 1; B1 = 0;
                A2 = 0; B2 = 0;
                A3 = 0; B3 = 1;
            } else {
                A1 = 0; B1 = 0;
                A2 = 0; B2 = 1;
                A3 = 1; B3 = 0;
            }

            break;

        case 1:                                                                     // 001 - HA=1, HB=0, HC=0 Only HA ACTIVE
            if(Mdes>0){
                A1 = 1; B1 = 0;
                A2 = 0; B2 = 1;
                A3 = 0; B3 = 0;
            } else {
                A1 = 0; B1 = 1;
                A2 = 0; B2 = 0;
                A3 = 1; B3 = 0;
            }
            break;

        case 2:                                                                     // 010 - HA=0, HB=1, HC=0 Only HB ACTIVE
            if(Mdes>0){
                A1 = 0; B1 = 0;
                A2 = 1; B2 = 0;
                A3 = 0; B3 = 1;
            } else {
                A1 = 1; B1 = 0;
                A2 = 0; B2 = 1;
                A3 = 0; B3 = 0;
            }

            break;

        case 3:                                                                     // 011 - HA=1, HB=1, HC=0 HA and HB ACTIVE

            break;

        case 4:                                                                     // 100 - HA=0, HB=0, HC=1 Only HC ACTIVE


            break;

        case 5:                                                                     // 101 - HA=1, HB=0, HC=1 HA and HC ACTIVE
            if(Mdes>0){
                 A1 = 0; B1 = 0;
                 A2 = 0; B2 = 1;
                 A3 = 1; B3 = 0;
             } else {
                 A1 = 0; B1 = 1;
                 A2 = 1; B2 = 0;
                 A3 = 0; B3 = 0;
                        }

            break;

        case 6:                                                                     // 110 - HA=0, HB=1, HC=1 HB and HC ACTIVE
            if(Mdes>0){
                A1 = 0; B1 = 1;
                A2 = 1; B2 = 0;
                A3 = 0; B3 = 0;
            } else {
                A1 = 1; B1 = 0;
                A2 = 0; B2 = 0;
                A3 = 0; B3 = 1;
            }

            break;

        case 7:                                                                     // 111 - HA=1, HB=1, HC=1 All ACTIVE
            if(Mdes>0){
                A1 = 0; B1 = 1;
                A2 = 0; B2 = 0;
                A3 = 1; B3 = 0;
            } else {
                A1 = 0; B1 = 0;
                A2 = 1; B2 = 0;
                A3 = 0; B3 = 1;
            }

            break;
    }


    if(fabsf(Mdes) < 2.0f) {
        A1 = B1 = A2 = B2 = A3 = B3 = 0;
    }

        //scia_msg(" | Case=");
        //scia_sendUint16(caso);
        //scia_msg("\r\n");
        //scia_sendUint16(A1);
        //scia_sendUint16(A2);
        //scia_sendUint16(A3);
        //scia_sendUint16(B1);
        //scia_sendUint16(B2);
        //scia_sendUint16(B3);
        //scia_msg("\r\n");
}

void main(void){

    InitSysCtrl();
    DINT;
    InitPieCtrl();
    IER = 0x0000;
    IFR = 0x0000;
    InitPieVectTable();
    EALLOW;

                                                                                        // === Configuration ===
    InitAdc();
    AdcRegs.ADCTRL1.bit.ACQ_PS = 6;                                                     // Sampling time
    AdcRegs.ADCTRL3.bit.ADCCLKPS = 3;                                                   // Prescaler
    AdcRegs.ADCTRL1.bit.SEQ_CASC = 1;                                                   // Cascade mode
    AdcRegs.ADCCHSELSEQ1.bit.CONV00 = 0;                                                // ADCINA0 channel

    EALLOW;                                                                              // Map SCI RX ISR
    PieVectTable.SCIRXINTA = &scia_rx_isr;
    EDIS;

    InitScia();                                                                         // Initialize peripherals
    InitEPwm1();

    PieCtrlRegs.PIEIER9.bit.INTx1 = 1;                                                  // Enable SCI RX interrupt
    IER |= M_INT9;

    EINT;
    ERTM;

    GpioCtrlRegs.GPAMUX2.bit.GPIO31 = 0;                                                 // GPIO31 as output (LED)
    GpioCtrlRegs.GPADIR.bit.GPIO31 = 1;

    // HA - GPIO34 as input WITH PULL-UP
    GpioCtrlRegs.GPBMUX1.bit.GPIO34 = 0;
    GpioCtrlRegs.GPBDIR.bit.GPIO34 = 0;
    GpioCtrlRegs.GPBPUD.bit.GPIO34 = 0;

    // HB - GPIO32 as input WITH PULL-UP
    GpioCtrlRegs.GPBMUX1.bit.GPIO32 = 0;
    GpioCtrlRegs.GPBDIR.bit.GPIO32 = 0;
    GpioCtrlRegs.GPBPUD.bit.GPIO32 = 0;

    // HC - GPIO30 as input WITH PULL-UP
    GpioCtrlRegs.GPAMUX2.bit.GPIO30 = 0;
    GpioCtrlRegs.GPADIR.bit.GPIO30 = 0;
    GpioCtrlRegs.GPAPUD.bit.GPIO30 = 0;
    EDIS;

    // Turn off LED initially
    GpioDataRegs.GPACLEAR.bit.GPIO31 = 1;

    for(;;){
        AdcRegs.ADCTRL2.bit.SOC_SEQ1 = 1;                                                   // Force conversion
        while(AdcRegs.ADCST.bit.INT_SEQ1 == 0);
        AdcRegs.ADCST.bit.INT_SEQ1_CLR = 1;

         adcResult = AdcRegs.ADCRESULT0 >> 4;                                               // 12 bits (0–4095)
         //adcResult = (adcResult/4095);

         Mdes = (((float)adcResult * 2.0f / 4095.0f)-1)*90;
         float Mdess = fabsf(Mdes);
         int signo = (Mdes >= 0) ? 1 : -1;

         duty = (Uint32)Mdess;

        // Send via SCI
        //scia_msg("ADC Value: ");
        //scia_sendFloat(Mdes, 2);
        //scia_msg("\r\n");

        Uint16 ha = GpioDataRegs.GPBDAT.bit.GPIO34;                                                // Read states (1 = inactive, 0 = active)
        Uint16 hb = GpioDataRegs.GPBDAT.bit.GPIO32;
        Uint16 hc = GpioDataRegs.GPADAT.bit.GPIO30;

       manejarCasosHall(ha, hb, hc);                                                              // Call function that handles all cases

        // Update EPWM1 - GPIO0 and GPIO1
        Uint32 temp1A = (((Uint32)EPwm1Regs.TBPRD * (Uint32)duty) / 100)*A1;
        EPwm1Regs.CMPA.half.CMPA = (Uint16)temp1A;

        Uint32 temp1B = (((Uint32)EPwm1Regs.TBPRD * (Uint32)duty) / 100)*B1;
        EPwm1Regs.CMPB = (Uint16)temp1B;

        // Update EPWM2 - GPIO2 and GPIO3
        Uint32 temp2A = (((Uint32)EPwm2Regs.TBPRD * (Uint32)duty) / 100)*A2;
        EPwm2Regs.CMPA.half.CMPA = (Uint16)temp2A;

        Uint32 temp2B = (((Uint32)EPwm2Regs.TBPRD * (Uint32)duty) / 100)*B2;
        EPwm2Regs.CMPB = (Uint16)temp2B;

        // Update EPWM3 - GPIO4 and GPIO5
        Uint32 temp3A = (((Uint32)EPwm3Regs.TBPRD * (Uint32)duty) / 100)*A3;
        EPwm3Regs.CMPA.half.CMPA = (Uint16)temp3A;

        Uint32 temp3B = (((Uint32)EPwm3Regs.TBPRD * (Uint32)duty) / 100)*B3;
        EPwm3Regs.CMPB = (Uint16)temp3B;
    }
}

// =============================
// SCI reception ISR
// =============================
__interrupt void scia_rx_isr(void)
{
    char rxChar;

    if(SciaRegs.SCIRXST.bit.RXRDY == 1)
    {
        rxChar = SciaRegs.SCIRXBUF.all;

        if(rxChar == '\r' || rxChar == '\n') // Enter - end of number
        {
            rxBuffer[rxIndex] = '\0';
            receivedValue = atoi(rxBuffer);   // Store received value
            if(receivedValue > 100) receivedValue = 100;

            duty = receivedValue;             // Update duty
            rxIndex = 0;

            scia_msg("New duty: ");
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
// SCI-A Configuration
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
// PWM1 Configuration
// =============================
void InitEPwm1(void)
{

    EALLOW;
    SysCtrlRegs.PCLKCR0.bit.TBCLKSYNC = 0;
    EDIS;

    // Configure actions - FORCE INITIAL LOW STATE
       // EPWM1
       EPwm1Regs.AQCTLA.bit.PRD = AQ_CLEAR;   // Low at PRD (start)
       EPwm1Regs.AQCTLA.bit.CAU = AQ_SET;     // High at CAU
       EPwm1Regs.AQCTLA.bit.ZRO = AQ_CLEAR;   // Low at ZRO

       EPwm1Regs.AQCTLB.bit.PRD = AQ_CLEAR;   // Low at PRD (start)
       EPwm1Regs.AQCTLB.bit.CBU = AQ_SET;     // High at CBU
       EPwm1Regs.AQCTLB.bit.ZRO = AQ_CLEAR;   // Low at ZRO

       // EPWM2
       EPwm2Regs.AQCTLA.bit.PRD = AQ_CLEAR;
       EPwm2Regs.AQCTLA.bit.CAU = AQ_SET;
       EPwm2Regs.AQCTLA.bit.ZRO = AQ_CLEAR;

       EPwm2Regs.AQCTLB.bit.PRD = AQ_CLEAR;
       EPwm2Regs.AQCTLB.bit.CBU = AQ_SET;
       EPwm2Regs.AQCTLB.bit.ZRO = AQ_CLEAR;

       // EPWM3
       EPwm3Regs.AQCTLA.bit.PRD = AQ_CLEAR;
       EPwm3Regs.AQCTLA.bit.CAU = AQ_SET;
       EPwm3Regs.AQCTLA.bit.ZRO = AQ_CLEAR;

       EPwm3Regs.AQCTLB.bit.PRD = AQ_CLEAR;
       EPwm3Regs.AQCTLB.bit.CBU = AQ_SET;
       EPwm3Regs.AQCTLB.bit.ZRO = AQ_CLEAR;


    EALLOW;
    // EPWM1 - GPIO0 and GPIO1
    GpioCtrlRegs.GPAMUX1.bit.GPIO0 = 1;   // GPIO0 = EPWM1A
    GpioCtrlRegs.GPADIR.bit.GPIO0 = 1;    // Output
    GpioCtrlRegs.GPAMUX1.bit.GPIO1 = 1;   // GPIO1 = EPWM1B
    GpioCtrlRegs.GPADIR.bit.GPIO1 = 1;    // Output

    // EPWM2 - GPIO2 and GPIO3
    GpioCtrlRegs.GPAMUX1.bit.GPIO2 = 1;   // GPIO2 = EPWM2A
    GpioCtrlRegs.GPADIR.bit.GPIO2 = 1;    // Output
    GpioCtrlRegs.GPAMUX1.bit.GPIO3 = 1;   // GPIO3 = EPWM2B
    GpioCtrlRegs.GPADIR.bit.GPIO3 = 1;    // Output

    // EPWM3 - GPIO4 and GPIO5
    GpioCtrlRegs.GPAMUX1.bit.GPIO4 = 1;   // GPIO4 = EPWM3A
    GpioCtrlRegs.GPADIR.bit.GPIO4 = 1;    // Output
    GpioCtrlRegs.GPAMUX1.bit.GPIO5 = 1;   // GPIO5 = EPWM3B
    GpioCtrlRegs.GPADIR.bit.GPIO5 = 1;    // Output

    SysCtrlRegs.PCLKCR0.bit.TBCLKSYNC = 0;
    EDIS;

    // EPWM1 Configuration
    EPwm1Regs.TBPRD = 9375;
    EPwm1Regs.TBCTL.bit.CTRMODE = 0;
    EPwm1Regs.AQCTLA.bit.ZRO = AQ_SET;     // GPIO0 - High at 0
    EPwm1Regs.AQCTLA.bit.CAU = AQ_CLEAR;   // GPIO0 - Low at CMPA
    EPwm1Regs.AQCTLB.bit.ZRO = AQ_SET;     // GPIO1 - High at 0
    EPwm1Regs.AQCTLB.bit.CBU = AQ_CLEAR;   // GPIO1 - Low at CMPB

    EPwm2Regs.TBPRD = 9375;
    EPwm2Regs.TBCTL.bit.CTRMODE = 0;
    EPwm2Regs.AQCTLA.bit.ZRO = AQ_SET;     // GPIO2 - High at 0
    EPwm2Regs.AQCTLA.bit.CAU = AQ_CLEAR;   // GPIO2 - Low at CMPA
    EPwm2Regs.AQCTLB.bit.ZRO = AQ_SET;     // GPIO3 - High at 0
    EPwm2Regs.AQCTLB.bit.CBU = AQ_CLEAR;   // GPIO3 - Low at CMPB

    // EPWM3 Configuration
    EPwm3Regs.TBPRD = 9375;
    EPwm3Regs.TBCTL.bit.CTRMODE = 0;
    EPwm3Regs.AQCTLA.bit.ZRO = AQ_SET;     // GPIO4 - High at 0
    EPwm3Regs.AQCTLA.bit.CAU = AQ_CLEAR;   // GPIO4 - Low at CMPA
    EPwm3Regs.AQCTLB.bit.ZRO = AQ_SET;     // GPIO5 - High at 0
    EPwm3Regs.AQCTLB.bit.CBU = AQ_CLEAR;   // GPIO5 - Low at CMPB

    Uint32 temp=0;



    EALLOW;
    SysCtrlRegs.PCLKCR0.bit.TBCLKSYNC = 1;
    EDIS;
}

// =============================
// SCI transmission functions
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

// Converts a float to ASCII and sends it with specified decimals
void scia_sendFloat(float value, int decimalPlaces)
{
    char buffer[10];  // Buffer for integer part
    int i = 0;
    int integerPart;
    float decimalPart;

    // Handle negative numbers
    if(value < 0)
    {
        scia_xmit('-');
        value = -value;
    }

    // Integer part
    integerPart = (int)value;
    decimalPart = value - integerPart;

    // Send integer part (using same algorithm as scia_sendUint16)
    if(integerPart == 0)
    {
        scia_xmit('0');
    }
    else
    {
        int temp = integerPart;
        i = 0;

        while(temp > 0)
        {
            buffer[i++] = (temp % 10) + '0';
            temp /= 10;
        }

        while(i > 0)
        {
            scia_xmit(buffer[--i]);
        }
    }

    // Send decimal point if decimals are required
    if(decimalPlaces > 0)
    {
        scia_xmit('.');

        // Send decimal part
        int d;
        for(d = 0; d < decimalPlaces; d++)
        {
            decimalPart *= 10;
            int digit = (int)decimalPart % 10;
            scia_xmit(digit + '0');
        }
    }
}
