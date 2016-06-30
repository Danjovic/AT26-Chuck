///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///                                                                                                         ///
///                      Adaptador de Wii Nunchuck para joystick padrão ATARI                               ///
///                v1.01 25 de Outubro de 2012  - Daniel José Viana - danjovic@hotmail.com                  ///
///                - Basic Release                                                                          ///
///                                                                                                         ///
///                v1.1  12 de Fevereiro de 2013 - Daniel José Viana - danjovic@hotmail.com                 ///
///                - Changed I2C routines to support clock stretching                                       ///
///                - Simplified Nunhcuck detection. Works with oritginal, clone and wireless nunchucks      ///
///                                                                                                         ///
///                v1.11 16 de Fevereiro de 2013 - Daniel José Viana - danjovic@hotmail.com                 ///
///                - Changed blink engine and blink sequences for error and programming                     ///
///                                                                                                         ///
///                v1.12 25 de Maio de 2013 - Daniel José Viana - danjovic@hotmail.com                      ///
///                - Enhanced Initialization routine to detect initial position of the stick and buttons    ///
///                - Added Watchdog timer                                                                   ///
///                                                                                                         ///
///                v1.13 04 de Outubro de 2015 - Daniel José Viana - danjovic@hotmail.com                   ///
///                - Included conditional definitions for SQUARE INCH and TKCHUCK boards                    ///
///                - Code licence changed to GPL V2.0                                                       ///
///                                                                                                         ///
///                                                                                                         ///
///////////////////////////////////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///                                                                                                         ///
///                                    CONFIGURAÇÃO DO CHIP                                                 ///
///                                                                                                         ///
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <16f688.h>
#use delay(clock=8000000)
#fuses INTRC_IO, NOPROTECT, NOBROWNOUT, NOMCLR, WDT, PUT, 

#rom  0x2108 = {"SNES to Atari2600"}




///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///                                                                                                         ///
///                                    DEFINIÇÃO DAS CONSTANTES                                             ///
///                                                                                                         ///
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define BUTTON1  pin_a0
#define LEFT     pin_a1
#define RIGHT    pin_a2
#define BUTTON3  pin_a3  // Not used in Atari 2600
#define DOWN     pin_a4
#define UP       pin_a5

#define BUTTON2  pin_c0  // Not used in Atari 2600
#define datapin  pin_c1  // Data IN line
#define clockPin pin_c2  // Clock OUT line
#define LED      pin_c3
#define p_TXD    pin_c4  
#define latchPin pin_c5  // Latch line



#define uint16_t unsigned int16
#define uint8_t unsigned char
#define digitalWrite output_bit
#define digitalRead  input

#define HIGH 1
#define LOW 0

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///                                                                                                         ///
///                                       PROTOTIPO DAS FUNCOES                                             ///
///                                                                                                         ///
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

//  Desativa as saídas, colocando-as em alta impedância 
void desativa_saidas(void);

//  Faz leitura do controle do SNES 
void read_SNES();

//  Pisca o LED para indicar uma mensagem
void pisca_msg(char codigo);

inline void Pulse_clock(void);

// Get buttons state. 
uint16_t get_buttons(void);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///                                                                                                         ///
///                                       Definição das Variáveis                                           ///
///                                                                                                         ///
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

uint16_t Buttons_on_at_startup;
uint16_t Buttons;

int1 din;
//boolean pinState = false; 
uint8_t Buttons_low;
uint8_t Autofire_modulation,flip_flop=0;

//char buttons_mask;   // Mascara dos bits para autofire
//char HighByte ;       // B  Y  Sl St Up Dw Lf Rt  
//char LowByte ;        // A  X  L  R  1  1  1  1 


///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///                                                                                                         ///
///                                       INICIO DO PROGRAMA                                                ///
///                                                                                                         ///
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

void main (void) {

  //
  // init hardware
  //
  setup_oscillator( OSC_8MHZ );
  setup_comparator(NC_NC_NC_NC);
  desativa_saidas();
  output_high(LED);
  delay_ms(100);  // espera alimentação estabilizar  
  setup_wdt(WDT_2304MS);


  output_high(clockPin);       //pinMode(clockPin,OUTPUT);
  output_low(latchPin);        //pinMode(latchPin,OUTPUT);
  din=input(dataPin);              //pinMode(dataPin,INPUT);

  // Now sample buttons and initialize autofire mask
  Buttons_on_at_startup = get_buttons();
  Autofire_modulation = (uint8_t)(Buttons_on_at_startup & 0xff);
  Autofire_modulation &= 0x3f; // Isolate Start and Select
  Autofire_modulation = ~Autofire_modulation; // invert bits.
  flip_flop = 0;
	
  // 
  // Loop principal
  //
  for (;;) {
	//Grab the buttons state. At this time B value is already at Data output
	Buttons = get_buttons();

	// fill in buttons                             7   6   5   4   3   2   1   0  bit
	Buttons_low=(uint8_t)(Buttons & 0xff);    //   St  Sl  Tr  Tl  Y   B   A   X  button  
                 

	// Now modulate buttons with autofire 
	if (++flip_flop & (1<<1)) {  // do it once at each 2 runs of 50ms (100ms turn rate or 10Hz)
		Buttons_low &= Autofire_modulation; // Clear the bits set to autofire
	}
	

	// Now test for the directional buttons
	if (Buttons & (1<<11)) output_low(UP); else output_float(UP);  // Up
	if (Buttons & (1<<10)) output_low(DOWN); else output_float(DOWN); //  Down
	if (Buttons & (1<<9))  output_low(LEFT); else output_float(LEFT); //  Left
	if (Buttons & (1<<8))  output_low(RIGHT); else output_float(RIGHT); // Right

	if (Buttons_low !=0xff) output_low(BUTTON1); else output_float(BUTTON1); //Button

	delay_ms(25);

  } // for (loop principal)
}  // main



///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///                                                                                                         ///
///                                    IMPLEMENTACAO  DAS FUNCOES                                           ///
///                                                                                                         ///
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

//
//  Desativa as saídas, colocando-as em alta impedância 
//
void desativa_saidas(void) {
  output_float(UP);
  output_float(DOWN);
  output_float(LEFT);
  output_float(RIGHT);
  output_float(BUTTON1);
  output_float(BUTTON2);
  output_float(BUTTON3);
}




//
//  Pisca o LED para indicar uma mensagem
//
void pisca_msg(char codigo) {
//  int k;
  desativa_saidas();

  for (;codigo!=0x01;codigo>>=1) {
      output_low(LED); // acende led
      if (codigo & 1 ) {// Piscada rapida
         delay_ms(100);
         output_high(LED); // apaga led   
         delay_ms(200); 
      } else {
         delay_ms(500); // piscada lenta
         output_high(LED); // apaga led   
         delay_ms(150);         
      }   
  } // for

} //pisca_msg


// Pulse clock line. 
inline void Pulse_clock() {
	delay_us (6);
	digitalWrite(clockPin, HIGH);
	delay_us(6);
	digitalWrite(clockPin, LOW);
}

// Get buttons state. 
uint16_t get_buttons(void) {
    //   shift in bits from controller and return them using the following format
    //   11  10   9   8   7   6   5   4   3   2   1   0
    //   Up  Dw   Lf  Rt  St  Sl  Tr  Tl  Y   B   A   X
    //
	uint16_t buttons=0;

	//latch buttons state. After latching, button B state is ready at data output
	digitalWrite(latchPin, HIGH);
	delay_us(12);
	digitalWrite(latchPin, LOW);
	digitalWrite(clockPin,LOW);

	// read the buttons	and store their values
	if (!digitalRead(dataPin)) buttons |= (1<<2); // B
	Pulse_clock();
	
	if (!digitalRead(dataPin)) buttons |= (1<<3); // Y
	Pulse_clock();

	if (!digitalRead(dataPin)) buttons |= (1<<7); // Select 
	Pulse_clock();

	if (!digitalRead(dataPin)) buttons |= (1<<6); // Start 
	Pulse_clock();

	if (!digitalRead(dataPin)) buttons |= (1<<11); // Up 
	Pulse_clock();

	if (!digitalRead(dataPin)) buttons |= (1<<10); // Down 
	Pulse_clock();

	if (!digitalRead(dataPin)) buttons |= (1<<9); // Left 
	Pulse_clock();

	if (!digitalRead(dataPin)) buttons |= (1<<8); // Right 
	Pulse_clock();

	if (!digitalRead(dataPin)) buttons |= (1<<1); // A  
	Pulse_clock();

	if (!digitalRead(dataPin)) buttons |= (1<<0); // X 
	Pulse_clock();

	if (!digitalRead(dataPin)) buttons |= (1<<4); // Top Left 
	Pulse_clock();

	if (!digitalRead(dataPin)) buttons |= (1<<5); // Top Right 
	Pulse_clock();

	// Return Clock signal to idle level.
	digitalWrite(clockPin,HIGH);
	
	return buttons;
}	

