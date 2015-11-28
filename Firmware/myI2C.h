///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///                                                                                                         ///
///     I2C Library for PIC - Based on code published on Wikipedia                                          ///
///                                                                                                         ///
///     Daniel Jose Viana, October 2012 - danjovic@hotmail.com                                              ///
///                                                                                                         ///
///     This code is licensed under GPL V2.0                                                                ///
///                                                                                                         ///
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

int1 read_SCL(void);  // Set SCL as input and return current level of line, 0 or 1
int1 read_SDA(void);  // Set SDA as input and return current level of line, 0 or 1
void clear_SCL(void); // Actively drive SCL signal low
void clear_SDA(void); // Actively drive SDA signal low
void arbitration_lost(void);
int1 i2c_write_byte(int1 send_start,int1 send_stop,unsigned char byte_);
int8 i2c_read_byte(int1 nack, int1 send_stop);



#define SCL_PIN  P_SCL
#define SDA_PIN  P_SDA

#include "myI2C.c"