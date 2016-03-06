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
static uint8_t header[] = {0x10, 0x00, 0x0A};
static uint8_t headerSize = 3;
static uint8_t bufferSize = 6;

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

void WriteRead (uint8_t *data, uint8_t size, uint8_t *receive_data) {
  spiAcquireBus(&SPID1);               /* Acquire ownership of the bus.    */
  spiStart(&SPID1, &bluefruit_config);     /* Setup transfer parameters.       */
  spiSelect(&SPID1);                   /* Slave Select assertion.          */
  spiExchange(&SPID1, size, data, receive_data);
  spiUnselect(&SPID1);                 /* Slave Select de-assertion.       */
  spiReleaseBus(&SPID1);               /* Ownership release.               */
}







///Gleb is working on this, still needs work, but getting closer to the sol'n

void WriteReadMain(uint8_t *send_data, uint8_t size, uint8_t *receive_data) {
  spiAcquireBus(&SPID1);               /* Acquire ownership of the bus.    */
  spiStart(&SPID1, &bluefruit_config);     /* Setup transfer parameters.       */
  spiSelect(&SPID1);                   /* Slave Select assertion.          */
  int i = 0;
  while(i < size) {
  	spiSend(&SPID1, 1, send_data);
    send_data++;
  	i++;
  }
  spiUnselect(&SPID1);                 /* Slave Select de-assertion.       */

  chThdSleepMilliseconds(3);
  
  spiSelect(&SPID1);	                /* Slave Select assertion.          */

  int j = 0;
  uint8_t recSize;
  while(j == 0) {
	spiReceive(&SPID1, 1, receive_data);
	 if ((receive_data[0] == 0x10) ||
		(receive_data[0] == 0x20) ||
		(receive_data[0] == 0x40) ||
		(receive_data[0] == 0x80)) {
    receive_data++;
		j = 1;
	 }
  }
  spiReceive(&SPID1, 1, receive_data);
  receive_data++;
  spiReceive(&SPID1, 1, receive_data);
  receive_data++;
  spiReceive(&SPID1, 1, &recSize);
  receive_data[3] = recSize;
  receive_data++;
  i = 0;
  while(i < recSize) {
  	spiReceive(&SPID1, 1, receive_data);
    receive_data++;
    i++;
  }
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
    uint8_t payloadSize = 0;
    uint8_t payload[16];
    /* Scan the message, gather size, and fill the message payload */
    while (argv[1][payloadSize] != '\0') {
      /* Need to take messages that are larger than 16 into account */

      /*********/
      payload[payloadSize] = (uint8_t)argv[1][payloadSize];
      payloadSize++;
    }
   
    /* Initialize send/receive messages */
    uint8_t messageSize = payloadSize + headerSize + 1;
    uint8_t tx_data[messageSize];
    uint8_t rx_data[20];

    /* Begin creating message */
    int messageIndex = 0;
    int payloadIndex = 0;

    /* Header */
    while (messageIndex < headerSize) { 
      tx_data[messageIndex] = header[messageIndex];
      messageIndex++;
    }

    /* Payload Size */
    tx_data[messageIndex] = payloadSize;
    messageIndex++;
    
    /* Payload */
    while (messageIndex < messageSize) {
      tx_data[messageIndex] = payload[payloadIndex];
      messageIndex++; payloadIndex++;
    }
    
    tx_data[messageIndex] = '\n';
    tx_data[messageIndex+1] = '\r';
    messageIndex = messageIndex + 2;
    chprintf(chp, "Sent: %d %x \n\r", payloadSize, tx_data);
    //CHANGE>>>
    WriteReadMain(tx_data, messageSize+2, rx_data);

    /* Everything below is the same */
    int i = headerSize + 1;
    //int size = 20;
    chprintf(chp, "Received:");
    while(i < messageSize) {
      chprintf(chp, " %c", rx_data[i]);  
      i++;
    }
    chprintf(chp, "\n\r");
    //i = headerSize + 1;
    /*while(i < messageSize && rx_data[i] != '\n') {
      chprintf(chp, "%c ", rx_data[i]);
      i++;
      }*/
    //chprintf(chp, "\n\r");

    /* Scratch work area */

    //size = 8;
    //int receive_size = 20;
    // uint8_t testAT[8] = {0x10, 0x00, 0x0A, 0x02, 0x41, 0x54, '\n', '\r'};

    
    //strcpy(tx_data, argv[1]);
    // WriteCommand(tx_data,size);

    //chThdSleepMicroseconds(100);
    // ReceiveData(rx_data, 2);

    /*********************/

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


