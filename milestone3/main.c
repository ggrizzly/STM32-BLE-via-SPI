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
static uint8_t tx_advstart[] = {0x10, 0x00, 0x0A, 0x13, 'A', 'T', '+', 'G', 'A', 'P', 'S', 'T', 'A', 'R', 'T', 'A', 'D', 'V'};
static uint8_t tx_advstop[] = {0x10, 0x00, 0x0A, 0x13, 'A', 'T', '+', 'G', 'A', 'P', 'S', 'T', 'O', 'P', 'A', 'D', 'V'};
static uint8_t tx_getconn[] = {0x10, 0x00, 0x0A, 0x13, 'A', 'T', '+', 'G', 'A', 'P', 'G', 'E', 'T', 'C', 'O', 'N', 'N'};
static uint8_t tx_disconnect[] = {0x10, 0x00, 0x0A, 0x13, 'A', 'T', '+', 'G', 'A', 'P', 'D', 'I', 'S', 'C', 'O', 'N', 'N', 'E', 'C', 'T'};
static uint8_t tx_uartrx[] = {0x10, 0x00, 0x0A, 0x13, 'A', 'T', '+', 'B', 'L', 'E', 'U', 'A', 'R', 'T', 'R', 'X', 0x5C, 0x72, 0x5C, 0x6E};

uint8_t bigData[1024];
int connectedFLAG = 0;

/* SPI configuration, sets up PortA Bit 8 as the chip select for the pressure sensor */
static SPIConfig bluefruit_config = {
  NULL,
  GPIOA,
  8,
  SPI_CR1_BR_2 | SPI_CR1_BR_1,
  0
};

/*
TODO:
Connect to phone and tinker with advertising data:
- Find the minimum interval on which the cell connects
- Alter intervals with AT+GAPINTERVALS (For future, power consumption)
- Two commands we have also is AT+GAPSTARTADV and STOPADV.
- During this time, use the command AT+GAPGETCONN
  -> 1 if we are connected
  -> 0 if we are not
- We advertise for x amount of times
  -> If we are not connected during any of those times go back to sleep
  -> If we are, get ack'd and send all the data accumulated from the trail counter

Other:
- Set the device name, so that our future app looks for the name automatically.
- Turn off LEDS
*/


void WriteReadWrapper(uint8_t *send_data, uint32_t size) {
  uint8_t tx_data_plus[208] = {0x10, 0x00, 0x0A, 0xD0, 'A', 'T', '+', 'B', 'L', 'E', 'U', 'A', 'R', 'T', 'T', 'X', '='};
  uint8_t rdata[20] = { 0 };
  uint32_t i = 0;
  while (size > 0) {
    if(size > 191) {
      for (i = 17; i < 208; i++) {
        tx_data_plus[i] = send_data[i - 17];
      }
      chThdSleepMilliseconds(100);
      WriteRead(tx_data_plus, 208, rdata);
      chThdSleepMilliseconds(100);
      send_data += 191;
      size -= 191;
    } 
    else {
      if(size > 3) {
        tx_data_plus[3] = (uint8_t) (0x80 | (size + 13));
      }
      else {
        tx_data_plus[3] = (uint8_t) size + 13;
      }
      for (i = 17; i < size+17; i++) {
        tx_data_plus[i] = send_data[i - 17];
      }
      //tx_data_plus[3] = (uint8_t) size;
      chThdSleepMilliseconds(100);
      WriteRead(tx_data_plus, size + 17, rdata);
      chThdSleepMilliseconds(100);
      size = 0;
    }
  }
}

void WriteRead(uint8_t *send_data, uint8_t size, uint8_t *receive_data) {
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
// static THD_WORKING_AREA(waCounterThread,128);
// static THD_FUNCTION(counterThread,arg) {
//   UNUSED(arg);
//   while (TRUE) {
//     palSetPad(GPIOE, GPIOE_LED3_RED);
//     chThdSleepMilliseconds(500);
//     palClearPad(GPIOE, GPIOE_LED3_RED);
//     chThdSleepMilliseconds(500);
//   }
// }

// static THD_WORKING_AREA(waAdvertisingThread,128);
// static THD_FUNCTION(advertisingThread,arg) {
//   UNUSED(arg);
//   uint8_t rx_data[20];
//   uint8_t tmp_data[20];

//   while (TRUE) {
//     chThdSleepMilliseconds(500);
//     WriteRead(tx_advstart, 22, tmp_data);
//     chprintf((BaseSequentialStream*)&SD1, "Advertising Started\r\n");
//     chThdSleepMilliseconds(10000);
//     WriteRead(tx_getconn, 21, rx_data);
//     if (rx_data[4] == '1') { //Pump Data if we're here
//       chprintf((BaseSequentialStream*)&SD1, "Connected\r\n");
//       connectedFLAG = 1;
//       chThdSleepMilliseconds(10000);
//     } else {
//       chprintf((BaseSequentialStream*)&SD1, "No connection, Advertising Stopped\r\n");
//       WriteRead(tx_advstop, 21, tmp_data);
//       connectedFLAG = 0;
//     }
//     chprintf((BaseSequentialStream*)&SD1, "Shutting off\r\n");
//     chThdSleepMilliseconds(5000);
//   }
// }

/* Thread for sending Hi all the time */
// static THD_WORKING_AREA(waMessageThread,128);
// static THD_FUNCTION(messageThread,arg) {
//   uint8_t tx_data_test[] = {0x10, 0x00, 0x0A, 0x13, 'A', 'T', '+', 'B', 'L', 'E', 'U', 'A', 'R', 'T', 'T', 'X', '=', 'h', 'i', 0x0D, 0x0A};
//   uint8_t rx_data_test[20];
//   UNUSED(arg);
//   while (TRUE) {
//     chThdSleepMilliseconds(500);
//     WriteReadMain(tx_data_test, 23, rx_data_test);
//     chThdSleepMilliseconds(1000);
//   }
// }


static THD_WORKING_AREA(waBigDataThread,4096);
static THD_FUNCTION(bigDataThread,arg) {
  UNUSED(arg);
  chThdSleepMicroseconds(1);
  uint8_t rx_tmp_data[20];
  uint8_t rx_data[20];
  uint8_t tmp_data[20];

  while(TRUE) {
    while (!connectedFLAG) {
      chThdSleepMilliseconds(500);
      WriteRead(tx_advstart, 22, tmp_data);
      chprintf((BaseSequentialStream*)&SD1, "Advertising Started\r\n");
      chThdSleepMilliseconds(10000);
      WriteRead(tx_getconn, 21, rx_data);
      if (rx_data[4] == '1') { //Pump Data if we're here
        chprintf((BaseSequentialStream*)&SD1, "Connected\r\n");
        connectedFLAG = 1;
        break;
      } else {
        chprintf((BaseSequentialStream*)&SD1, "No connection, Advertising Stopped\r\n");
        WriteRead(tx_advstop, 21, tmp_data);
        connectedFLAG = 0;
      }
      chprintf((BaseSequentialStream*)&SD1, "Shutting off\r\n");
      chThdSleepMilliseconds(5000);
    }
    if (connectedFLAG) {
      WriteReadWrapper(bigData, 1024);
      chprintf((BaseSequentialStream*)&SD1, "BigData Sent\r\n");
      chThdSleepMilliseconds(10000);
      WriteRead(tx_disconnect, 20, rx_tmp_data);
      chprintf((BaseSequentialStream*)&SD1, "Disconnect Sent\r\n");
      connectedFLAG = 0;
      chThdSleepMilliseconds(10000);
    }
    chThdSleepMilliseconds(1000);
  }
  chThdSleepMilliseconds(100);
}

static void cmd_bluefruit(BaseSequentialStream *chp, int argc, char *argv[]) {
  UNUSED(argc);
  if(!strcmp("cmd", argv[0])) {
    uint16_t payloadSize = 0;
    
    uint8_t n = 0;

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

    WriteRead(tx_data, messageSize, rx_data);

    /* Everything below is the same */
    int i = headerSize + 1;
    //int size = 20;
    chprintf(chp, "Received:");
    while(i < 20) {
      chprintf(chp, " %c", rx_data[i]);  
      i++;
    }
    chprintf(chp, "\n\r");
    chThdSleepMilliseconds(3);
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

  //Setup the pins for the spi link on the GPIOA. This link connects to the pressure sensor and the gyro.  
  palSetPadMode(GPIOA, 5, PAL_MODE_ALTERNATE(5));     /* SCK. */
  palSetPadMode(GPIOA, 6, PAL_MODE_ALTERNATE(5));     /* MISO.*/
  palSetPadMode(GPIOA, 7, PAL_MODE_ALTERNATE(5));     /* MOSI.*/
  palSetPadMode(GPIOA, 8, PAL_MODE_OUTPUT_PUSHPULL);  /* pressure sensor chip select */
  palSetPadMode(GPIOE, 3, PAL_MODE_OUTPUT_PUSHPULL);  /* gyro chip select */
  palSetPad(GPIOA, 8);                                /* Deassert the bluetooth sensor chip select */
  palSetPadMode(GPIOC, GPIOC_PIN3, PAL_MODE_INPUT_PULLUP); /* IRQ pin */

  chprintf((BaseSequentialStream*)&SD1, "\n\rUp and Running\n\r");
  /* Initialize the command shell */ 
  shellInit();


  uint32_t i;

  for (i = 0; i < 1024; i++) {
    if (i % 5 == 0) {
      bigData[i] = '1';
    }
    else {
      bigData[i] = '0';
    }
  }
  //setup to listen for the shell_terminated event. This setup will be stored in the tel  * event listner structure in item 0
  chEvtRegister(&shell_terminated, &tel, 0);

  shelltp1 = shellCreate(&shell_cfg1, sizeof(waShell), NORMALPRIO);
  chThdCreateStatic(waBigDataThread, sizeof(waBigDataThread), NORMALPRIO+2, bigDataThread, NULL);
  //chThdCreateStatic(waCounterThread, sizeof(waCounterThread), NORMALPRIO+1, counterThread, NULL);
  //chThdCreateStatic(waMessageThread, sizeof(waMessageThread), NORMALPRIO+1, messageThread, NULL);
  //chThdCreateStatic(waAdvertisingThread, sizeof(waAdvertisingThread), NORMALPRIO+1, advertisingThread, NULL);
  while (TRUE) {
    chEvtDispatch(fhandlers, chEvtWaitOne(ALL_EVENTS));
  }
 }