//Name: Gleb Alexeev
//Lab: Lab4
//Point: 
//Class: P442
//Date: 1/28/16

#include "ch.h"
#include "hal.h"
#include "test.h"
#include "shell.h" 
#include "chprintf.h"
#include <chstreams.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <math.h>

#define UNUSED(x) (void)(x)
static THD_WORKING_AREA(waShell,2048);

static thread_t *shelltp1;

/* SPI configuration, sets up PortA Bit 8 as the chip select for the pressure sensor */
static SPIConfig bluefruit_config = {
  NULL,
  GPIOA,
  8,
  SPI_CR1_BR_2 | SPI_CR1_BR_1,
  0
};

void ReceiveData (char *receive_data, uint8_t size) {
  spiAcquireBus(&SPID1);               /* Acquire ownership of the bus.    */
  spiStart(&SPID1, &bluefruit_config);     /* Setup transfer parameters.       */
  spiSelect(&SPID1);                   /* Slave Select assertion.          */
  spiReceive(&SPID1, size, receive_data); 
  spiUnselect(&SPID1);                 /* Slave Select de-assertion.       */
  spiReleaseBus(&SPID1);               /* Ownership release.               */
}

void WriteCommand (char *data, uint8_t size) {
  spiAcquireBus(&SPID1);               /* Acquire ownership of the bus.    */
  spiStart(&SPID1, &bluefruit_config);     /* Setup transfer parameters.       */
  spiSelect(&SPID1);                   /* Slave Select assertion.          */
  spiSend(&SPID1, size, data); 
  spiUnselect(&SPID1);                 /* Slave Select de-assertion.       */
  spiReleaseBus(&SPID1);               /* Ownership release.               */
}

void WriteRead (char *data, uint8_t size, char *receive_data) {
  spiAcquireBus(&SPID1);               /* Acquire ownership of the bus.    */
  spiStart(&SPID1, &bluefruit_config);     /* Setup transfer parameters.       */
  spiSelect(&SPID1);                   /* Slave Select assertion.          */
  spiSend(&SPID1, size, data);
  spiReceive(&SPID1, 20, receive_data);
  spiUnselect(&SPID1);                 /* Slave Select de-assertion.       */
  spiReleaseBus(&SPID1);               /* Ownership release.               */
}
/* Thread that blinks North LED as an "alive" indicator */
static THD_WORKING_AREA(waCounterThread,128);
static THD_FUNCTION(counterThread,arg) {
  UNUSED(arg);
  while (TRUE) {
    palSetPad(GPIOE, GPIOE_LED3_RED);
    chThdSleepMilliseconds(500);
    palClearPad(GPIOE, GPIOE_LED3_RED);
    chThdSleepMilliseconds(500);
  }
}

static void cmd_bluefruit(BaseSequentialStream *chp, int argc, char *argv[]) {
  UNUSED(argc);
  if(!strcmp("cmd", argv[0])) {
    uint8_t size = 0;
    // while (argv[1][size] != '\0') {
    //   size++;
    // }
    size = 6;
    int receive_size = 20;

    uint8_t testAT[6] = {0x10, 0x00, 0x0A, 0x02, 0x41, 0x54};
    
    char s_data[size];
    char r_data[receive_size];

    //strcpy(s_data, argv[1]);
    // WriteCommand(s_data,size);
    chprintf(chp, "Sent: %d %s \n\r", size, testAT);
    //chThdSleepMicroseconds(100);
    // ReceiveData(r_data, 2);
    WriteRead(testAT,size, r_data);
    int i = -1;
    chprintf(chp, "Received:");
    while(++i < receive_size) {
      chprintf(chp, " %02x", r_data[i]);  
    }
    chprintf(chp, "\n\r");
    i = -1;
    while(++i < receive_size && r_data[i] != '\n') {
      chprintf(chp, "%c ", r_data[i]);
    }
    chprintf(chp, "\n\r");
  }
}

static void cmd_myecho(BaseSequentialStream *chp, int argc, char *argv[]) {
  int32_t i;
  (void)argv;
  for (i=0;i<argc;i++) {
    chprintf(chp, "%s\n\r", argv[i]);
  }
}

static const ShellCommand commands[] = {
  {"myecho", cmd_myecho},
  {"bf", cmd_bluefruit},
  {NULL, NULL}
};

static const ShellConfig shell_cfg1 = {
  (BaseSequentialStream *)&SD1,
  commands
};

static void termination_handler(eventid_t id) {

  (void)id;
  chprintf((BaseSequentialStream*)&SD1, "Shell Died\n\r");

  if (shelltp1 && chThdTerminatedX(shelltp1)) {
    chThdWait(shelltp1);
    chprintf((BaseSequentialStream*)&SD1, "Restarting from termination handler\n\r");
    chThdSleepMilliseconds(100);
    shelltp1 = shellCreate(&shell_cfg1, sizeof(waShell), NORMALPRIO);
  }
}

static evhandler_t fhandlers[] = {
  termination_handler
};

/*
 * Application entry point.
 */

int main(void) {
  event_listener_t tel;
  /*
   * System initializations.
   * - HAL initialization, this also initializes the configured device drivers
   *   and performs the board-specific initializations.
   * - Kernel initialization, the main() function becomes a thread and the
   *   RTOS is active.
   */
  halInit();
  chSysInit();

  /*
   * Activates the serial driver 1 using the driver default configuration.
   * PC4(RX) and PC5(TX). The default baud rate is 9600.
   */

  sdStart(&SD1, NULL);
  palSetPadMode(GPIOC, 4, PAL_MODE_ALTERNATE(7));
  palSetPadMode(GPIOC, 5, PAL_MODE_ALTERNATE(7));

  /* 
   *  Setup the pins for the spi link on the GPIOA. This link connects to the pressure sensor and the gyro.  
   * 
   */

  palSetPadMode(GPIOA, 5, PAL_MODE_ALTERNATE(5));     /* SCK. */
  palSetPadMode(GPIOA, 6, PAL_MODE_ALTERNATE(5));     /* MISO.*/
  palSetPadMode(GPIOA, 7, PAL_MODE_ALTERNATE(5));     /* MOSI.*/
  palSetPadMode(GPIOA, 8, PAL_MODE_OUTPUT_PUSHPULL);  /* pressure sensor chip select */
  palSetPadMode(GPIOE, 3, PAL_MODE_OUTPUT_PUSHPULL);  /* gyro chip select */
  palSetPad(GPIOA, 8);                                /* Deassert the bluetooth sensor chip select */
  palSetPadMode(GPIOC, GPIOC_PIN3, PAL_MODE_INPUT_PULLUP); /* IRQ pin */



  chprintf((BaseSequentialStream*)&SD1, "\n\rUp and Running\n\r");
  // chprintf((BaseSequentialStream*)&SD1, "Gyro Whoami Byte = 0x%02x\n\r",gyro_read_register(0x0F));
  // chprintf((BaseSequentialStream*)&SD1, "Pressure Whoami Byte = 0x%02x\n\r", pressure_read_register(0x0F));

  /* Initialize the command shell */ 
  shellInit();

  // gyro_write_register(0x20, 0x3F);
  // pressure_write_register(0x10, 0x7A);
  // pressure_write_register(0x20, 0xB4);
  /* 
   *  setup to listen for the shell_terminated event. This setup will be stored in the tel  * event listner structure in item 0
  */
  chEvtRegister(&shell_terminated, &tel, 0);

  shelltp1 = shellCreate(&shell_cfg1, sizeof(waShell), NORMALPRIO);
  chThdCreateStatic(waCounterThread, sizeof(waCounterThread), NORMALPRIO+1, counterThread, NULL);
  while (TRUE) {
    chEvtDispatch(fhandlers, chEvtWaitOne(ALL_EVENTS));
  }
 }


