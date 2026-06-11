#include "DSP28x_Project.h"

#define BUFFER_SIZE 1000

Uint16 source_buf[BUFFER_SIZE];
Uint16 dest_buf[BUFFER_SIZE];

void SetupMyGPIO(void) {
    EALLOW;
    // Configurar GPIO0 y GPIO1 como salidas para LEDs
    GpioCtrlRegs.GPAMUX1.bit.GPIO0 = 0;  // LED para CPU
    GpioCtrlRegs.GPAMUX1.bit.GPIO1 = 0;  // LED para DMA
    GpioCtrlRegs.GPADIR.bit.GPIO0 = 1;
    GpioCtrlRegs.GPADIR.bit.GPIO1 = 1;

    // Apagar ambos LEDs inicialmente
    GpioDataRegs.GPACLEAR.bit.GPIO0 = 1;
    GpioDataRegs.GPACLEAR.bit.GPIO1 = 1;
    EDIS;
}

void main(void) {
    InitSysCtrl();
    SetupMyGPIO();
    DINT;
    InitPieCtrl();
    IER = 0x0000;
    IFR = 0x0000;
    InitPieVectTable();

    // Configurar DMA una sola vez al inicio
    EALLOW;
    DmaRegs.CH1.MODE.bit.DATASIZE = 0;    // 16-bit transfers
    DmaRegs.CH1.MODE.bit.CHINTMODE = 0;   // No interrupt
    DmaRegs.CH1.MODE.bit.CONTINUOUS = 0;  // One-shot mode
    DmaRegs.CH1.MODE.bit.OVRINTE = 0;     // No overflow interrupt
    DmaRegs.CH1.CONTROL.bit.RUN = 0;

    // Configurar burst y transfer size
    DmaRegs.CH1.BURST_SIZE.all = 1;       // 1 transfer per burst
    DmaRegs.CH1.TRANSFER_SIZE = BUFFER_SIZE;

    // Configurar direcciones
    DmaRegs.CH1.SRC_BEG_ADDR_SHADOW = (Uint32)&source_buf[0];
    DmaRegs.CH1.DST_BEG_ADDR_SHADOW = (Uint32)&dest_buf[0];
    DmaRegs.CH1.SRC_ADDR_SHADOW = (Uint32)&source_buf[0];
    DmaRegs.CH1.DST_ADDR_SHADOW = (Uint32)&dest_buf[0];
    EDIS;

    EINT;
    ERTM;

    while(1) {
        /* ******************************************************
         * PRUEBA 1: COPIA CON CPU (GPIO0 FIJO)
         ****************************************************** */

        // Preparar datos
        int i;
        for(i = 0; i < BUFFER_SIZE; i++) {
            source_buf[i] = i;
            dest_buf[i] = 0;
        }

        // Encender LED CPU (GPIO0) y apagar LED DMA (GPIO1)
        GpioDataRegs.GPASET.bit.GPIO0 = 1;    // GPIO0 ON para CPU
        GpioDataRegs.GPACLEAR.bit.GPIO1 = 1;  // GPIO1 OFF
        DELAY_US(500000);  // Espera 0.5s para ver LED encendido

        // Copia por CPU - GPIO0 permanece ENCENDIDO FIJO
        for(i = 0; i < BUFFER_SIZE; i++) {
            dest_buf[i] = source_buf[i];
        }

        // Apagar GPIO0 después de la copia
        DELAY_US(500000);
        GpioDataRegs.GPACLEAR.bit.GPIO0 = 1;  // GPIO0 OFF
        DELAY_US(1000000);

        /* ******************************************************
         * PRUEBA 2: COPIA CON DMA (GPIO1 PARPADEANTE)
         ****************************************************** */

        // Preparar nuevos datos
        for(i = 0; i < BUFFER_SIZE; i++) {
            source_buf[i] = i + 1000;
            dest_buf[i] = 0;
        }

        // Encender LED DMA (GPIO1) y apagar LED CPU (GPIO0)
        GpioDataRegs.GPASET.bit.GPIO1 = 1;    // GPIO1 ON para DMA
        GpioDataRegs.GPACLEAR.bit.GPIO0 = 1;  // GPIO0 OFF
        DELAY_US(500000);  // Espera 0.5s para ver LED encendido

        // Iniciar DMA
        EALLOW;
        DmaRegs.CH1.CONTROL.bit.RUN = 1;
        EDIS;

        // Durante la transferencia DMA, parpadear GPIO1
        int parpadeos = 0;
        while(DmaRegs.CH1.CONTROL.bit.RUN == 1) {
            //GpioDataRegs.GPATOGGLE.bit.GPIO1 = 1;  // Toggle GPIO1
            //DELAY_US(100000);  // Parpadeo cada 100ms
            parpadeos++;

            // Timeout de seguridad
            if (parpadeos > 50) {
                break;
            }
        }

        // Apagar GPIO1 cuando termina
        GpioDataRegs.GPACLEAR.bit.GPIO1 = 1;  // GPIO1 OFF
        DELAY_US(1000000);


    }
}
