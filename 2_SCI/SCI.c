
#include "DSP28x_Project.h"

// Prototypes
void InitSciaGpio(void);
void InitScia(void);
void scia_xmit(char a);
void scia_msg(char *msg);
char scia_receive(void);

void main(void)
{
    // System initialization
    InitSysCtrl();

    // Disable interrupts
    DINT;
    InitPieCtrl();
    IER = 0x0000;
    IFR = 0x0000;
    InitPieVectTable();

    // Initialize SCI
    InitSciaGpio();
    InitScia();

    // Startup message
    scia_msg("Serial Echo System started\r\n");
    scia_msg("Type something and I will echo it back:\r\n");

    // Enable global interrupts
    EINT;
    ERTM;

    // Main loop
    while(1)
    {
        // Receive character
        char received = scia_receive();

        // Send the same character back echo
        scia_xmit(received);
    }
}

// =============================
// GPIO configuration for SCI-A
// =============================
void InitSciaGpio(void)
{
    EALLOW;

    // Configure GPIO28 = SCIRXDA reception
    GpioCtrlRegs.GPAPUD.bit.GPIO28 = 0;   // Enable pull-up
    GpioCtrlRegs.GPAQSEL2.bit.GPIO28 = 3; // Asynchronous synchronization
    GpioCtrlRegs.GPAMUX2.bit.GPIO28 = 1;  // Select SCIRXDA

    // Configure GPIO29 = SCITXDA transmission
    GpioCtrlRegs.GPAPUD.bit.GPIO29 = 0;   // Enable pull-up
    GpioCtrlRegs.GPAMUX2.bit.GPIO29 = 1;  // Select SCITXDA

    EDIS;
}

// =============================
// SCI-A configuration
// =============================
void InitScia(void)
{
    // Configure 8 bits, 1 stop, no parity
    SciaRegs.SCICCR.all = 0x0007;

    // Enable TX, RX, internal clock
    SciaRegs.SCICTL1.all = 0x0003;

    // Disable TX and RX interrupts
    SciaRegs.SCICTL2.bit.TXINTENA = 0;
    SciaRegs.SCICTL2.bit.RXBKINTENA = 0;

    // Configure baudrate for 115200 at 150MHz
    // Formula: BRR = LSPCLK / (BAUD * 8)
    // Where LSPCLK = 150MHz for F28335
    // 150,000,000 / (115,200 * 8) = 150,000,000 / 921,600 = 162.76
    // Take the integer part = 162 decimal = 0xA2 hexadecimal
    // SCILBAUD = low byte of the value = 0xA2
    // SCIHBAUD = high byte of the value = 0x00
    SciaRegs.SCIHBAUD = 0x00;
    SciaRegs.SCILBAUD = 0xA2;  // 162 decimal = 0xA2

    // Other common baudrates with LSPCLK = 150MHz:
    // Baudrate 9600:   150,000,000 / (9600 * 8) = 150,000,000 / 76,800 = 1953.125 -> SCILBAUD = 0x1A, SCIHBAUD = 0x07
    // Baudrate 19200:  150,000,000 / (19200 * 8) = 150,000,000 / 153,600 = 976.56 -> SCILBAUD = 0xD0, SCIHBAUD = 0x03
    // Baudrate 115200: 150,000,000 / (115200 * 8) = 150,000,000 / 921,600 = 162.76 -> SCILBAUD = 0xA2, SCIHBAUD = 0x00
    // Baudrate 460800: 150,000,000 / (460800 * 8) = 150,000,000 / 3,686,400 = 40.69 -> SCILBAUD = 0x28, SCIHBAUD = 0x00
    // Baudrate 921600: 150,000,000 / (921600 * 8) = 150,000,000 / 7,372,800 = 20.34 -> SCILBAUD = 0x14, SCIHBAUD = 0x00

    // Finalize initialization
    SciaRegs.SCICTL1.all = 0x0023;
}

// =============================
// Transmit a character
// =============================
void scia_xmit(char a)
{
    // Wait for TX buffer to be ready
    while(SciaRegs.SCICTL2.bit.TXRDY == 0);

    // Send character
    SciaRegs.SCITXBUF = a;
}

// =============================
// Receive a character blocking
// =============================
char scia_receive(void)
{
    // Wait for data to be available
    while(SciaRegs.SCIRXST.bit.RXRDY == 0);

    // Read and return the character
    return SciaRegs.SCIRXBUF.all;
}

// =============================
// Send a string
// =============================
void scia_msg(char *msg)
{
    while(*msg != '\0')
    {
        scia_xmit(*msg);
        msg++;
    }
}
