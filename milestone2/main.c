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
//static uint8_t bufferSize = 6;

/* SPI configuration, sets up PortA Bit 8 as the chip select for the pressure sensor */
static SPIConfig bluefruit_config = {
  NULL,
  GPIOA,
  8,
  SPI_CR1_BR_2 | SPI_CR1_BR_1,
  0
};

void WriteReadMain(uint8_t *send_data, uint8_t size, uint8_t *receive_data) {
  spiAcquireBus(&SPID1);               /* Acquire ownership of the bus.    */
  spiStart(&SPID1, &bluefruit_config);     /* Setup transfer parameters.       */
  spiSelect(&SPID1);                   /* Slave Select assertion.          */

  int i = size - 4;
  int k = 1;
  int m = 0;
  int sdpointer = 0;

  /* Split the message if possible */
  while (i > 0) {
    if (i > 16) { 
      uint8_t tx_data[20];
      uint8_t temp;
      uint8_t rec_temp = 0xFE;
      i = i - 16;
      tx_data[0] = 0x10;
      while(rec_temp == 0xFE) {
        temp = tx_data[0];
        spiExchange(&SPID1, 1, &temp, &rec_temp);
        //spiSend(&SPID1, 1, &temp);
        //spiReceive(&SPID1, 1, &rec_temp);
        if(rec_temp == 0xFE) {
          break;
          break;
          break;
        } else {
        chThdSleepMicroseconds(10);
        spiUnselect(&SPID1);                 /* Slave Select de-assertion. */
        chThdSleepMicroseconds(100);
        spiSelect(&SPID1);
        }
      }
      tx_data[1] = 0x00;
      tx_data[2] = 0x0A;
      tx_data[3] = (16|0x80);
      int thresh = sdpointer; //current sdpointer for counting purposes.
      while (sdpointer < thresh + 16) {
        tx_data[m+4] = send_data[sdpointer+4];
        m++;
        sdpointer++;
      }
      while(k < 20) {
        temp = tx_data[k];
        spiSend(&SPID1, 1, &temp);
        k++;    
      }
      k = 1;
      m = 0;
      chThdSleepMicroseconds(20);
      spiUnselect(&SPID1);                 /* Slave Select de-assertion. */
      chThdSleepMilliseconds(3);
      spiSelect(&SPID1);
    } else {
      uint8_t tx_data[i+4];
      uint8_t temp;
      uint8_t rec_temp = 0xFE;
      tx_data[0] = 0x10;
      while(rec_temp == 0xFE) {
        temp = tx_data[0];
        spiExchange(&SPID1, 1, &temp, &rec_temp);
        //spiSend(&SPID1, 1, &temp);
        //spiReceive(&SPID1, 1, &rec_temp);
        if(rec_temp == 0xFE) {
          break;
        } else {
          chThdSleepMicroseconds(10);
          spiUnselect(&SPID1);                 /* Slave Select de-assertion. */
          chThdSleepMicroseconds(100);
          spiSelect(&SPID1);
        }
      }
      tx_data[1] = 0x00;
      tx_data[2] = 0x0A;
      tx_data[3] = i;
      int thresh = sdpointer;

      while (sdpointer < thresh + i) {
        tx_data[m+4] = send_data[sdpointer+4];
        m++;
        sdpointer++;
      }

      while(k < i+4) {
        temp = tx_data[k];
        spiSend(&SPID1, 1, &temp);
        k++;
      }
      k = 1;
      m = 0;
      sdpointer = 0;
      i = -2;
    }
  }

  spiUnselect(&SPID1);                 /* Slave Select de-assertion.       */

  chThdSleepMilliseconds(3);
  
  spiSelect(&SPID1);                  /* Slave Select assertion.          */

  int j = 0;
  uint8_t recSize;
  while(j == 0) {
  spiReceive(&SPID1, 1, receive_data);
   if (((receive_data[0] == 0x10) ||
    (receive_data[0] == 0x20) ||
    (receive_data[0] == 0x40) ||
    (receive_data[0] == 0x80))) {
    receive_data++;
    j = 1;
   } // else {
   //  spiUnselect(&SPID1);
   //  chThdSleepMicroseconds(3);
   //  spiSelect(&SPID1);
   //  chThdSleepMicroseconds(1);
   // }
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

/* Thread that blinks North LED as an "alive" indicator */
static THD_WORKING_AREA(waMessageThread,128);
static THD_FUNCTION(messageThread,arg) {
  uint8_t tx_data_test[] = {0x10, 0x00, 0x0A, 0x13, 'A', 'T', '+', 'B', 'L', 'E', 'U', 'A', 'R', 'T', 'T', 'X', '=', 'h', 'i', 0x5C, 0x72, 0x5C, 0x6E};
  uint8_t rx_data_test[20];
  UNUSED(arg);
  while (TRUE) {
    chThdSleepMilliseconds(500);
    WriteReadMain(tx_data_test, 23, rx_data_test);
    chThdSleepMilliseconds(1000);
  }
}

static void cmd_bluefruit(BaseSequentialStream *chp, int argc, char *argv[]) {
  UNUSED(argc);
  if(!strcmp("cmd", argv[0])) {
    uint16_t payloadSize = 0;
    
    uint8_t n = 0;
    /*
    1)
    Get gyroscope data
    Sprintf(gyroscopedata)
    make a new command bf sendGyro
    header + "AT+BLEUARTTX=" + sendGyrodata

    2)
    IRQ pin interrupt on receive data;
    anytime the IRQ pin goes, we start reading.
    BUT -> how do we change IRQ pin to work on bluetooth connection?
    Mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm
    */

    /* Scan the message, gather size, and fill the message payload */
    while (argv[1][payloadSize] != '\0') {
      payloadSize++;
    }

    uint8_t payload[payloadSize];

    for(n = 0; n < payloadSize; n++) {
      payload[n] = (uint8_t)argv[1][n];
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
    
    chprintf(chp, "Sent: %d %x \n\r", payloadSize, tx_data);

    WriteReadMain(tx_data, messageSize, rx_data);

    /* Everything below is the same */
    int i = headerSize + 1;
    //int size = 20;
    chprintf(chp, "Received:");
    while(i < 20) {
      chprintf(chp, " %c", rx_data[i]);  
      i++;
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
  chThdCreateStatic(waMessageThread, sizeof(waMessageThread), NORMALPRIO+1, messageThread, NULL);
  while (TRUE) {
    chEvtDispatch(fhandlers, chEvtWaitOne(ALL_EVENTS));
  }
 }
