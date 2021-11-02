/*
 * ms_son.c
 *
 * Created: 28.07.2021 16:03:31
 * Author : FATİH YILMAZ
 */ 

#ifndef F_CPU
#define  F_CPU 16000000UL
#endif

#include <avr/io.h>
#include <util/delay.h>




int duty1 = 0 , duty2 =0  , deathtime = 50 , temp_duty = 0  , limit_acc = 1;
int temp_c1 = 0 , temp_volt = 0 ,temp = 0  , temp_c2 = 0 , temp_circuit = 0 , temp_motor = 0, fan_duty = 0 ,max_c_temp = 100;
int  potvalue = 0 , duty = 0 ;
uint8_t channel ;
uint16_t result_adc = 0 ;
////////////// CURREENT SENSOR SETUP ////////////////////
double miliVoltsPerAmp = 13.3; //See the diagram for scale factors
int acOffset = 2500;  //See the diagram for offset value
int rawSense = 0;
double voltageValue = 0;
double amp = 0;

/*#define potpin       0
#define senseIn      1
#define motortemp    2
#define ctemp_1      3
#define ctemp_2      4
*/

///////////// PROTOTYPES ///////////////////////////

double read_amp();
uint16_t read_adc(uint8_t channel);
void adc_init(void);


int main(void)
{
    
	 DDRD  |= (1<<4) | (1 << 6) | (1 << 2);             //LEDS OUT
	 PORTD |= (1<<4);                          // LED ON
	 DDRB  |= (1<<1)|(1<<2);                 //TIMER1 PINS OUT
	 DDRC  &= ~( (1<<0) | (1 << 1) | (1 << 2) | (1 <<3) | (1 << 4) );    //ANALOG PINS IN
	 
	 //////////////////////// TIMER1 PWM SETUP ////////////////////
	 TCCR1A = 0;
	 TCCR1B = 0;
	 TCNT1H = 0;
	 TCNT1L = 0;
	 
	 TCCR1A |= (1<<COM1A1) | (1<<COM1B1) | (1<<COM1B0) | (1<<WGM11);
	 TCCR1B |= (1<<WGM13)  |  (1<<CS10)   | (CS11) ;                     // 16 kHz frequency
	 ICR1 = 256;                                                        //  top timer value
	 OCR1A = 0;
	 OCR1B = 0;
	 /////////////////////////////////////FAN  ADJUSTMENT PWM //////////////////
	 TCCR0A = 0;
	 TCCR0B = 0;
	 TCNT0 = 0;
	 TCCR0A |= (1<<COM0A1) | (1<<WGM00);
	 TCCR0B |= (1<<CS01);
	 OCR0A = 0;
	
	adc_init();
	
    while (1) 
    {
		ADCH = 0;
		temp_c1    = read_adc(3);
		ADCH = 0;
		temp_c2    = read_adc(4);
		ADCH = 0;
		temp_motor = read_adc(2);
		ADCH = 0;
		if ( temp_c1 > max_c_temp || temp_c2 > max_c_temp || temp_motor > 150 )
		{
			OCR1A = 0 ;
			OCR1B = 0 ;
			// alert 
			PORTD |= (1 << 2) ;
			goto exit1 ;
		}
		PORTD &= ~(1 << 2) ;
		amp = read_amp();
		
		if ( amp > 85 ) goto durak1 ;

		potvalue = read_adc(0);
		
		if (potvalue < 0)
		{
			potvalue = 0;
		}
		if(potvalue > 0 && potvalue <1024 && duty < potvalue)
		{
			for(duty = duty ; duty < potvalue ; duty++)
			{
				duty1 = duty ;
				duty2 = duty  - deathtime ;
				OCR1A = duty1 ;
				OCR1B = duty2 ;
				_delay_ms(5);
				potvalue = read_adc(0) ;
				if (duty > potvalue){
					duty = potvalue ;
					duty1 = duty  ;
					duty2 = duty  - deathtime;
					OCR1A = duty1 ;
					OCR1B = duty2 ;
					goto exit2;
				}
			}
		}else{
			 exit2:
			 duty = potvalue;
			 duty1 = duty ;
			 duty2 = duty  - deathtime;
			 OCR1A = duty1 ;
			 OCR1B = duty2 ;
		 }
		 PORTD &= ~(1 << 2) ;
		 durak1:
		 if (amp > 100)
		 {
			 duty1-- ;
			 duty2-- ;
			 PORTD |= (1 << 2) ;
			 OCR1A = duty1 ;
			 OCR1B = duty2 ;
		 }
		    if(amp >=85 && amp < 100)
		    {
			    OCR1A = duty1 ;
			    OCR1B = duty2 ;
			    potvalue = read_adc(0) ;
			    if(duty > potvalue)
			    {
				    duty = potvalue;
				    duty1 = duty ;
				    duty2 = duty  - deathtime;
				    OCR1A = duty1 ;
				    OCR1B = duty2 ;
			    }
		    }
			exit1:
			/////////////////// measuring circuit temperatures and adjusting circuit fan ///////////////////////////////////
			if (temp_c1 >= temp_c2)    {temp_circuit = temp_c1 ;}
			else               {temp_circuit = temp_c2 ;}
			
			fan_duty= temp_circuit *4  ; //20 and 102 values are min and max temperature bit limits for fan // gerekli düzenleme yapılacak
			if(fan_duty> 255)   fan_duty= 255;
			if(fan_duty< 10)   fan_duty= 0;
			OCR0A = fan_duty;
    }
}

     /////////////////// FUNCTIONS ////////////////////////////
     double read_amp()
     {
	     double  amp_1;
	     rawSense = read_adc(1);
	     voltageValue = (rawSense / 1023.0) * 5000;
	     amp_1 = ((voltageValue - acOffset)/miliVoltsPerAmp);
	     return amp_1 ;
     }
	 
		void adc_init(void)
		{
	
			ADCSRA |= ((1<<ADPS2) | (1<<ADPS1) | (1<<ADPS0));
			ADMUX  |= (1<<REFS0);
			ADCSRA |= (1<<ADEN);
			ADCSRA |= (1<<ADATE);
			ADCSRA |= (1<<ADSC);
			ADMUX  |= (1<<ADLAR);
			
		}

		uint16_t read_adc(uint8_t channel)
		{
			ADMUX  &= 0xF0;
			ADMUX  |= channel;
			DIDR0   = (1<<channel);
			ADCSRA |= (1<<ADSC);
			while (ADCSRA & (1<<ADSC));
			//result_adc = ADCH ;
			//result_adc = result_adc << 8 ;
			//result_adc = result_adc + ADCL ;
			return ADCH;
		}


