/************************** 
 *f3d_uart.c 
 *contains the initialization basic i/o functions for UART
****************************/ 
/*Name: Gleb Alexeev
 *Partner: Taylor Ball
 *Last edited: 2/13/2015
 */
/* Code: */

#include <stm32f30x.h>
#include <f3d_uart.h>
//the initialization function to call
void f3d_uart_init(void) { //Initialization of both the TX and RX pins, and setting the Baud rate to 9600.
  GPIO_InitTypeDef GPIO_InitStructure;
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOC, ENABLE);
  GPIO_StructInit(&GPIO_InitStructure);
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_5;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(GPIOC,&GPIO_InitStructure);
  GPIO_PinAFConfig(GPIOC,4,GPIO_AF_7);
  GPIO_PinAFConfig(GPIOC,5,GPIO_AF_7);

  USART_InitTypeDef USART_InitStructure;
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
  USART_StructInit(&USART_InitStructure);
  USART_InitStructure.USART_BaudRate = 9600;
  USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
  USART_Init(USART1 ,&USART_InitStructure);
  USART_Cmd(USART1 , ENABLE);
}

//sends a character
int putchar(int c) {
  while (USART_GetFlagStatus(USART1,USART_FLAG_TXE) == (uint16_t)RESET);
  USART_SendData(USART1, c);  
  return 0;
} 

//gets a character
int getchar(void) {
  while (USART_GetFlagStatus(USART1, USART_FLAG_RXNE) == (uint16_t)RESET);
  int c = USART_ReceiveData(USART1);
  return c;
}

//sends a string
void putstring(char *s) {
  int i;
  for(i = 0; s[i] != '\0'; i++) {
    putchar(s[i]);
  }
}

int getcharXXL(void) { //Our own get character function so that it doesn't reset
  if (USART_GetFlagStatus(USART1, USART_FLAG_RXNE) != (uint16_t)RESET) {
    int c = USART_ReceiveData(USART1);
    return c;
  }
  return 0;
}


/* f3d_uart.c ends here */
