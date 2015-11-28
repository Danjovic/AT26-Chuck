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

//#define TKCHUCK
#define SQUARE


#rom  0x2100 = {0x13,0x78,0x78}
#ifdef TKCHUCK             // Definitions for SQUARE INCH Board
#rom  0x2108 = {"sn:0000 Prototipo"}
#endif

#ifdef SQUARE             // Definitions for SQUARE INCH Board
#rom  0x2108 = {"AT26-Chuck"}
#endif



///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///                                                                                                         ///
///                                    DEFINIÇÃO DAS CONSTANTES                                             ///
///                                                                                                         ///
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef SQUARE             // Definitions for SQUARE INCH Board

#define BUTTON1  pin_a0
#define LEFT     pin_a1
#define RIGHT    pin_a2
#define BUTTON3  pin_a3
#define DOWN     pin_a4
#define UP       pin_a5

#define BUTTON2  pin_c0
#define P_SDA    pin_c1
#define P_SCL    pin_c2
#define LED      pin_c3
#define p_TXD    pin_c4
#define p_RXD    pin_c5

#endif


#ifdef TKCHUCK             // Definitions for TK Chuck Board

#define BUTTON2  pin_a0
#define RIGHT    pin_a1
#define LEFT     pin_a2
#define BUTTON3  pin_a3
#define DOWN     pin_a4
#define BUTTON1  pin_a5

#define UP       pin_c0
#define VPLUS33  pin_c1
#define P_SDA    pin_c2
#define LED      pin_c3
#define p_SCL    pin_c4
#define VMINUS   pin_c5

#endif 

#include "myI2C.h"

#define bit_b1c    0         // Bits do registro de configuração de operação
#define bit_b1z    1 
#define bit_b2c    2
#define bit_b2z    3
#define bit_afbc   4
#define bit_afbz   5
#define bit_useax  6
#define bit_useay  7

#define bit_Right  0         // Bits de opção de inicialização       
#define bit_Left   1
#define bit_Up     2  
#define bit_Down   3
#define bit_Z      4
#define bit_C      5

#define treshold 32          // Limiar em torno do centro da manete analógica
#define upper 128+treshold   // Limite superior
#define lower 128-treshold   // Limite inferior
#define tresholdA 64         // Limiar em torno do centro dos acelerômetros
#define MAX_TRIES 4           // Tentativas de leituras iguais na inicialização.

#define MSG_ERRINIT 0x08
#define MSG_CONFIG  0x3f
                          // Endereços na EEPROM 
#define config_address 0  // Configuração do adaptador
#define calibX_address 1  // Calibração do acelerometro X (MSB)
#define calibY_address 2  // Calibração do acelerometro X (MSB)

//#rom  0x2100={0x09,0x80,0x80}   // Inicializa endereços na eeprom


///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///                                                                                                         ///
///                                       PROTOTIPO DAS FUNCOES                                             ///
///                                                                                                         ///
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

//  Desativa as saídas, colocando-as em alta impedância 
void desativa_saidas(void);

//  Inicializa Nunchuck e retorna estado Z=não respondeu 
int1 init_Nunchuck(void);

//  Faz leitura do Nunchuck e retorna estado Z=não respondeu 
int1 read_Nunchuck();

//  Pisca o LED para indicar uma mensagem
void pisca_msg(char codigo);

//  Configura nunchuck conforme posição da manete analogica e botoes durante o startup
void configura_nunchuck(char opcoes);



///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///                                                                                                         ///
///                                       INICIO DO PROGRAMA                                                ///
///                                                                                                         ///
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

//
// Definição das Variáveis
//
char config;         // Configuração do adaptador
char opcao_startup;  // Opcao de inicializacao
char temp;           // variavel temporaria
char tries;          // quantidade de tentativas de leitura na inicialização

int1 B1C,            // Usa ou não Botão 1 conforme estado do botão C
     B1Z,            // Usa ou não Botão 1 conforme estado do botão Z
     B2C,            // Usa ou não Botão 2 conforme estado do botão C
     B2Z,            // Usa ou não Botão 2 conforme estado do botão Z
     AFBC,           // Auto Fire Botao C
     AFBZ,           // Auto Fire Botao Z
     USEAX,          // Opção de uso do acelerômetro X
     USEAY;          // Opção de uso do acelerômetro Y

int1 MODAF;          // Modulador do Auto Fire;
 
char X,Y;            // Eixos X e Y da manete mais .
int1 Z,C;            // Botões 

char sX,sY;          // Salva valor das manetes
//int1 sZ,sC;          // e dos botoes para check no startup

int16 AX,AY,AZ;      // Eixos X, Y e Z dos acelerômtros

int16 calibrAX,       // Valor de calibração do acelerometro X
      calibrAY;       // Valor de calibração do acelerometro Y

char AFModCont=0; // Contador do modulador do auto fire.

void main (void) {
  // init hardware
  setup_oscillator( OSC_8MHZ );
  setup_comparator(NC_NC_NC_NC);
  desativa_saidas();
  output_high(LED);
  delay_ms(100);  // espera alimentação estabilizar  
  setup_wdt(WDT_2304MS);

  // 
  // Loop principal
  //
  for (;;) {
    // Testa presença do nunchuck
    if (init_Nunchuck()) {
       delay_ms(50);
       if (read_nunchuck()) {
          // Checa opção de startup
          read_nunchuck();  
          sX=X; sY=Y;
          tries=MAX_TRIES;
          temp=20;

	      while (temp>0 && tries>0){
	           read_nunchuck();
	           if (X==sX && Y==sY)
	               tries--;
	           else {
		           sX=X; sY=Y;
		           tries=MAX_TRIES;
	           }
	           delay_ms(1);
	           temp--;	           
          }
          if (temp==0) break; // 

                            
          opcao_startup=0;
          if (Z)       opcao_startup|=(1<<bit_Z);    // Botao Z
          if (C)       opcao_startup|=(1<<bit_C);    // Botao C
          if (X>upper) opcao_startup|=(1<<bit_Right);// Right
          if (X<lower) opcao_startup|=(1<<bit_Left); // Left
          if (Y>upper) opcao_startup|=(1<<bit_Up);   // Up
          if (Y<lower) opcao_startup|=(1<<bit_Down); // Down
          if (opcao_startup)             // caso manete ou botoes não estejam em repouso na inicialização
             configura_nunchuck(opcao_startup);  // Checa opção escolhida e cria configuração de operação

          // Lê da EEPROM o modo de operação
          config=READ_EEPROM (config_address);  // Lê configuração da EEPROM
           
          if (config & (1<<bit_b1c)) B1C  =1; else B1C=0;
          if (config & (1<<bit_b1z)) B1Z  =1; else B1Z=0;
          if (config & (1<<bit_b2c)) B2C  =1; else B2C=0;
          if (config & (1<<bit_b2z)) B2Z  =1; else B2Z=0;
          if (config & (1<<bit_afbc)) AFBC  =1; else AFBC=0;
          if (config & (1<<bit_afbz)) AFBZ  =1; else AFBZ=0;
          if (config & (1<<bit_useax)) USEAX  =1; else USEAX=0;
          if (config & (1<<bit_useay)) USEAY  =1; else USEAY=0;

          // Lê da EEPROM os vetores de calibração dos acelerometros
          temp=READ_EEPROM (calibX_address);  // Calibração do Eixo X
          calibrAX=(int16)temp<<2;

          temp=READ_EEPROM (calibY_address);  // Calibração do Eixo Y
          calibrAY=(int16)temp<<2;

       } else break;  


         //                                                                                                  //
         //                Tarefa principal. Lê nunchuck, atualiza saídas conforme leitura                   //
         //           Se houver algum erro na leitura, sai e tenta inicializar novamente o nunchuck          //
         //                                                                                                  //
         while (read_nunchuck()) { 

            // Aguarda período de amostragem
            //
            restart_wdt();
            delay_ms(10); // while (!GO);
            // processa Auto Fire
            //
            AFModCont++;
            AFModCont&=3; // contador com modulo 4
            if (AFModCont==0) 
              MODAF=!MODAF;   
         

            // Atualisa saidas
            //
            // Botao 1
            if ( (Z&&B1Z&&(!AFBZ||MODAF))||(C&&B1C&&(!AFBC||MODAF)) ) output_low(BUTTON1);
              else output_float(BUTTON1);
            // Botao 2
            if ( (Z&&B2Z&&(!AFBZ||MODAF))||(C&&B2C&&(!AFBC||MODAF)) ) output_low(BUTTON2);
              else output_float(BUTTON2);
            // RIGHT
            if ( (X>upper) || ((AX>(calibrAX+tresholdA)) && USEAX)  ) output_low(RIGHT);
              else output_float(RIGHT);
            // LEFT
            if ( (X<lower) || ((AX<(calibrAX-tresholdA)) && USEAX)  ) output_low(LEFT);
              else output_float(LEFT);
            // UP
            if ( (Y>upper) || ((AY>(calibrAY+tresholdA)) && USEAY)  ) output_low(UP);
              else output_float(UP);
            // DOWN
            if ( (Y<lower) || ((AY<(calibrAY-tresholdA)) && USEAY)  ) output_low(DOWN);
              else output_float(DOWN);

           }  // while 
         //break;   // se alcançou aqui, houve um erro de comunicação.
         
      } 

    // Se alcançou este ponto, indica erro e tenta novamente após 1 segundo
    pisca_msg(MSG_ERRINIT);
    delay_ms(1000);

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
//  INIT NUNCHUCK - Testa se dispositivo responde no barramento I2C
//
//  Zero=Erro  NZ = Inicializou OK
int1 init_Nunchuck(void) {

  i2c_write_byte(true,false,0xa4);
  i2c_write_byte(false,false,0xf0);
  if (i2c_write_byte(false,true,0x55)) 
     return false; 
     
  i2c_write_byte(true,false,0xa4);
  i2c_write_byte(false,false,0xfb);
  if (i2c_write_byte(false,true,0x00)) 
     return false; 
  return true;
}



//
//  Faz leitura do Nunchuck e retorna estado. False=não respondeu 
//
int1 read_Nunchuck(){

  int8 temp;
  
  // Send request
  i2c_write_byte(true,false,0xa4);  
  if (i2c_write_byte(false,true,0x00))  // if not acked, return false
     return false; 
  delay_ms(1);           // consider removing this

  // receive report 
  i2c_write_byte(true,false,0xa5);  // send read request. maybe check an ack here
  
  X  = i2c_read_byte(false, false);       // 1st byte: X axis
  Y  = i2c_read_byte(false, false);       // 2nd byte: Y axis
  AX = ((int16)i2c_read_byte(false, false)<<2);  // 3rd byte: Accelerometer X, msbits
  AY = ((int16)i2c_read_byte(false, false)<<2);  // 4th byte: Accelerometer Y, msbits
  AZ = ((int16)i2c_read_byte(false, false)<<2);  // 5th byte: Accelerometer Z, msbits
  temp      =  i2c_read_byte(true, true);        // 6th byte: Accelerometers lsbits, buttons C,Z

  AX+=((temp>>2) & 0x03);  // insert LSBits from accelerometers
  AY+=((temp>>4) & 0x03);
  AZ+=((temp>>6) & 0x03);

  C=( (temp&0x02)==0);
  Z=( (temp&0x01)==0);
  
  return true;
} //read_nunchuck


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



//
//   Configura nunchuck conforme posição da manete analogica e botoes durante o startup
//
void configura_nunchuck(char opcoes) {
   char conf,a;
   
   conf=READ_EEPROM(config_address);
   switch (opcoes) { // opcoes dos botoes 

     // C + neutro  ->  B2=C, Z=B1       
     case (1<<bit_C):          
                     conf&=~((1<<bit_b1c)|                          (1<<bit_b2z )|(1<<bit_afbc)|(1<<bit_afbz));    
                     conf|= (             (1<<bit_b1z)|(1<<bit_b2c)                                          );                    
                     break;

     // Z + neutro  ->  C=B1, Z=B1
     case (1<<bit_Z):     
                     conf&=~(                          (1<<bit_b2c)|(1<<bit_b2z )|(1<<bit_afbc)|(1<<bit_afbz));    
                     conf|= ((1<<bit_b1c)|(1<<bit_b1z)                                                       ); 
                     break;

     // C + up  ->  C=B2+AF, Z=B1  
     case ((1<<bit_C)+(1<<bit_Up)):       
                     conf&=~((1<<bit_b1c)|                          (1<<bit_b2z )|              (1<<bit_afbz));    
                     conf|= (             (1<<bit_b1z)|(1<<bit_b2c)|              (1<<bit_afbc)              ); 
                     break;

     // Z + up  ->  C=B2+AF, Z=B1+AF  
     case ((1<<bit_Z)+(1<<bit_Up)):       
                     conf&=~((1<<bit_b1c)|                          (1<<bit_b2z )                            );    
                     conf|= (             (1<<bit_b1z)|(1<<bit_b2c)|              (1<<bit_afbc)|(1<<bit_afbz)); 
                     break;

    // C + down  ->  C=B2, Z=B1+AF 
    case ((1<<bit_C)+(1<<bit_Down)):      
                     conf&=~((1<<bit_b1c)|                          (1<<bit_b2z )|(1<<bit_afbc)              );    
                     conf|= (             (1<<bit_b1z)|(1<<bit_b2c)|                            (1<<bit_afbz)); 
                     break;

     // Z + down  ->  C=B1+AF, Z=B1 
     case ((1<<bit_Z)+(1<<bit_Down)):      
                     conf&=~(                          (1<<bit_b2c)|(1<<bit_b2z )|              (1<<bit_afbz));    
                     conf|= ((1<<bit_b1c)|(1<<bit_b1z)|                           (1<<bit_afbc)              ); 
                     break;


     // opcoes dos acelerometros

     // up  ->  Use AY
     case (1<<bit_Up):               
                     conf&=~((1<<bit_useax)               );    
                     conf|= (               (1<<bit_useay));
                     a=(int8)(AY>>2);                 // MSBits do valor do acelerômetro lido na inicialização
                     WRITE_EEPROM(calibY_address,a);  // Armazena na EEPROM
                     break;

     // down  ->  desativa ação dos acelerometros
     case (1<<bit_Down):            
                     conf&=~((1<<bit_useax)|(1<<bit_useay));    
                   //conf|= ((1<<bit_useax)|(1<<bit_useax));
                     break;

     // left  ->  Usa acelerometro no eixo X
     case (1<<bit_Left):            
                     conf&=~(               (1<<bit_useay));    
                     conf|= ((1<<bit_useax)               );
                     a=(int8)(AX>>2);                 // MSBits do valor do acelerômetro lido na inicialização
                     WRITE_EEPROM(calibX_address,a);  // Armazena na EEPROM
                     break;

     // right  ->  Usa acelerometro nos eixos X e Y
     case (1<<bit_Right):           
                   //conf&=~((1<<bit_useax)|(1<<bit_useay));    
                     conf|= ((1<<bit_useax)|(1<<bit_useay));
                     a=(int8)(AY>>2);                 // MSBits do valor do acelerômetro lido na inicialização
                     WRITE_EEPROM(calibY_address,a);  // Armazena na EEPROM
                     a=(int8)(AX>>2);                 // MSBits do valor do acelerômetro lido na inicialização
                     WRITE_EEPROM(calibX_address,a);  // Armazena na EEPROM
                     break;
   
   }  // switch  
   WRITE_EEPROM(config_address,conf);  // Atualiza opções na EEPROM

   pisca_msg(MSG_CONFIG);

} // configura_nunchuck