/* Server Room Monitoring System using PIC18 Group Project */
/* Authors: Maksymilian Cwirzen, Nicholas Kyprianou */

#pragma config OSC = HS 	//Set osc mode to HS 
#pragma config WDT = OFF 	//Set watchdog timer off
#pragma config LVP = OFF 	//Low Voltage Programming Off
#pragma config DEBUG = OFF 	//Compile without extra Debug compile Code
#pragma config MCLRE = ON

#include <p18f4520.h>
#include <timers.h>
#include <delays.h>		
#include <stdlib.h>
#include <usart.h>
#include <adc.h>

float v_result, t_result; 		//Initialisation of float type variables
int v_dig1, v_dig2, v_dig3, v_rem, v_intex, t_dig1, t_dig2, t_dig3, t_rem, t_intex, counter, y, z; //Initialisation of int type variables
unsigned int cache[5]; 			//Initialisation of array used to store temperature values later on
unsigned int pointer = 0;		//Initialisation of a pointer used to point at and replace the oldest piece of data in the array
int five = 1; 					//Initialisation of variable which counts up to 5 in order to make sure that the average temperature is calculated correctly
int avg = 0; 					//Initialisation of variable that will store the average temperature

#define POWER PORTCbits.RC4 	//Sets the pin RC4 as POWER, will be referred to later
#define FANS PORTCbits.RC5		//Sets the pin RC5 as FANS, will be referred to later

void COOLING(void); 			//Initialisation of the COOLING function for interrupt
void TMR0_ISR(void); 			//Initialisation of the Timer0 interrupt service routine 		

#pragma code overheat = 0x08 	//High priority overheating funtion
void overheat(void) { 			//Assembly code that allows the interrupt ot 
   _asm
   GOTO COOLING
   _endasm
}
#pragma code	//Allows the program to link back to this point

#pragma interrupt COOLING 
void COOLING(void){ 		//Checks to see if the Timer0 interrupt flag has been triggered
   if (INTCONbits.TMR0IF == 1) {
      TMR0_ISR(); 			//If the overflow flag is 1 then go to the Timer0 interrupt service routine
   }
}

void TMR0_ISR (void) { 		//Timer0 interrupt service routine
   INTCONbits.TMR0IE = 0; 	//Turns off interrupts 
   counter--; 				//Decrements the counter from 300-0
   if (counter == 0) { 		//Everything inside the if statement occurs every 5 seconds
      int i;		
      counter = 300; 		//Resets counter
      avg = 0; 				//Resets the average
      
      cache[pointer] = t_intex; 	//Adds the latest temperature value to the tempeature array

      for (i = 0; i < five; i++) {avg = avg + cache[i];}	//The variable "five" counts up from 1-5 so that before 25 seconds have passed there is still a value displayed on the 7 segments. Gathers the sum of the elements inside the array 
      avg = avg/five;	//Generates the average temperature from the above line to be displayed on the 7 segments
      pointer++;	//Itterates the pointer in the array
      if (five < 5) {five++;}			//Itterates "five" until it reaches the value of 5. Very important for the first 5 readings
      if (pointer > 4) {pointer = 0;} 	//Resets the pointers position to the 0th value of the array when over 4 so the array does not overflow
   }
   WriteTimer0(65535-5000); 	//Resets Timer0
   INTCONbits.TMR0IE = 1;		//Turns on interrupts
   INTCONbits.TMR0IF = 0; 		//Resets Timer0 interrupt flag
} 

unsigned char address = 0x00;
unsigned char databyte;
char password[4] = {0,0,0,0}; 	//Array that forwards the password to the EEPROM

unsigned char EEPROM_READ(unsigned char address); 	//Function for reading the EEPROM
void EEPROM_Write(unsigned char address, unsigned char databyte); 	//Function for editing the EEPROM
void timeout(); 	//Function for the 60 second timeout

void main() { 
      
   //EEPROM_READ(0xFF); EEPROM_Write(0xFF, 0xFF); //This code was used to reset the PIC for our own testing
   
   address = 0x00;
   TRISA = 0xFF;
   TRISD = 0x00;
   TRISCbits.RC0 = TRISCbits.RC1 = TRISCbits.RC2 = TRISCbits.RC3 = TRISCbits.RC4 = TRISCbits.RC5 = 0; 
   TRISCbits.RC6 = 0;
   TRISCbits.RC7 = 1; 
	
   OpenUSART(USART_TX_INT_OFF & USART_RX_INT_OFF & USART_ASYNCH_MODE & USART_EIGHT_BIT & USART_CONT_RX & USART_BRGH_HIGH, 128);
  
   if (EEPROM_READ(0xFF) == 0xFF) {  //Code for the initial setup of the PIC. If passwords have been set previously this step is skipped
      char users;
      int firstCounter;
      int secondCounter;
      putrsUSART(" Enter the number of users: \r\n ");
      while(BusyUSART());
      while(!DataRdyUSART());
      users = getcUSART(); 	//Input the number of users
      putcUSART(users);		//Send the string message
	  while(BusyUSART());
	  putrsUSART("\r\n");
	  while(BusyUSART());
      for (firstCounter = 48; firstCounter < users; firstCounter++) { 	//Loop that counts the users
		putrsUSART(" Enter the 4-digit password for the user: \r\n "); 	//Send the string message
		while(BusyUSART());
		for (secondCounter = 0; secondCounter < 4; secondCounter++) { //Loop that writes the password inside the EEPROM
			while(!DataRdyUSART());
			password[secondCounter]  = getcUSART(); //Fill up password[] using the input
			putcUSART(password[secondCounter]); 	//Start new line
			while(BusyUSART());
			EEPROM_READ(address);
			EEPROM_Write(address, password[secondCounter]);
			address++;
	 }
      }
      EEPROM_READ(0xFF); 
      EEPROM_Write(0xFF, 0x00); //Writes the last bit of EEPROM, so the program knows that it has been initialized and by the next launch, these lines will be ignored
   }
   
   while (1) { //Code that asks for the password, checks if it is correct then proceeds as appropriate

      putrsUSART(" Enter Password: "); //Send the string message
      while(BusyUSART());
      while(!DataRdyUSART());
      getsUSART(password, 4); //Input password
	  putrsUSART("****\r\n");
      while(BusyUSART());
      if (password[0] == EEPROM_READ(0x00) && password[1] == EEPROM_READ(0x01) && password[2] == EEPROM_READ(0x02) && password[3] == EEPROM_READ(0x03)) {
	 break;
      }
      else if (password[0] == EEPROM_READ(0x04) && password[1] == EEPROM_READ(0x05) && password[2] == EEPROM_READ(0x06) && password[3] == EEPROM_READ(0x07)) {
	 break;
      }
      else if (password[0] == EEPROM_READ(0x08) && password[1] == EEPROM_READ(0x09) && password[2] == EEPROM_READ(0x0A) && password[3] == EEPROM_READ(0x0B)) {
	 break;
      }
      else if (password[0] == EEPROM_READ(0x0C) && password[1] == EEPROM_READ(0x0D) && password[2] == EEPROM_READ(0x0E) && password[3] == EEPROM_READ(0x0F)) {
	 break;
      }
      else if (password[0] == EEPROM_READ(0x10) && password[1] == EEPROM_READ(0x11) && password[2] == EEPROM_READ(0x12) && password[3] == EEPROM_READ(0x13)) {
	 break;
      }
      else if (password[0] == EEPROM_READ(0x14) && password[1] == EEPROM_READ(0x15) && password[2] == EEPROM_READ(0x16) && password[3] == EEPROM_READ(0x17)) {
	 break;
      }
      else if (password[0] == EEPROM_READ(0x18) && password[1] == EEPROM_READ(0x19) && password[2] == EEPROM_READ(0x1A) && password[3] == EEPROM_READ(0x1B)) {
	 break;
      }
      else if (password[0] == EEPROM_READ(0x1C) && password[1] == EEPROM_READ(0x1D) && password[2] == EEPROM_READ(0x1E) && password[3] == EEPROM_READ(0x1F)) {
	 break;
      }
      else if (password[0] == EEPROM_READ(0x20) && password[1] == EEPROM_READ(0x21) && password[2] == EEPROM_READ(0x22) && password[3] == EEPROM_READ(0x23)) {
	 break;
      }
      else {
	 timeout();
      }
      putrsUSART( " \r\n " ); //Start new line
      while(BusyUSART());
   } 	//The program breaks out of the while(1) loop if the password is correct and proceeds to the next one

   putrsUSART(" Access Granted \r\n ");
   while(BusyUSART());
   
   y = 0;
   z = 0;
		
   OpenTimer0 (TIMER_INT_ON & T0_16BIT & T0_SOURCE_INT & T0_PS_1_16);
   counter = 300; 	//Initialisation of the timers counter to have a 5 second period
   WriteTimer0(65535-5000);
		
   INTCONbits.GIE = 1; 		//Enable global interrupts  
   INTCONbits.INT0IE = 1;	//Turns on generic interrupts
   INTCONbits.INT0IF = 0;	//Clears interrupt flag
   
   INTCONbits.TMR0IE = 1; 	//Turns on Timer0 interrupots
   INTCONbits.TMR0IF = 0; 	//Clears Timer0 interrupt flag
   
   while (1) {
	   
      //The following 6 lines are the ADC setup for monitoring voltage 
      OpenADC(ADC_FOSC_32 & ADC_RIGHT_JUST & ADC_20_TAD,ADC_CH0 & ADC_INT_OFF & ADC_VREFPLUS_VDD & ADC_VREFMINUS_VSS, 0b1011); //VDD taken as the reference voltage
      SetChanADC(ADC_CH0); 
      ConvertADC( ); 
      while(BusyADC( )); 
      v_result = ReadADC();
      CloseADC();
		
      //The following 6 lines are the ADC setup for monitoring temperature
      OpenADC(ADC_FOSC_32 & ADC_RIGHT_JUST & ADC_20_TAD,ADC_CH1 & ADC_INT_OFF & ADC_VREFPLUS_EXT & ADC_VREFMINUS_VSS, 0b1011); //Temperature readings used a reference voltage of 1V
      SetChanADC(ADC_CH1); 
      ConvertADC( ); 
      while(BusyADC( )); 
      t_result = ReadADC();
      CloseADC();
		
      //The following lines are the voltage ADC calibration and calculations to give an appropriate reading
      v_result = v_result / 2.046;
      v_intex = v_result;
      v_dig1 = v_intex / 100;
      v_rem = v_intex % 100;
      v_dig2 = v_rem / 10;
      v_dig3 = v_rem % 10;
      /*LATD = (v_dig1 << 4) | v_dig2; //These two lines were used to display the voltage on the 7 segments for visualsation whilst calibrating it serves no functional purpose for final build, but was very useful when testing
      LATC = v_dig3; */ 
		
      //The following lines are the temperature ADC calibration and calculations to give an appropriate reading
      t_result = t_result / 1.023;
      t_intex = t_result;
      t_dig1 = avg / 100;
      t_rem = avg % 100;
      t_dig2 = t_rem / 10;
      t_dig3 = t_rem % 10;
      LATD = (t_dig1 << 4) | t_dig2; //Displays the temperature values that have been read onto the 7 segment displays
      LATC = t_dig3;
	
      if (v_dig1 <= 4 && v_dig2 <5 || v_dig1 <4) {POWER = 1; if (y == 0){putrsUSART(" Supplied power is unstable, switching to Failsafe Power Supply. \r\n"); y = 1;}}	//Turns on the green POWER LED if the voltage meets or exceeds 4.5V...
      else {POWER = 0; if (y == 1){putrsUSART(" Power is now stable, returning to Internal Power Source. \r\n"); y = 0;}} 	//... Otherwise remains off
	       
      if (t_dig1 >= 2) {FANS = 1; if (z == 0){putrsUSART(" Temperature exceeding safe levels, turning on Cooling Systems. \r\n"); z = 1;}}	//Turns on the red FANS LED if the average temperature meets of exceeds 20 degrees Celcius... 
      else {FANS = 0; if (z == 1){putrsUSART(" Temperature has reduced to acceptable levels, turning off Cooling Systems. \r\n"); z = 0;}} //... Otherwise remains off
	  
   }
}

void timeout() {	//Code when password is not correct
   int i;
   putrsUSART(" Access Denied. \r\n");
   while(BusyUSART());
   putrsUSART(" You have been timed out for 60 seconds. \r\n");
   while(BusyUSART());
   for (i = 0; i < 91; i++) {Delay10KTCYx(255);} //~60 second delay
} 

unsigned char EEPROM_READ(unsigned char address) { //Function used for reading the given EEPROM address
   EECON1bits.EEPGD = 0;
   EECON1bits.CFGS = 0;
   EEADR = address;
   EECON1bits.RD = 1;
   return EEDATA;
}

void EEPROM_Write(unsigned char address, unsigned char databyte) { //Function used for writing on the given EEPROM address
   EECON1bits.EEPGD = 0;
   EECON1bits.CFGS = 0;
   EEDATA = databyte;
   EEADR = address;
   EECON1bits.WREN = 1;
   INTCONbits.GIE = 0; 			//Disable interrupts
   EECON2 = 0x55;
   EECON2 = 0xAA;
   EECON1bits.WR = 1;
   INTCONbits.GIE = 1;			//Re-enable interrupts
   while(EECON1bits.WR == 1);	//Wait the write be complete
   EECON1bits.WREN = 0; 		//Disable writing
}
