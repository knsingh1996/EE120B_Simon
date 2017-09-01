/*
 * final_project.c
 *
 * Created: 8/30/2017 3:43:18 PM
 * Author : Kushagra Singh
 */ 

#include <avr/io.h>
#include <stdlib.h>
#include "io.c"
#include <avr/interrupt.h>

volatile unsigned char TimerFlag = 0; // TimerISR() sets this to 1. C programmer should clear to 0.

// Internal variables for mapping AVR's ISR to our cleaner TimerISR model.
unsigned long _avr_timer_M = 1; // Start count from here, down to 0. Default 1 ms.
unsigned long _avr_timer_cntcurr = 0; // Current internal count of 1ms ticks
unsigned char score = 5;


void TimerOn() {
	// AVR timer/counter controller register TCCR1
	TCCR1B = 0x0B;// bit3 = 0: CTC mode (clear timer on compare)
	// bit2bit1bit0=011: pre-scaler /64
	// 00001011: 0x0B
	// SO, 8 MHz clock or 8,000,000 /64 = 125,000 ticks/s
	// Thus, TCNT1 register will count at 125,000 ticks/s

	// AVR output compare register OCR1A.
	OCR1A = 125;	// Timer interrupt will be generated when TCNT1==OCR1A
	//OCR1A = 37.5;
	// We want a 1 ms tick. 0.001 s * 125,000 ticks/s = 125
	// So when TCNT1 register equals 125,
	// 1 ms has passed. Thus, we compare to 125.
	// AVR timer interrupt mask register
	TIMSK1 = 0x02; // bit1: OCIE1A -- enables compare match interrupt

	//Initialize avr counter
	TCNT1=0;

	_avr_timer_cntcurr = _avr_timer_M;
	// TimerISR will be called every _avr_timer_cntcurr milliseconds

	//Enable global interrupts
	SREG |= 0x80; // 0x80: 1000000
}

void TimerOff() {
	TCCR1B = 0x00; // bit3bit1bit0=000: timer off
}

void TimerISR() {
	TimerFlag = 1;
}
// In our approach, the C programmer does not touch this ISR, but rather TimerISR()
ISR(TIMER1_COMPA_vect) {
	// CPU automatically calls when TCNT1 == OCR1 (every 1 ms per TimerOn settings)
	_avr_timer_cntcurr--; // Count down to 0 rather than up to TOP
	if (_avr_timer_cntcurr == 0) { // results in a more efficient compare
		TimerISR(); // Call the ISR that the user uses
		_avr_timer_cntcurr = _avr_timer_M;
	}
}

// Set TimerISR() to tick every M ms
void TimerSet(unsigned long M) {
	_avr_timer_M = M;
	_avr_timer_cntcurr = _avr_timer_M;
}

double freq[3] = {261.63, 460.63, 800.63};

void set_PWM(double frequency) {
	static double current_frequency;
	if (frequency != current_frequency) {
		if (!frequency) { TCCR3B &= 0x08; }
		if (frequency < 0.954) { OCR3A = 0xFFFF; }
		else if (frequency > 31250) { OCR3A = 0x0000; }
		else { OCR3A = (short)(8000000 / (128 * frequency)) - 1; }
		TCNT3 = 0;
		current_frequency = frequency;
	}
}

void PWM_on() {
	TCCR3A = (1 << COM3A0);
	TCCR3B = (1 << WGM32) | (1 << CS31) | (1 << CS30);
	set_PWM(0);
}

void PWM_off() {
	TCCR3A = 0x00;
	TCCR3B = 0x00;
}

char gameScore = 0;
char fail = 0;
char isDone = 1;
char isRead = 1;
unsigned char j = 0;
unsigned char out[9] = {0,1,2,0,1,2,0,1,2};
unsigned char in[9] = {0,0,0,0,0,0,0,0,0};

enum LCD_States { LCD_welcome, LCDcnt3, LCDcnt2, LCDcnt1 , LCDcurScore, LCD_GIP, LCD_lose, LCD_win } LCD_State;

void TickFct_Simon() {
	/*VARIABLES MUST BE DECLARED STATIC*/
	/*e.g., static int x = 0;*/
	/*Define user variables for this state machine here. No functions; make them global.*/
	unsigned char tempA = ~PINA & 0x07;
	switch(LCD_State) { // Transitions
		case LCD_welcome:
			if (tempA) {
				out[0] = rand()%3;
				out[1] = rand()%3;
				out[2] = rand()%3;
				out[3] = rand()%3;
				out[4] = rand()%3;
				out[5] = rand()%3;
				out[6] = rand()%3;
				out[7] = rand()%3;
				LCD_State = LCDcnt3;
			}
		break;
		case LCDcnt3:
			if (1) {
				LCD_State = LCDcnt2;
			}
		break;
		case LCDcnt2:
			if (1) {
				LCD_State = LCDcnt1;
			}
		break;
		case LCDcnt1:
			if (1) {
				LCD_State = LCD_GIP;
				gameScore++;
				isDone = 0;
			}
		break;
		case LCDcurScore:
			if (tempA) {
				LCD_State = LCDcnt3;
			}
		break;
		case LCD_GIP:
			if(isDone == 0){
				LCD_State = LCD_GIP;	
			}
			else if ((isDone == 1) && gameScore < 9 && !fail) {
				LCD_State = LCDcurScore;
			}
			else if ((isDone == 1) && fail) {
				LCD_State = LCD_lose;
			}
			else if ((isDone == 1)  && gameScore >= 9) {
				LCD_State = LCD_win;
			}
		break;
		case LCD_lose:
			if(tempA){
				gameScore = 0;
				fail = 0;
				LCD_State = LCD_welcome;
			}
		break;
		case LCD_win:
			if(tempA){
				gameScore = 0;
				fail = 0;
				LCD_State = LCD_welcome;
			}
		break;
		
		default:
		LCD_State = LCD_welcome;
	} // Transitions

	switch(LCD_State) { // State actions
		case LCD_welcome:
			LCD_Cursor(1);
			LCD_WriteData( 'W'); LCD_WriteData( 'E');LCD_WriteData( 'L');LCD_WriteData( 'C');LCD_WriteData( 'O');LCD_WriteData( 'M');LCD_WriteData( 'E');
			LCD_Cursor(17);
			LCD_WriteData( 'C');LCD_WriteData( 'L');LCD_WriteData( 'I');LCD_WriteData( 'C');LCD_WriteData( 'K');LCD_WriteData( ' ');
			LCD_WriteData( 'T');LCD_WriteData( 'O');LCD_WriteData( ' ');LCD_WriteData( 'S');LCD_WriteData( 'T');LCD_WriteData( 'A');LCD_WriteData( 'R');LCD_WriteData( 'T');
		break;
		case LCDcnt3:
			LCD_ClearScreen();
			LCD_Cursor(8);
			LCD_WriteData(3 + '0');
		//LCD 3
		break;
		case LCDcnt2:
			LCD_ClearScreen();
			LCD_Cursor(8);
			LCD_WriteData(2 + '0');
		//LCD 2
		break;
		case LCDcnt1:
			LCD_ClearScreen();
			LCD_Cursor(8);
			LCD_WriteData(1 + '0');
		//LCD 1
		break;
		case LCDcurScore:
			LCD_ClearScreen();
			LCD_Cursor(1);
			LCD_WriteData('S');LCD_WriteData('C');LCD_WriteData('O');LCD_WriteData('R');LCD_WriteData('E');LCD_WriteData(':');LCD_WriteData(gameScore + '0');
			LCD_Cursor(17);
			LCD_WriteData( 'C');LCD_WriteData( 'L');LCD_WriteData( 'I');LCD_WriteData( 'C');LCD_WriteData( 'K');LCD_WriteData( ' ');
			LCD_WriteData( 'T');LCD_WriteData( 'O');LCD_WriteData( ' ');LCD_WriteData( 'S');LCD_WriteData( 'T');LCD_WriteData( 'A');LCD_WriteData( 'R');LCD_WriteData( 'T');
		break;
		case LCD_GIP:
			LCD_ClearScreen();
			LCD_WriteData('G');LCD_WriteData('A');LCD_WriteData('M');LCD_WriteData('E');LCD_WriteData(' ');
			LCD_WriteData('I');LCD_WriteData('N');LCD_WriteData(' ');
			LCD_WriteData('P');LCD_WriteData('R');LCD_WriteData('O');LCD_WriteData('G');LCD_WriteData('R');LCD_WriteData('E');LCD_WriteData('S');LCD_WriteData('S');
		break;
		case LCD_lose:
			LCD_ClearScreen();
			LCD_WriteData('S');LCD_WriteData('O');LCD_WriteData('R');LCD_WriteData('R');LCD_WriteData('Y');LCD_WriteData(' ');
			LCD_WriteData('Y');LCD_WriteData('O');LCD_WriteData('U');LCD_WriteData(' ');
			LCD_WriteData('L');LCD_WriteData('O');LCD_WriteData('S');LCD_WriteData('T');
			LCD_Cursor(17);
			LCD_WriteData( 'C');LCD_WriteData( 'L');LCD_WriteData( 'I');LCD_WriteData( 'C');LCD_WriteData( 'K');LCD_WriteData( ' ');
			LCD_WriteData( 'T');LCD_WriteData( 'O');LCD_WriteData( ' ');LCD_WriteData( 'S');LCD_WriteData( 'T');LCD_WriteData( 'A');LCD_WriteData( 'R');LCD_WriteData( 'T');
		break;
		case LCD_win:
			LCD_ClearScreen();
			LCD_WriteData('C');LCD_WriteData('O');LCD_WriteData('N');LCD_WriteData('G');LCD_WriteData('R');LCD_WriteData('A');LCD_WriteData('T');LCD_WriteData('S');LCD_WriteData(' ');
			LCD_WriteData('Y');LCD_WriteData('O');LCD_WriteData('U');LCD_WriteData(' ');
			LCD_WriteData('W');LCD_WriteData('I');LCD_WriteData('N');LCD_WriteData(' ');
			LCD_Cursor(17);
			LCD_WriteData( 'C');LCD_WriteData( 'L');LCD_WriteData( 'I');LCD_WriteData( 'C');LCD_WriteData( 'K');LCD_WriteData( ' ');
			LCD_WriteData( 'T');LCD_WriteData( 'O');LCD_WriteData( ' ');LCD_WriteData( 'S');LCD_WriteData( 'T');LCD_WriteData( 'A');LCD_WriteData( 'R');LCD_WriteData( 'T');
		break;
		default: // ADD default behavior below
		break;
	} // State actions
}

enum Light_States { Light_wait_start, Light_on, Light_off, Light_alert, Light_wait_input } Light_State;

void TickFct_Output() {
	switch(Light_State) { // Transitions
		case Light_wait_start:
			if (isDone == 0) {
				j = 0;
				Light_State = Light_on;
			}
		break;
		case Light_on:
			if (1) {
				Light_State = Light_off;
				j++;
			}
		break;
		case Light_off:
			if (j < gameScore) {
				Light_State = Light_on;
			}
			else if (j >= gameScore) {
				Light_State = Light_alert;
			}
		break;
			case Light_alert:
			if (1) {
				Light_State = Light_wait_input;
				isRead = 0;
				PORTB = 0x00;
			}
		break;
			case Light_wait_input:
				if(isRead == 1){
					unsigned char temp = 0;
					for(temp = 0; temp < gameScore; temp++){
						if(in[temp] != out[temp]){
							fail = 1;	
						}
					}
					Light_State = Light_wait_start;
					isDone = 1;
				}
				
			break;
		default:
		Light_State = Light_wait_start;
	} // Transitions

	switch(Light_State) { // State actions
		case Light_wait_start:
		break;
		case Light_on:
			if(out[j] == 0){
				PORTB = 0x01;
				set_PWM(freq[0]);
			}else if (out[j] == 1){
				PORTB = 0x02;
				set_PWM(freq[1]);
			}else if (out[j] == 2){
				PORTB = 0x04;
				set_PWM(freq[2]);
			}
		break;
		case Light_off:
			set_PWM(1);
			PORTB = 0x00;
		break;
		case Light_alert:
			PORTB = 0x07;
		break;
		case Light_wait_input:
		break;
		default: // ADD default behaviour below
		break;
	} // State actions
}

enum ButtonIn_States { ButtonIn_wait_start, ButtonIn_wait, ButtonIn_A0, ButtonIn_A1, ButtonIn_A2 } ButtonIn_State;
	
void TickFct_Input() {
	//static unsigned char tempA = ~PINA & 0x07;
	
	switch(ButtonIn_State) { // Transitions
		case ButtonIn_wait_start:
			if (isRead == 0) {
				ButtonIn_State = ButtonIn_wait;
				j = 0;
			}
		break;
		case ButtonIn_wait:
			if (~PINA & 0x01) {
				ButtonIn_State = ButtonIn_A0;
			}
		else if (~PINA & 0x02) {
			ButtonIn_State = ButtonIn_A1;
		}
		else if (~PINA & 0x04) {
			ButtonIn_State = ButtonIn_A2;
		}
		else if (j >= gameScore) {
			ButtonIn_State = ButtonIn_wait_start;
			isRead = 1;
		}
		break;
		case ButtonIn_A0:
		if (!(~PINA & 0x01)) {
			ButtonIn_State = ButtonIn_wait;
			in[j] = 0;
			j++;
		}
		break;
		case ButtonIn_A1:
		if (!(~PINA & 0x02)) {
			ButtonIn_State = ButtonIn_wait;
			in[j] = 1;
			j++;
		}
		break;
		case ButtonIn_A2:
		if (!(~PINA & 0x04)) {
			ButtonIn_State = ButtonIn_wait;
			in[j] = 2;
			j++;
		}
		break;
		default:
		ButtonIn_State = ButtonIn_wait_start;
	} // Transitions

	switch(ButtonIn_State) { // State actions
		case ButtonIn_wait_start:
		break;
		case ButtonIn_wait:
			PORTB = 0x00;
			set_PWM(1);
			//setPWM
		break;
		case ButtonIn_A0:
			PORTB = 0x01;
			set_PWM(freq[0]);
		break;
		case ButtonIn_A1:
			PORTB = 0x02;
			set_PWM(freq[1]);
		break;
		case ButtonIn_A2:
			PORTB = 0x04;
			set_PWM(freq[2]);
		break;
		default: // ADD default behaviour below
		break;
	} // State actions
}

int main(void)
{
	DDRA = 0x00; PINA = 0xFF;
	DDRB = 0xFF; PORTB = 0x00;
	DDRC = 0xFF; PORTC = 0x00; // LCD data lines
	DDRD = 0xFF; PORTD = 0x00; // LCD control lines
	
	// Initializes the LCD display
	srand(400);
	LCD_init();
	TimerSet(100);
	TimerOn();
	PWM_on();
	set_PWM(1);
	LCD_State = LCD_welcome;
	LCD_Cursor(1);
	Light_State = Light_wait_start;
	ButtonIn_State = ButtonIn_wait_start;

	while(1) {
		TickFct_Simon(); //LCD
		TickFct_Output();
		TickFct_Input();
		while (!TimerFlag);
		TimerFlag = 0;
	}
	return 0;
}



