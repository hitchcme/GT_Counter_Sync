#include <Arduino.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define TOPBIT 0x80000000
#define POS 0
#define NEG 1
#define HOLDING 1
#define NOT_HOLDING 0
#define HR_D0 = 0
#define MARK = 1
#define SPACE = 0

#define HR_BIN_D0 = 11
#define HR_BIN_D1 = 12
#define HR_BIN_D2 = 13
#define HR_BIN_D3 = 14

#define MIN_D1_BIN_D0 = 16
#define MIN_D1_BIN_D1 = 17
#define MIN_D1_BIN_D2 = 18
#define MIN_D1_BIN_D3 = 19

#define MIN_D0_BIN_D0 = 21
#define MIN_D0_BIN_D1 = 22
#define MIN_D0_BIN_D2 = 23
#define MIN_D0_BIN_D3 = 24

#define SEC_D1_BIN_D0 = 26
#define SEC_D1_BIN_D1 = 27
#define SEC_D1_BIN_D2 = 28
#define SEC_D1_BIN_D3 = 29

#define SEC_D0_BIN_D0 = 31
#define SEC_D0_BIN_D1 = 32
#define SEC_D0_BIN_D2 = 33
#define SEC_D0_BIN_D3 = 34

#define mSEC_D2_BIN_D0 = 36
#define mSEC_D2_BIN_D1 = 37
#define mSEC_D2_BIN_D2 = 38
#define mSEC_D2_BIN_D3 = 39

#define mSEC_D1_BIN_D0 = 41
#define mSEC_D1_BIN_D1 = 42
#define mSEC_D1_BIN_D2 = 43
#define mSEC_D1_BIN_D3 = 44

#define mSEC_D0_BIN_D0 = 46
#define mSEC_D0_BIN_D1 = 47
#define mSEC_D0_BIN_D2 = 48
#define mSEC_D0_BIN_D3 = 49

#define pi          3.141592653589793238462643383279502884L /* pi */
#define pi_o_180    3.141592653589793238462643383279502884L/180.000000000000000000L /* pi */
#define f2          125000.000L/63.00000L /* frequency @ prescaler value 62 */
#define f1          125000.000L/62.00000L /* freq     @                 61 */

//float cal_fact = 1.00011552063f;  // 		     (2_sec)  / (15_hr)  	=  0.000037037037037
float cal_fact   = 1.00011530582f;  //
//float cal_fact = 1.00010217865f;  //		     (-8*500_uS)/ (2_min)	= -0.000033333333333333
//float cal_fact = 1.0001110427f;   //		     (-4*500_uS)/ (1_min)	= -0.000033333333333334
//float cal_fact = 1.00007848359f;  // approximately (-1_sec) / (30_min) 	= -0.00055555555555


int BITS = 0;
int SYNC = 1;
float ratio62_to_61;
float freq;
float offset = 0;
float freq_adj;
int xngtp;
int numer;
int denom;
int numer1;
int denom1;
int x3;
int ocp;
float ocp1;



int GT_PIN = A5;
int TOGGLE_CNTPOL_STATE;
int TOGGLE_HOLD_STATE;

int x = 0;
boolean atvgtz = 0;

//Command Control
String readString = String(30);
int str_len;
int tim_set_beg;
int tim_set_end;
String tim_set_str = String(8);

int cal_fact_beg;
int cal_fact_end;
String cal_fact_str = String(8);

volatile float othours;
volatile float thours;
boolean toggle0 = 0;
boolean HOLD = NOT_HOLDING;
boolean CNTPOL = NEG;
volatile int p = 0;
int PLS = 0;
long HmS = 0;

boolean pV[50] = {
  0, 1, 1, 1, 1, 0, 1, 1, //Sync Word  0=>7 
  HOLD, //Holding or Not        8=>8
  POS, //Count Polarity                9=>9
  0,                            //    10=>10
  0, 0, 0, 0,    // Hours             11=>14
  0,             //                   15=>15
  0, 0, 0, 0,    // Min_D1            16=>19
  0,             //                   20=>20
  0, 0, 0, 0,   // MIN_D0             21=>24
  0,             //                   25=>25 
  0, 0, 0, 0,   // SEC_D1             26=>29
  0,             //                   30=>30
  0, 0, 0, 0,   // SEC_D0             31=>34
  0,             //                   35=>35
  0, 0, 0, 0,    // mSEC_D2           36=>39
  0,             //                   40=>40
  0, 0, 0, 0,    // mSEC_D1           41=>44
  0,             //                   45=>45
  0, 0, 0, 0,   // mSEC_D0            46=>49
};


int gcd(int n, int m)
{
    int gcd, remainder;
    while (n != 0)
    {
        remainder = m % n;
        m = n;
        n = remainder;
    }
    gcd = m;
    return gcd;
}


void offset_calc(float cal_fact) {
    
    freq_adj = 100 - 100 * ( 1000000 / (cal_fact * 500 ) - f2 ) / ( f1 - f2 ) - 0.40000000008L;
    numer1 = ( freq_adj + 0.50000000L ) / gcd( freq_adj + 0.50000000L, 10000.0000L);
    denom1 = 10000.0000L / gcd(freq_adj + 0.5000000L, 10000.0000L);
    offset = (float) ((freq_adj - 50.0000000L) / 10.00000000L);
    //offset = (float) ( 1 / 2 ) - (float) (numer1/denom1);
    //offset = (float) 0.5 - (float) numer1/denom1/100;
    for (int x3 = 91; x3 <= 269; x3++) {
        ocp1 = (62 + offset + ((float) sin(x3*pi_o_180) / 2 ));
        ocp = (int) ocp1 + 0.5;
        if (ocp < 62) {
            xngtp = x3 - 1;
            x3 = 271;
        }
        else {
            xngtp = 270;
        }
    }
    ratio62_to_61 = (float) ( 2*(xngtp - 90))/( 2*(270 - xngtp));
    numer = 2*(xngtp - 90) / gcd(2*(xngtp - 90), 2*(270 - xngtp));
    denom = 2*(270 - xngtp) / gcd(2*(xngtp - 90), 2*(270 - xngtp));
    freq = ((float) ((long) f2 * numer) + ((long) f1 * denom)) / (denom + numer);
    //return offset;

}

void GT_Sync_PGT() {

	while ((PIND & (0<<PD3))) {
	}
	int PULSE = 0;
	delayMicroseconds(30);
	while ((PIND & (0<<PD2)) && p % 5 == 0) {
        }
        if ( (PIND & 1<<PD2) && (PIND & 1<<PD3) ) {
			if (p == 8) {
				HOLD = NOT_HOLDING;
			}			
			else if (p > 10 && pV[p - 4] && pV[p - 3] && pV[p - 2] == HOLD && pV[p - 6] && !pV[p - 5] && pV[p-7] && pV[p - 8] && pV[p - 9] && !pV[p - 10]) {
				p = 10;
			}
			else if ( p == 5 && (!pV[1] || !pV[2] || !pV[3] || !pV[4])) {
				pV[1] = 1;
				pV[2] = 1;
				pV[3] = 1;
				pV[4] = 1;
				p += 5;
			}
		
                	PORTC = (1<<PC5);
                	pV[p] = 0;
	}
	else {
		if (p == 8) {
			HOLD = HOLDING;			
		}
		PORTC = (0<<PC5);
		pV[p] = 1;
	}
	delayMicroseconds(220);
	PORTC = (0<<PC5);
	//delayMicroseconds(150);
	if (p < 49) {
		p++ ;
	}
	else {
		p = 0;
	}
}
void pV_Shift() {
	for ( int i = 0; i <= 49; i++ ) {
		if (i < 49) {
			pV[i] = pV[i+1];
		}
		else {
			pV[i] = 0;
		}
	}
}

void setup() {
 
  cli();//stop interrupts
  offset_calc(cal_fact);

//set timer0 interrupt at 2kHz
  TCCR0A = 0;// set entire TCCR0A register to 0
  TCCR0B = 0;// same for TCCR0B
  TCNT0  = 0;//initialize counter value to 0
  // set compare match register for 2khz increments
  OCR0A = 62;// = (16*10^6) / (2*2000*64) - 1 (must be <256)
  // turn on CTC mode
  TCCR0A |= (1 << WGM01);
  // Set CS01 and CS00 bits for 64 prescaler
  TCCR0B |= (1 << CS01) | (1 << CS00);   
  // enable timer compare interrupt
  TIMSK0 |= (1 << OCIE0A);
  
  Serial.begin(9600);
  // Pins A4 and A5 Direct Port control outputs
  // A5 GT Out
  // A4 2kHz Square wave pin
  DDRC = 0b00110000;
  //DDRC |= (1<<PC5);  // Pin A5 Output
  //DDRC |= (1<<PC4);  // Pin A5 Output
  //PORTC = (1<<PC5);  
  //PORTC = (1<<PC4);
  Serial.print("numder = ");
  Serial.println(numer);
  Serial.print("denom = ");
  Serial.println(denom); 
  //Serial.print("freq_adj = ");
  //Serial.println(freq_adj);
  Serial.print("offset = ");
  Serial.println(offset);
  if (SYNC) {

	//DDRD = DDRD | B11110000;
	DDRD = 0<<PD2;
	DDRD = 0<<PD3;
	PORTD = 1<<PD2;
	PORTD = 1<<PD3;
	//PORTD |= ((0<<PD2) | (0<<PD3));
	attachInterrupt(1, GT_Sync_PGT, RISING);
  }
sei();
}

void loop(){
  if (Serial.available()) {
    char c = Serial.read();
    if (readString.length() < 40 ) {
        readString += c;
    }  
    if (c == '\n') {
        if (readString.indexOf("count_negative") != -1) {
                if (pV[9] == NEG) {
                        CNTPOL = POS;
                }
                else {
                        CNTPOL = NEG;
                }
                HOLD = NOT_HOLDING;
                pV[8] = NOT_HOLDING;
                Serial.println("Counting Negative\n");
        }
        else if (readString.indexOf("count_positive") != -1) {
                if (pV[9] == NEG) {
                        CNTPOL = NEG;
                }
                else {
                        CNTPOL = POS;
                }
                HOLD = NOT_HOLDING;
                pV[8] = NOT_HOLDING;
                Serial.println("Counting Positive\n");
        }
        else if (readString.indexOf("sign_positive") != -1) {
                pV[9] = POS;
                Serial.println("Time is Positive\n");
        }
        else if (readString.indexOf("sign_negative") != -1) {
                pV[9] = NEG;
                Serial.println("Time is Negative\n");
        }
	else if (readString.indexOf("cal_fact_") != -1) {
                str_len = readString.length();                                // Get String Length
                cal_fact_beg = readString.indexOf("cal_fact_")+9;              // Store beginning of cal factor setting position to integer
                cal_fact_end = tim_set_beg + 8;                                // Store end of cal factor setting position to integer
                cal_fact_str = readString.substring(tim_set_beg,tim_set_end);  // Store the cal factor setting to a string
		Serial.println(cal_fact_str);
//		int ch = cal_fact_str[0];
//		calfact_wv = (int) ch - '0';
//		for ( int i = 2; i <= 8; i++) {
//			calfact_wv = calfact_wv + (( (int) cal_fact_str[i] - (int) '0' )/(pow((double) 10, (double) i )));
//			Serial.println(calfact_wv, DEC);
//		}
//		cal_fact_str = "";
	}
        else if (readString.indexOf("get_time") != -1) {
		if (!BITS) {
			int hr;
			int min;
			int sec;
			int msec;

			hr = pV[11]+pV[12]*2+pV[13]*2*2+pV[14]*2*2*2;
			min = 10*(pV[16]+pV[17]*2+pV[18]*2*2+pV[19]*2*2*2)+(pV[21]+pV[22]*2+pV[23]*2*2+pV[24]*2*2*2);
			sec = 10*(pV[26]+pV[27]*2+pV[28]*2*2+pV[29]*2*2*2)+(pV[31]+pV[32]*2+pV[33]*2*2+pV[34]*2*2*2);
			msec = 100*(pV[36]+pV[37]*2+pV[38]*2*2+pV[39]*2*2*2)+10*(pV[41]+pV[42]*2+pV[43]*2*2+pV[44]*2*2*2)+1*(pV[46]+pV[47]*2+pV[48]*2*2+pV[49]*2*2*2);

			if (pV[9] == NEG) {
				Serial.print("-");
			}
			else {
				Serial.print("+");
			}
			Serial.print(hr, DEC);
			Serial.print(":");

			if (min <= 9 ) {
				Serial.print('0');
			}
			Serial.print(min, DEC);
			Serial.print(":");

			if (sec <= 9 ) {
				Serial.print(0, DEC);
			}
			Serial.print(sec, DEC);
			Serial.print(".");

			if (msec <= 9) {
				Serial.print(0, DEC);
			}
			if (msec <= 99) {
				Serial.print(0, DEC);
			}
			Serial.println(msec, DEC);

			
		}
		else if (BITS) {
			for ( int i = 0; i < 10; i++ ) {
				if (i % 5 == 0 || i == 0) {
					Serial.print('T');
				}
				else if(i == 8 && pV[i] == NOT_HOLDING) {
					Serial.print("C");
				}
				else if(i == 8 && pV[i] == HOLDING) {
					Serial.print("H");
				}
				else if(i == 9 && pV[i] == NEG) {
					Serial.print("N");
				}
				else if (i == 9 && pV[i] != NEG) {
					Serial.print("P");
				}
				else if (pV[i]) {
					Serial.print("_ ");
				}
				else if (!pV[i]) {
					Serial.print("-- ");
				}
			}
			Serial.print("\n");
			for (int i = 10; i < 20; i++) {
				if (i % 5 == 0) {
					Serial.print('T');
				}
                                else if (pV[i]) {
                                        Serial.print("_ ");
                                }
                                else if (!pV[i]) {
                                        Serial.print("-- ");
                                }
                                else {
                                        Serial.print("\n");
                                }
			}
                        Serial.print("\n");
                        for (int i = 20; i < 30; i++) {
                                if (i % 5 == 0) {
                                        Serial.print('T');
                                }
                                else if (pV[i]) {
                                        Serial.print("_ ");
                                }
                                else if (!pV[i]) {
                                        Serial.print("-- ");
                                }
			}
                        Serial.print("\n");
                        for (int i = 40; i < 50; i++) {
                                if (i % 5 == 0) {
                                        Serial.print('T');
                                }
                                else if (pV[i]) {
                                        Serial.print("_ ");
                                }
                                else if (!pV[i]) {
                                        Serial.print("-- ");
                                }
			}
		}

	}
	else if (readString.indexOf("TM_Bits") != -1) {
		BITS = 1;
	}
	else if (readString.indexOf("TM_Time") != -1) {
		BITS = 0;
	}
        else if (readString.indexOf("hold") != -1) {
                HOLD = HOLDING;
                pV[8] = HOLDING;
                Serial.println("Holding");
        }
	else if (readString.indexOf("Master")!= -1) {
		TCCR0A = 0;// set entire TCCR0A register to 0
  		TCCR0B = 0;// same for TCCR0B
  		TCNT0  = 0;//initialize counter value to 0
  		// set compare match register for 2khz increments
  		OCR0A = 62;// = (16*10^6) / (2*2000*64) - 1 (must be <256)
  		// turn on CTC mode
  		TCCR0A |= (1 << WGM01);
  		// Set CS01 and CS00 bits for 64 prescaler
  		TCCR0B |= (1 << CS01) | (1 << CS00);
  		// enable timer compare interrupt
  		TIMSK0 |= (1 << OCIE0A);
		SYNC = 0;
		detachInterrupt(1);
		sei();
	}
	else if (readString.indexOf("Slave") != -1) {
		cli();
		SYNC = 1;
		OCR0A = 255;
	        //DDRD = DDRD | B11110000;
       	 	DDRD = 0<<PD2;
        	DDRD = 0<<PD3;
        	PORTD = 1<<PD2;
        	PORTD = 1<<PD3;
		
        	attachInterrupt(1, GT_Sync_PGT, RISING);
		sei();
	}
        else if (readString.indexOf("set_time_") != -1) {
                HOLD = HOLDING;
                pV[8] = HOLDING;
                str_len = readString.length();                                // Get String Length
                tim_set_beg = readString.indexOf("set_time_")+9;              // Store beginning of time setting position to integer
                tim_set_end = tim_set_beg + 8;                                // Store end of time setting position to integer
                tim_set_str = readString.substring(tim_set_beg,tim_set_end);  // Store the time setting to a string

                // Process the Time Setting String into a bunch of weird backwards binary stuff
                for (int i = 0; i < 8; i++) {
                  int ch = tim_set_str[i];
                  int ch_ct_num = ch - '0';
                      if (ch_ct_num >= 8 && ch_ct_num <= 9) {
                        pV[14+5*i] = 1;
                        pV[13+5*i] = 0;
                        pV[12+5*i] = 0;
                        pV[11+5*i] = ch_ct_num-8;
                      }
                      else if ( ch_ct_num >= 6 && ch_ct_num <= 7 ) {
                        pV[14+5*i] = 0;
                        pV[13+5*i] = 1;
                        pV[12+5*i] = 1;
                        pV[11+5*i] = ch_ct_num-6;
                      }
                      else if ( ch_ct_num >= 4 && ch_ct_num <= 5 ) {
                        pV[14+5*i] = 0;
                        pV[13+5*i] = 1;
                        pV[12+5*i] = 0;
                        pV[11+5*i] = ch_ct_num-4;
                      }
                      else if ( ch_ct_num >= 2 && ch_ct_num <= 3 ) {
                        pV[14+5*i] = 0;
                        pV[13+5*i] = 0;
                        pV[12+5*i] = 1;
                        pV[11+5*i] = ch_ct_num-2;
                      }
                      else if ( ch_ct_num == 1 ) {
                        pV[14+5*i] = 0;
                        pV[13+5*i] = 0;
                        pV[12+5*i] = 0;
                        pV[11+5*i] = 1;
                      }
                      else{
                        pV[14+5*i] = 0;
                        pV[13+5*i] = 0;
                        pV[12+5*i] = 0;
                        pV[11+5*i] = 0;
                      }
                }
                
                Serial.println("time is set to");
                Serial.print(tim_set_str);
                tim_set_str = "";
                tim_set_beg = 0 ;
                tim_set_end = 0 ;
        }
        else {
           //Serial.println("Command Not Recognized\n"); 
           //Serial.println(readString);
        }
        
        readString = "";
    }

  }

othours = thours;
thours = (pV[11]+pV[12]*2+pV[13]*2*2+pV[14]*2*2*2) + ((10*(pV[16]+pV[17]*2+pV[18]*2*2+pV[19]*2*2*2)+(pV[21]+pV[22]*2+pV[23]*2*2+pV[24]*2*2*2)) / 60) + (((10*(pV[26]+pV[27]*2+pV[28]*2*2+pV[29]*2*2*2)+(pV[31]+pV[32]*2+pV[33]*2*2+pV[34]*2*2*2)) / 60) / 60) + ((((100*(pV[36]+pV[37]*2+pV[38]*2*2+pV[39]*2*2*2)+10*(pV[41]+pV[42]*2+pV[43]*2*2+pV[44]*2*2*2)+1*(pV[46]+pV[47]*2+pV[48]*2*2+pV[49]*2*2*2))/ 60) / 60) / 60);

if ((float) thours - othours > 0) {
	CNTPOL = POS;
}
else if ((float) thours - othours < 0) {
	CNTPOL = NEG;
}

}

ISR(TIMER0_COMPA_vect) {

  if (SYNC) {

  }
  else if (!SYNC) {
  	x++;
  	if (x < numer) {
        	OCR0A = 61;
  	}
  	else if ( x >= numer && x < (int) numer + denom ) {
        	OCR0A = 62;
 	}
  	else if ( x >= (int) numer + denom ) {
        	x = 0;
  	}

  	if (pV[p] && !PLS) {
    		PORTC = (0<<PC5);
    		PLS=1;
  	}
  	else if (!pV[p] && !PLS) {
    		PORTC = (1<<PC5);
    		PLS=1;
  	}
  	else{
    		PORTC = (0<<PC5);
    		PLS = 0;
    		if ( p < 49) {
      			p++ ;
    		}
    		else{
      			p = 0;
    		}
  	}
  
    	HmS = HmS + 1 ;
    
if (HmS >= 4 && CNTPOL == POS && HOLD==NOT_HOLDING) {
  HmS = 0;
  if (pV[49] && !pV[48] && !pV[47] && pV[46]) {  // 0b1001 = 9
     //Bits Maxed, cycle back to Zero
    pV[49] = 0;
    pV[48] = 0;
    pV[47] = 0;
    pV[46] = 0;
    //mSec D0 => D3 Zeroed
    //Carry over to mSec D1
    if (pV[44] && !pV[43] && !pV[42] && pV[41]) { // 0b1001 = 9
      pV[44] = 0;
      pV[43] = 0;
      pV[42] = 0;
      pV[41] = 0;
  
      // Carry over to mSec D2
      if (pV[39] && !pV[38] && !pV[37] && pV[36]) {  // 0b1001 = 9
        pV[39] = 0;
        pV[38] = 0;
        pV[37] = 0;
        pV[36] = 0;

      // Carry over to Sec D0
      if (pV[34] && !pV[33] && !pV[32] && pV[31]) {  // 0b1001 = 9
        pV[34] = 0;
        pV[33] = 0;
        pV[32] = 0;
        pV[31] = 0;

        // Carry over to Sec D1
        if (!pV[29] && pV[28] && !pV[27] && pV[26]) {  // 0b0101 = 5
          pV[29] = 0;
          pV[28] = 0;
          pV[27] = 0;
          pV[26] = 0;

          // Carry over to Min D0
          if (pV[24] && !pV[23] && !pV[22] && pV[21]) {  // 0b1001 = 9
            pV[24] = 0;
            pV[23] = 0;
            pV[22] = 0;
            pV[21] = 0;

            // Carry over to Min D1
            if (!pV[19] && pV[18] && !pV[17] && pV[16]) {  // 0b0101 = 5
              pV[19] = 0;
              pV[18] = 0;
              pV[17] = 0;
              pV[16] = 0;

              // Carry over to Hour D0
              if (pV[14] && !pV[13] && !pV[12] && pV[11]) {   // 0b1001 = 9
                pV[14] = 0;
                pV[13] = 0;
                pV[12] = 0;
                pV[11] = 0;
              }
              // Carry over to Min D1
              else if (!pV[11]) {
                pV[11]++ ;
              }
              else{
                if (!pV[12] && pV[11]) {
                  pV[12]++ ;
                  pV[11] = 0 ;
                }
                else{
                  if (!pV[13] && pV[12] && pV[11]) {
                    pV[13]++ ;
                    pV[12] = 0 ;
                    pV[11] = 0 ;
                  }
                  else{
                    if (!pV[14] && pV[13] && pV[12] && pV[11]) {
                      pV[14]++ ;
                      pV[13] = 0 ;
                      pV[12] = 0 ;
                      pV[11] = 0 ;
                    }
                  }
                }
              }
            }
            // Carry over to Min D1
            else if (!pV[16]) {
              pV[16]++ ;
            }
            else{
              if (!pV[17] && pV[16]) {
                 pV[17]++ ;
                pV[16] = 0 ;
              }
              else{
                if (!pV[18] && pV[17] && pV[16]) {
                  pV[18]++ ;
                  pV[17] = 0 ;
                  pV[16] = 0 ;
                }
                else{
                  if (!pV[19] && pV[18] && pV[17] && pV[16]) {
                    pV[19]++ ;
                    pV[18] = 0 ;
                    pV[17] = 0 ;
                    pV[16] = 0 ;
                  }
                }
              }
            }
          }
          // Carry over to Min D0
          else if (!pV[21]) {
            pV[21]++ ;
          }
          else{
            if (!pV[22] && pV[21]) {
              pV[22]++ ;
              pV[21] = 0 ;
            }
            else{
              if (!pV[23] && pV[22] && pV[21]) {
                pV[23]++ ;
                pV[22] = 0 ;
                pV[21] = 0 ;
              }
              else{
                if (!pV[24] && pV[23] && pV[22] && pV[21]) {
                  pV[24]++ ;
                  pV[23] = 0 ;
                  pV[22] = 0 ;
                  pV[21] = 0 ;
                }
              }
            }
          }
        }
      // Carry over to Sec D1
        else if (!pV[26]) {
          pV[26]++ ;
        }
        else{
          if (!pV[27] && pV[26]) {
            pV[27]++ ;
            pV[26] = 0 ;
          }
          else{
            if (!pV[28] && pV[27] && pV[26]) {
              pV[28]++ ;
              pV[27] = 0 ;
              pV[26] = 0 ;
            }
            else{
              if (!pV[29] && pV[28] && pV[27] && pV[26]) {
                pV[29]++ ;
                pV[28] = 0 ;
                pV[27] = 0 ;
                pV[26] = 0 ;
              }
            }
          }
        }
      }
      // Carry over to Sec D0
        else if (!pV[31]) {
          pV[31]++ ;
        }
        else{
          if (!pV[32] && pV[31]) {
            pV[32]++ ;
            pV[31] = 0 ;
          }
          else{
            if (!pV[33] && pV[32] && pV[31]) {
              pV[33]++ ;
              pV[32] = 0 ;
              pV[31] = 0 ;
            }
            else{
              if (!pV[34] && pV[33] && pV[32] && pV[31]) {
                pV[34]++ ;
                pV[33] = 0 ;
                pV[32] = 0 ;
                pV[31] = 0 ;
              }
            }
          }
        }
      }
      // Carry over to mSec D2
      else if (!pV[36]) {
        pV[36]++ ;
      }
      else{
        if (!pV[37] && pV[36]) {
          pV[37]++ ;
          pV[36] = 0 ;
        }
        else{
          if (!pV[38] && pV[37] && pV[36]) {
            pV[38]++ ;
            pV[37] = 0 ;
            pV[36] = 0 ;
          }
          else{
            if (!pV[39] && pV[38] && pV[37] && pV[36]) {
              pV[39]++ ;
              pV[38] = 0 ;
              pV[37] = 0 ;
              pV[36] = 0 ;
            }
          }
        }
      }
    }
    else if (!pV[41]) {
      pV[41]++ ;
    }
    else{
      if (!pV[42] && pV[41]) {
        pV[42]++ ;
        pV[41] = 0 ;
      }
      else{
        if (!pV[43] && pV[42] && pV[41]) {
          pV[43]++ ;
          pV[42] = 0 ;
          pV[41] = 0 ;
        }
        else{
          if (!pV[44] && pV[43] && pV[42] && pV[41]) {
            pV[44]++ ;
            pV[43] = 0 ;
            pV[42] = 0 ;
            pV[41] = 0 ;
          }
        }
      }
    }
  }
  else if (!pV[46]) {
    pV[46]++ ;
  }
  else{
    if (!pV[47] && pV[46]) {
      pV[47]++ ;
      pV[46] = 0 ;
    }
    else{
      if (!pV[48] && pV[47] && pV[46]) {
        pV[48]++ ;
        pV[47] = 0 ;
        pV[46] = 0 ;
      }
      else{
        if (!pV[49] && pV[48] && pV[47] && pV[46]) {
          pV[49]++ ;
          pV[48] = 0 ;
          pV[47] = 0 ;
          pV[46] = 0 ;
        }
      }
    }
  }
} // End Positive Count
     
else if (HmS >= 4 && CNTPOL == NEG && HOLD == NOT_HOLDING) {
  HmS = 0;
  if (!pV[49] && !pV[48] && !pV[47] && !pV[46]) {  // 0b1001 = 9
     //Bits Maxed, cycle back to Zero
    pV[49] = 1;
    pV[48] = 0;
    pV[47] = 0;
    pV[46] = 1;
    //mSec D0 => D3 Zeroed
    //Carry over to mSec D1
    if (!pV[44] && !pV[43] && !pV[42] && !pV[41]) { // 0b1001 = 9
      pV[44] = 1;
      pV[43] = 0;
      pV[42] = 0;
      pV[41] = 1;
  
      // Carry over to mSec D2
      if (!pV[39] && !pV[38] && !pV[37] && !pV[36]) {  // 0b1001 = 9
        pV[39] = 1;
        pV[38] = 0;
        pV[37] = 0;
        pV[36] = 1;

      // Carry over to Sec D0
      if (!pV[34] && !pV[33] && !pV[32] && !pV[31]) {  // 0b1001 = 9
        pV[34] = 1;
        pV[33] = 0;
        pV[32] = 0;
        pV[31] = 1;

        // Carry over to Sec D1
        if (!pV[29] && !pV[28] && !pV[27] && !pV[26]) {  // 0b0101 = 5
          pV[29] = 0;
          pV[28] = 1;
          pV[27] = 0;
          pV[26] = 1;

          // Carry over to Min D0
          if (!pV[24] && !pV[23] && !pV[22] && !pV[21]) {  // 0b1001 = 9
            pV[24] = 1;
            pV[23] = 0;
            pV[22] = 0;
            pV[21] = 1;

            // Carry over to Min D1
            if (!pV[19] && !pV[18] && !pV[17] && !pV[16]) {  // 0b0101 = 5
              pV[19] = 0;
              pV[18] = 1;
              pV[17] = 0;
              pV[16] = 1;

              // Carry over to Hour D0
              if (!pV[14] && !pV[13] && !pV[12] && !pV[11]) {   // 0b1001 = 9
                pV[14] = 1;
                pV[13] = 0;
                pV[12] = 0;
                pV[11] = 1;
              }
              // Carry over to Min D1
              else if (pV[11]) {
                pV[11]-- ;
              }
              else{
                if (pV[12] && !pV[11]) {
                  pV[12]-- ;
                  pV[11] = 1 ;
                }
                else{
                  if (pV[13] && !pV[12] && !pV[11]) {
                    pV[13]-- ;
                    pV[12] = 1 ;
                    pV[11] = 1 ;
                  }
                  else{
                    if (pV[14] && !pV[13] && !pV[12] && !pV[11]) {
                      pV[14]-- ;
                      pV[13] = 1 ;
                      pV[12] = 1 ;
                      pV[11] = 1 ;
                    }
                  }
                }
              }
            }
            // Carry over to Min D1
            else if (pV[16]) {
              pV[16]-- ;
            }
            else{
              if (pV[17] && !pV[16]) {
                 pV[17]-- ;
                pV[16] = 1 ;
              }
              else{
                if (pV[18] && !pV[17] && !pV[16]) {
                  pV[18]-- ;
                  pV[17] = 1 ;
                  pV[16] = 1 ;
                }
                else{
                  if (pV[19] && !pV[18] && !pV[17] && !pV[16]) {
                    pV[19]-- ;
                    pV[18] = 1 ;
                    pV[17] = 1 ;
                    pV[16] = 1 ;
                  }
                }
              }
            }
          }
          // Carry over to Min D0
          else if (pV[21]) {
            pV[21]-- ;
          }
          else{
            if (pV[22] && !pV[21]) {
              pV[22]-- ;
              pV[21] = 1 ;
            }
            else{
              if (pV[23] && !pV[22] && !pV[21]) {
                pV[23]-- ;
                pV[22] = 1 ;
                pV[21] = 1 ;
              }
              else{
                if (pV[24] && !pV[23] && !pV[22] && !pV[21]) {
                  pV[24]-- ;
                  pV[23] = 1 ;
                  pV[22] = 1 ;
                  pV[21] = 1 ;
                }
              }
            }
          }
        }
      // Carry over to Sec D1
        else if (pV[26]) {
          pV[26]-- ;
        }
        else{
          if (pV[27] && !pV[26]) {
            pV[27]-- ;
            pV[26] = 1 ;
          }
          else{
            if (pV[28] && !pV[27] && !pV[26]) {
              pV[28]-- ;
              pV[27] = 1 ;
              pV[26] = 1 ;
            }
            else{
              if (pV[29] && !pV[28] && !pV[27] && !pV[26]) {
                pV[29]-- ;
                pV[28] = 0 ;
                pV[27] = 0 ;
                pV[26] = 0 ;
              }
            }
          }
        }
      }
      // Carry over to Sec D0
        else if (pV[31]) {
          pV[31]-- ;
        }
        else{
          if (pV[32] && !pV[31]) {
            pV[32]-- ;
            pV[31] = 1 ;
          }
          else{
            if (pV[33] && !pV[32] && !pV[31]) {
              pV[33]-- ;
              pV[32] = 1 ;
              pV[31] = 1 ;
            }
            else{
              if (pV[34] && !pV[33] && !pV[32] && !pV[31]) {
                pV[34]-- ;
                pV[33] = 1 ;
                pV[32] = 1 ;
                pV[31] = 1 ;
              }
            }
          }
        }
      }
      // Carry over to mSec D2
      else if (pV[36]) {
        pV[36]-- ;
      }
      else{
        if (pV[37] && !pV[36]) {
          pV[37]-- ;
          pV[36] = 1 ;
        }
        else{
          if (pV[38] && !pV[37] && !pV[36]) {
            pV[38]-- ;
            pV[37] = 1 ;
            pV[36] = 1 ;
          }
          else{
            if (pV[39] && !pV[38] && !pV[37] && !pV[36]) {
              pV[39]-- ;
              pV[38] = 1 ;
              pV[37] = 1 ;
              pV[36] = 1 ;
            }
          }
        }
      }
    }
    else if (pV[41]) {
      pV[41]-- ;
    }
    else{
      if (pV[42] && !pV[41]) {
        pV[42]-- ;
        pV[41] = 1 ;
      }
      else{
        if (pV[43] && !pV[42] && !pV[41]) {
          pV[43]-- ;
          pV[42] = 1 ;
          pV[41] = 1 ;
        }
        else{
          if (pV[44] && !pV[43] && !pV[42] && !pV[41]) {
            pV[44]-- ;
            pV[43] = 1 ;
            pV[42] = 1 ;
            pV[41] = 1 ;
          }
        }
      }
    }
  }
  else if (pV[46]) {
    pV[46]-- ;
  }
  else{
    if (pV[47] && !pV[46]) {
      pV[47]-- ;
      pV[46] = 1 ;
    }
    else{
      if (pV[48] && !pV[47] && !pV[46]) {
        pV[48]-- ;
        pV[47] = 1 ;
        pV[46] = 1 ;
      }
      else{
        if (pV[49] && !pV[48] && !pV[47] && !pV[46]) {
          pV[49]-- ;
          pV[48] = 1 ;
          pV[47] = 1 ;
          pV[46] = 1 ;
        }
      }
    }
  }
} // End Negative Count Routine


else{

}

// Fix the Atari Football effect
if (pV[49] || pV[48] || pV[47] || pV[46] && !atvgtz) {
	atvgtz = 1;
}
else if (!pV[11] && !pV[12] && !pV[13] && !pV[14] && !pV[16] && !pV[17] && !pV[18] && !pV[19] && !pV[21] && !pV[22] && !pV[23] && !pV[24] && !pV[26] && !pV[27] && !pV[28] && !pV[29] && !pV[31] && !pV[32] && !pV[33] && !pV[34] && !pV[36] && !pV[37] && !pV[38] && !pV[39] && !pV[41] && !pV[42] && !pV[43] && !pV[44] && !pV[46] && !pV[47] && !pV[48] && !pV[49] && atvgtz) {
	atvgtz = 0;
	if (pV[9] == NEG && CNTPOL == NEG) {
		pV[9] = POS;
		CNTPOL = POS;
	}
	else if (pV[9] == POS && CNTPOL == NEG) {
                pV[9] = NEG;
                CNTPOL = POS;
        }

}
}
}  // Main Loop


