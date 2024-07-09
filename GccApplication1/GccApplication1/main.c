// Includes
// ********************************************************************************

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <util/delay.h>

// ********************************************************************************
#define F_CPU 4000000UL

//KEYPAD IS ATTACHED ON PORTB
#define KEYPAD_PORT PORTB
#define KEYPAD_DDR   DDRB
#define KEYPAD_PIN   PINB

//Interrupt defines
#define THRESHOLD 1//124 in real chip
#define PRESCALE (1<<CS00)//(1<<CS02) | (1<<CS00)//(1<<CS02) in real chip
#define START 0//0x06 in real chip

//maximum number of accounts
#define ACCOUNT_COUNT 5 

//empty number 
#define EMPTY 15

volatile uint8_t seconds = 0;
volatile uint8_t minutes = 0;
volatile uint8_t hours = 0;

volatile uint8_t con = 0;

//convert time to an BCD representation
int time_corrector(int timer){
	if (timer % 16 >= 10){
		return ((timer + 6)/16 << 4) ;
	}
	else
	return timer;
}


void add_second()
{	
	seconds ++;
	seconds = time_corrector(seconds);
	if (seconds == 96){
		seconds = 0;
		minutes = minutes + 1;
		minutes = time_corrector(minutes);
		if (minutes == 96){
			minutes = 0;
			hours = hours + 1;
			hours = time_corrector(hours);
			if (hours == 36){
				hours = 0;
				minutes = 0;
				seconds = 0;
			}
		}
	}
}

// timer0 overflow
ISR(TIMER0_OVF_vect)
{
	TCNT0=START;
	con ++;
	if (con >= THRESHOLD)
	{
		con = 0;
		add_second();
	}

}

//convert keypad to number
uint8_t GetKeyPressed(){
	uint8_t r,c, number;

	KEYPAD_PORT|= 0X0F;  //0F
	
	for(c=0;c<3;c++)
	{
		KEYPAD_DDR&=~(0X7F);

		KEYPAD_DDR|=(0X40>>c);
		for(r=0;r<4;r++)
		{
			if(!(KEYPAD_PIN & (0X08>>r)))
			{
				number = (r*3+c) + 1;
				if (number==0x0B)
				return 0x00;
				else
				return number;
			}
		}
	}

	return 0XFF;//Indicate No key pressed
}

//blinking LED
void show_error()
{
	PORTA = 0;
	PORTC = 0;
	PORTD = 0;
	for (uint8_t i=0; i< 6; i++)
	{
		PORTB ^= 1<<7;
		_delay_ms(75);	
	}
}

// ********************************************************************************
// Main
// ********************************************************************************
int main( void ) {
	DDRA = 0xFF;			/* Making all 8 pins of Port A as output pins */
	DDRC = 0xFF;			/* Making all 8 pins of Port C as output pins */
	DDRD = 0xFF;			/* Making all 8 pins of Port D as output pins */
	DDRB |= 1<<7;			/* Making pin7 of Port B as output pins */
	
	uint8_t pressed = 0;
	uint8_t key;
	uint8_t indexEnteredPass = 0;
	uint8_t last_inputed_char = EMPTY;
	uint8_t logins[ACCOUNT_COUNT][4] = {{0,0,0,0},{0,0,0,0}};
	uint8_t passwords[ACCOUNT_COUNT][4] = {{1,2,3,4},{1,3,5,7}};
	uint8_t isPasswordTrue;
	uint8_t entered_password[] = {EMPTY,EMPTY,EMPTY,EMPTY};
	uint8_t addPassword = 0;
	uint8_t accountCounts = 2;
	
	uint8_t logged_user = 0;
	uint8_t log_out = 0;
	uint8_t pb7 = 0;
	
	
	// enable timer overflow interrupt for both Timer0 and Timer1
	TIMSK=(1<<TOIE0);
	// set timer0 counter initial value to 0
	TCNT0=START;
	// start timer0 with /1024 prescaler
	TCCR0=  PRESCALE;
	
	sei(); //enable interupts
	
	while(1) {
		if(last_inputed_char == EMPTY)
		{
			PORTD=seconds;
			PORTC=minutes;
			PORTA=hours;
		}

		key=GetKeyPressed();
		if (key == 0xFF )
		{
			pressed = 0;
		}
		if(key!=0xFF && pressed == 0){
			pressed = 1;
			last_inputed_char = key;
			//not logged or logged and no command
			if ((pb7==0 && key!= 0x0C && key != 0x0A) || (key!= 0x0C && key != 0x0A))
			{
				entered_password[indexEnteredPass] = key;
				indexEnteredPass++;
				//log_out = 0;
			}
			//show last login
			else if (pb7==1 && key == 0x0C)
			{
				if (addPassword == 1 )
				{
					show_error();
					PORTB &= 0 << 7;
					pb7 = 0;
					//log_out = 0;
					addPassword = 0;
					last_inputed_char = EMPTY;
					_delay_ms(500);
					
				}
				else
				{
					last_inputed_char = EMPTY;
					for (uint8_t j=0;j<accountCounts;j++)
					{
						PORTA = j+1;
						PORTC = 0xAC;
						PORTD = 0xCC;
						_delay_ms(1000);
						PORTD = logins[j][0];
						PORTC = logins[j][1];
						PORTA = logins[j][2];	
						_delay_ms(1000);
					}
				}
			}
			//add password
			else if (pb7==1 && key == 0x0A)
			{
				
				if (accountCounts < ACCOUNT_COUNT)
				{
					//log_out = 1	;
					addPassword = 1;
					_delay_ms(500);
					
				}
				else
				{
					//log_out = 1;
					last_inputed_char = EMPTY;
					_delay_ms(500);
					//show_error();
				}
			}
			else
			{
				last_inputed_char = EMPTY;
			}
			
			
			//show inputed chars
			switch (indexEnteredPass){
				
				case 1:
				PORTA = 0;
				PORTC = 0;
				if (addPassword == 1)
				{
					PORTD = 0xAD;
				}
				else 
				{
					PORTD = 0xCD;
				}
				
				PORTA = last_inputed_char<<4;
				break;
				
				case 2:
				PORTA |= last_inputed_char;
				break;
				
				case 3:
				PORTC = last_inputed_char<<4;
				break;
				
				case 4:
				PORTC |= last_inputed_char;
				break;
				
			}
			
			//4 digits entered
			if (indexEnteredPass == 4)
			{
				//adding a password
				if (addPassword == 1)
				{
					for(uint8_t j =0; j<4; j++)
					{
						passwords[accountCounts][j] = entered_password[j];
						logins[accountCounts][j] = 0;
					}
					accountCounts++;
					addPassword = 0;
					//log_out = 0;
				}
				else
				{
					//checking a password
					for(uint8_t i=0; i<accountCounts;i++)
					{
						isPasswordTrue = 1;
						for(uint8_t j =0; j<4; j++)
						{
							if(passwords[i][j] != entered_password[j])
							{
								isPasswordTrue = 0;
								break;
							}
						
						}
						if (isPasswordTrue == 1)
						{
							//logged_user = i;
							logins[i][0] = seconds;
							logins[i][1] = minutes;
							logins[i][2] = hours;
							logins[i][3]++;
							break;
						}
					
					}
				
					if (isPasswordTrue == 1)
					{
					
						PORTB |= 1 << 7;
						pb7 = 1;
					}
					else
					{
						show_error();
					}
				}
				//wait and set back everything
				_delay_ms(500);
				last_inputed_char = EMPTY;
				indexEnteredPass = 0;
				entered_password[0] = EMPTY;
				entered_password[1] = EMPTY;
				entered_password[2] = EMPTY;
				entered_password[3] = EMPTY;
			}

		}
		
	}
	
}