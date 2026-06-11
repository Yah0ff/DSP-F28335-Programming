/************************************************************
 * F28335 - CAN-A 4 NODOS con 3 botones y 3 LEDs por nodo
 * - Cada nodo: 3 botones, 3 LEDs
 * - BROADCAST: Todos reciben en ID 0x100
 * - Payload: BYTE0 = DESTINO usuario, BYTE1 = LED a togglear
 ************************************************************/
#include "DSP2833x_Device.h"
#include "DSP2833x_Examples.h"

/*===================== CONFIGURA TU NODE AQUÍ =====================*/
#define NODE_ID  (3u)   /* CAMBIA a 1, 2, 3 o 4 según el DSP */

/*===================== ID ÚNICO para todos ========================*/
#define BROADCAST_ID  (0x100u)  /* TODOS usan este mismo ID */

/*===================== GPIO LEDS (3 LEDs por nodo) ===============*/
/* GPIO24 = LED1, GPIO25 = LED2, GPIO26 = LED3 */
#define LEDS_INIT()      do{ EALLOW; \
                               GpioCtrlRegs.GPAMUX2.bit.GPIO24 = 0; \
                               GpioCtrlRegs.GPAMUX2.bit.GPIO25 = 0; \
                               GpioCtrlRegs.GPAMUX2.bit.GPIO26 = 0; \
                               GpioCtrlRegs.GPADIR.bit.GPIO24  = 1; \
                               GpioCtrlRegs.GPADIR.bit.GPIO25  = 1; \
                               GpioCtrlRegs.GPADIR.bit.GPIO26  = 1; \
                               EDIS; \
                            }while(0)

#define LED1_TOGGLE()    do{ GpioDataRegs.GPATOGGLE.bit.GPIO24 = 1; }while(0)
#define LED2_TOGGLE()    do{ GpioDataRegs.GPATOGGLE.bit.GPIO25 = 1; }while(0)
#define LED3_TOGGLE()    do{ GpioDataRegs.GPATOGGLE.bit.GPIO26 = 1; }while(0)

/*===================== GPIO BOTONES (3 botones) ==================*/
/* GPIO12 = BTN1, GPIO13 = BTN2, GPIO14 = BTN3 */
#define BTNS_INIT()      do{ EALLOW; \
                               /* BTN1 - GPIO12 */ \
                               GpioCtrlRegs.GPAMUX1.bit.GPIO12 = 0; \
                               GpioCtrlRegs.GPADIR.bit.GPIO12  = 0; \
                               GpioCtrlRegs.GPAPUD.bit.GPIO12  = 0; \
                               GpioCtrlRegs.GPAQSEL1.bit.GPIO12= 3; \
                               /* BTN2 - GPIO13 */ \
                               GpioCtrlRegs.GPAMUX1.bit.GPIO13 = 0; \
                               GpioCtrlRegs.GPADIR.bit.GPIO13  = 0; \
                               GpioCtrlRegs.GPAPUD.bit.GPIO13  = 0; \
                               GpioCtrlRegs.GPAQSEL1.bit.GPIO13= 3; \
                               /* BTN3 - GPIO14 */ \
                               GpioCtrlRegs.GPAMUX1.bit.GPIO14 = 0; \
                               GpioCtrlRegs.GPADIR.bit.GPIO14  = 0; \
                               GpioCtrlRegs.GPAPUD.bit.GPIO14  = 0; \
                               GpioCtrlRegs.GPAQSEL1.bit.GPIO14= 3; \
                               EDIS; \
                            }while(0)
#define BTN1_IS_PRESSED()   (GpioDataRegs.GPADAT.bit.GPIO12 == 0)
#define BTN2_IS_PRESSED()   (GpioDataRegs.GPADAT.bit.GPIO13 == 0)
#define BTN3_IS_PRESSED()   (GpioDataRegs.GPADAT.bit.GPIO14 == 0)

/*===================== Mapeo según especificación ================*/
/*
 * Especificación:
 * Usuario1 BTN1  LED1 usuario2 (DEST=2, LED=1)
 * Usuario1 BTN2  LED1 usuario3 (DEST=3, LED=1)
 * Usuario1 BTN3  LED1 usuario4 (DEST=4, LED=1)
 *
 * Usuario2 BTN1 LED1 usuario1 (DEST=1, LED=1)
 * Usuario2 BTN2  LED2 usuario3 (DEST=3, LED=2)
 * Usuario2 BTN3  LED2 usuario4 (DEST=4, LED=2)
 *
 * Usuario3 BTN1  LED2 usuario1 (DEST=1, LED=2)
 * Usuario3 BTN2  LED2 usuario2 (DEST=2, LED=2)
 * Usuario3 BTN3  LED3 usuario4 (DEST=4, LED=3)
 *
 * Usuario4 BTN1  LED3 usuario1 (DEST=1, LED=3)
 * Usuario4 BTN2  LED3 usuario2 (DEST=2, LED=3)
 * Usuario4 BTN3  LED3 usuario3 (DEST=3, LED=3)
 */

/* Retorna destino y LED para cada botón según NODE_ID */
static inline void get_btn1_params(Uint16 *dest, Uint16 *led) {
    if (NODE_ID == 1) { *dest = 2; *led = 1; }
    else if (NODE_ID == 2) { *dest = 1; *led = 1; }
    else if (NODE_ID == 3) { *dest = 1; *led = 2; }
    else { *dest = 1; *led = 3; }  /* NODE_ID == 4 */
}

static inline void get_btn2_params(Uint16 *dest, Uint16 *led) {
    if (NODE_ID == 1) { *dest = 3; *led = 1; }
    else if (NODE_ID == 2) { *dest = 3; *led = 2; }
    else if (NODE_ID == 3) { *dest = 2; *led = 2; }
    else { *dest = 2; *led = 3; }  /* NODE_ID == 4 */
}

static inline void get_btn3_params(Uint16 *dest, Uint16 *led) {
    if (NODE_ID == 1) { *dest = 4; *led = 1; }
    else if (NODE_ID == 2) { *dest = 4; *led = 2; }
    else if (NODE_ID == 3) { *dest = 4; *led = 3; }
    else { *dest = 3; *led = 3; }  /* NODE_ID == 4 */
}

/*===================== Prototipos ================================*/
static void init_system(void);
static void cana_basic_init(void);
static void cana_setup_mailboxes(void);
static void debounce_press(void);
static void debounce_release(void);
static void can_send_message(Uint16 dest, Uint16 led_num);

/*===================== Globals ===================================*/
volatile Uint32 g_txCount = 0;
volatile Uint32 g_rxCount = 0;

/*===================== MAIN =====================================*/
void main(void)
{
    /* Estados botones */
    Uint16 s1 = 0, s2 = 0, s3 = 0;
    Uint16 b1_prev = 1, b2_prev = 1, b3_prev = 1;

    /* Inicialización */
    init_system();
    InitECanGpio();
    cana_basic_init();
    cana_setup_mailboxes();

    /* Inicializar LEDs */
    LEDS_INIT();

    /* Inicializar botones */
    BTNS_INIT();

    /* Loop principal */
    for (;;)
    {
        /* 1) RECEPCIÓN BROADCAST */
        if (ECanaRegs.CANRMP.bit.RMP1 == 1) {
            volatile struct MBOX *mbx = &ECanaMboxes.MBOX1;
            Uint16 dest = (Uint16)mbx->MDL.byte.BYTE0;
            Uint16 led_num = (Uint16)mbx->MDL.byte.BYTE1;

            /* Solo actuar si el mensaje es para MÍ */
            if (dest == NODE_ID) {
                if (led_num == 1) {
                    LED1_TOGGLE();
                } else if (led_num == 2) {
                    LED2_TOGGLE();
                } else if (led_num == 3) {
                    LED3_TOGGLE();
                }
                g_rxCount++;
            }

            /* Limpiar flag de recepción */
            ECanaRegs.CANRMP.all = (1UL<<1);
        }

        /* 2) BOTÓN 1 */
        {
            Uint16 b1_now = (Uint16)BTN1_IS_PRESSED();
            if (s1 == 0) {
                if ((b1_prev == 1) && (b1_now == 0)) {
                    Uint16 dest, led;
                    get_btn1_params(&dest, &led);
                    can_send_message(dest, led);
                    g_txCount++;
                    debounce_press();
                    s1 = 1;
                }
            } else {
                if ((b1_prev == 0) && (b1_now == 1)) {
                    debounce_release();
                    s1 = 0;
                }
            }
            b1_prev = b1_now;
        }

        /* 3) BOTÓN 2 */
        {
            Uint16 b2_now = (Uint16)BTN2_IS_PRESSED();
            if (s2 == 0) {
                if ((b2_prev == 1) && (b2_now == 0)) {
                    Uint16 dest, led;
                    get_btn2_params(&dest, &led);
                    can_send_message(dest, led);
                    g_txCount++;
                    debounce_press();
                    s2 = 1;
                }
            } else {
                if ((b2_prev == 0) && (b2_now == 1)) {
                    debounce_release();
                    s2 = 0;
                }
            }
            b2_prev = b2_now;
        }

        /* 4) BOTÓN 3 */
        {
            Uint16 b3_now = (Uint16)BTN3_IS_PRESSED();
            if (s3 == 0) {
                if ((b3_prev == 1) && (b3_now == 0)) {
                    Uint16 dest, led;
                    get_btn3_params(&dest, &led);
                    can_send_message(dest, led);
                    g_txCount++;
                    debounce_press();
                    s3 = 1;
                }
            } else {
                if ((b3_prev == 0) && (b3_now == 1)) {
                    debounce_release();
                    s3 = 0;
                }
            }
            b3_prev = b3_now;
        }
    }
}

/*===================== Función para enviar mensaje ===============*/
static void can_send_message(Uint16 dest, Uint16 led_num)
{
    struct ECAN_REGS sh;

    /* Configurar payload */
    ECanaMboxes.MBOX0.MDL.byte.BYTE0 = (Uint16)dest;
    ECanaMboxes.MBOX0.MDL.byte.BYTE1 = (Uint16)led_num;
    ECanaMboxes.MBOX0.MSGCTRL.bit.DLC = 2;

    /* Disparar transmisión */
    sh.CANTRS.all = 0;
    sh.CANTRS.bit.TRS0 = 1;
    ECanaRegs.CANTRS.all = sh.CANTRS.all;

    /* Esperar fin de transmisión */
    do {
        sh.CANTA.all = ECanaRegs.CANTA.all;
    } while (sh.CANTA.bit.TA0 == 0);

    /* Limpiar flag */
    sh.CANTA.all = 0;
    sh.CANTA.bit.TA0 = 1;
    ECanaRegs.CANTA.all = sh.CANTA.all;
}

/*===================== Inicializaciones ==========================*/
static void init_system(void)
{
    InitSysCtrl();
    DINT;
    InitPieCtrl();
    IER = 0x0000;
    IFR = 0x0000;
    InitPieVectTable();
    EINT;
    ERTM;
}

static void cana_basic_init(void)
{
    InitECana();
}

static void cana_setup_mailboxes(void)
{
    struct ECAN_REGS sh;

    ECanaRegs.CANME.all = 0x00000000;

    /* TX MBOX0 - BROADCAST_ID */
    ECanaMboxes.MBOX0.MSGID.all = 0;
    ECanaMboxes.MBOX0.MSGID.bit.IDE = 0;
    ECanaMboxes.MBOX0.MSGID.bit.STDMSGID = (BROADCAST_ID & 0x7FF);
    ECanaMboxes.MBOX0.MSGCTRL.bit.DLC = 2;

    sh.CANMD.all = ECanaRegs.CANMD.all;
    sh.CANMD.bit.MD0 = 0;
    ECanaRegs.CANMD.all = sh.CANMD.all;

    /* RX MBOX1 - BROADCAST_ID */
    ECanaMboxes.MBOX1.MSGID.all = 0;
    ECanaMboxes.MBOX1.MSGID.bit.IDE = 0;
    ECanaMboxes.MBOX1.MSGID.bit.STDMSGID = (BROADCAST_ID & 0x7FF);
    ECanaMboxes.MBOX1.MSGCTRL.bit.DLC = 2;

    sh.CANMD.all = ECanaRegs.CANMD.all;
    sh.CANMD.bit.MD1 = 1;
    ECanaRegs.CANMD.all = sh.CANMD.all;

    ECanaRegs.CANME.all = (1UL<<0) | (1UL<<1);

    ECanaRegs.CANTA.all   = 0xFFFFFFFF;
    ECanaRegs.CANRMP.all  = 0xFFFFFFFF;
    ECanaRegs.CANGIF0.all = 0xFFFFFFFF;
    ECanaRegs.CANGIF1.all = 0xFFFFFFFF;
}

/*===================== Antirrebote ===============================*/
static void debounce_press(void)   { DELAY_US(10000); }
static void debounce_release(void) { DELAY_US(2000);  }
