//Filename: Main.c part of Lab6
//Authors: Justin Smock | jtsmock, Gleb Alexeev | galexeev
//Created: 2/20/15 whatever time it was
//Last Edited: Today. Jkjk, 2/25/15, 9:34PM

#include <f3d_uart.h>
#include <stdio.h>
#include <f3d_gyro.h>
#include <f3d_led.h>
#include <f3d_user_btn.h>
#include <math.h>


int main(void){

  float a[3]; //placeholder float for getdata
  f3d_gyro_init(); //inits!!!
  f3d_uart_init();
  f3d_led_init();
  int steven = 0; //this is our cooldown, since the user button is extremely sensitive... MMMMmmmmmmm

  setvbuf(stdin, NULL, _IONBF, 0); //clear it out, you gotta do dis
  setvbuf(stdout, NULL, _IONBF, 0); //wakka wakka sky walka, wakka wakka flame
  setvbuf(stderr, NULL, _IONBF, 0); //wakka a dog on a leash wakka wakka

  int count = 0; //This is to check for what button, or for what acis of a[n]
  while(1) {
    f3d_gyro_getdata(a); //Gets the data from the axes and returns and angle
    
    //printf(" %f %f %f\n", a[0], a[1], a[2]);

    if ((user_btn_read()) && (steven > 50)) { //if cooldown is 0 and userbutton read, increment count and if its greater than 2, reset it.
      count += 1;
      if(count > 2) {
	count = 0;
      }
      steven = 0;
    }
    else { //getchar cases where you have letters (made our own method instead of always holding the button) 
      switch(getcharXXL()) {
      case 'x' : //x axis
	count = 0;
	break;
      case 'y' : //y axis
	count = 1;
	break;
      case 'z' : //z axis
	count = 2;
	break;
      default :
	break;
      }
    }

    printf("Cooldown: %i \n", steven); //cooldown printf

    if(count == 0) {
      printf("X AXIS BITCHES, ANGLE: %f \n", a[count]); //self
    }
    if(count == 1) {
      printf("Y AXIS BITCHES, ANGLE: %f \n", a[count]); //explanatory
    }
    if(count == 2) {
      printf("Z AXIS BITCHES, ANGLE: %f \n", a[count]); // :)
    }
    f3d_led_all_off();
    if(a[count] > 5.000) { //All the led cases, we tried different angles, but yeah, it's exciting! :D
      f3d_led_on(0);
    }
    if(a[count] > 80.000) {
      f3d_led_on(7);
    }
    if(a[count] > 155.000) {
      f3d_led_on(6);
    }
    if(a[count] > 230.000) {
      f3d_led_on(5);
    }
    if(a[count] > 305.000) {
      f3d_led_on(4);
    }
    if(a[count] < -5.000) {
      f3d_led_on(0);
    }
    if(a[count] < -80.000) {
      f3d_led_on(1);
    }
    if(a[count] < -155.000) {
      f3d_led_on(2);
    }
    if(a[count] < -230.000) {
      f3d_led_on(3);
    }
    if(a[count] < -305.000) {
      f3d_led_on(4);
    }
    //So according to the lab instructions, the North is supposed to light up, and then the east lights up for 
    //the negatives, and positives are west?? Okay...
    steven++; //increment cooldown every time/
  }
}

void assert_failed(uint8_t* file, uint32_t line) {
/* Infinite loop */
/* Use GDB to find out why we're here */
  while (1);
}
