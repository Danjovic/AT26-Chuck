///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///                                                                                                         ///
///     I2C Library for PIC - Based on code published on Wikipedia                                          ///
///                                                                                                         ///
///     Daniel Jose Viana, October 2012 - danjovic@hotmail.com                                              ///
///                                                                                                         ///
///     This code is licensed under GPL V2.0                                                                ///
///                                                                                                         ///
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
int1 read_SCL(void);  // Set SCL as input and return current level of line, 0 or 1
int1 read_SDA(void);  // Set SDA as input and return current level of line, 0 or 1
void clear_SCL(void); // Actively drive SCL signal low
void clear_SDA(void); // Actively drive SDA signal low
void arbitration_lost(void);
int1 i2c_write_byte(int1 send_start,int1 send_stop,unsigned char byte_);
int8 i2c_read_byte(int1 nack, int1 send_stop);
*/
int1 started = false; // global data
int1 have_arbitration=true;

 
//
//  I2C Routines
//
void i2c_start_cond(void) {
  if (started) { // if started, do a restart cond
    // set SDA to 1
    read_SDA();
    //I2C_delay();
    while (read_SCL() == 0) {   // Clock stretching
      // You should add timeout to this loop
    }
    // Repeated start setup time, minimum 4.7us
    //I2C_delay();
  }
  if (read_SDA() == 0) {
    arbitration_lost();
  }
  // SCL is high, set SDA from 1 to 0.
  clear_SDA();
  //I2C_delay();
  clear_SCL();
  started = true;
}
 
void i2c_stop_cond(void){
  // set SDA to 0
  clear_SDA();
  //I2C_delay();
  // Clock stretching
  while (read_SCL() == 0) {
    // You should add timeout to this loop.
  }
  // Stop bit setup time, minimum 4us
  //I2C_delay();
  // SCL is high, set SDA from 0 to 1
  if (read_SDA() == 0) {
    arbitration_lost();
  }
  //I2C_delay();
  started = false;
}
 
// Write a bit to I2C bus
void i2c_write_bit(int1 bit) {
  if (bit) {
    read_SDA();
  } else {
    clear_SDA();
  }
  //I2C_delay();
  while (read_SCL() == 0) {   // Clock stretching
    // You should add timeout to this loop
  }
  // SCL is high, now data is valid
  // If SDA is high, check that nobody else is driving SDA
  if (bit && read_SDA() == 0) {
    arbitration_lost();
  }
  //I2C_delay();
  clear_SCL();
}
 
// Read a bit from I2C bus
int1 i2c_read_bit(void) {
  int1 bit;
  // Let the slave drive data
  read_SDA();
  //I2C_delay();
  while (read_SCL() == 0) {   // Clock stretching
    // You should add timeout to this loop
  }
  // SCL is high, now data is valid
  bit = read_SDA();
  //I2C_delay();
  clear_SCL();
  return bit;
}
 
// Write a byte to I2C bus. Return 0 if ack by the slave.
int1 i2c_write_byte(int1 send_start,
                    int1 send_stop,
                    unsigned char byte_) {
  unsigned bit;
  int1 nack;
  if (send_start) {
    i2c_start_cond();
  }
  for (bit = 0; bit < 8; bit++) {
    i2c_write_bit((byte_ & 0x80) != 0);
    byte_ <<= 1;
  }
  nack = i2c_read_bit();
  if (send_stop) {
    i2c_stop_cond();
  }
  return nack;
}
 
// Read a byte from I2C bus
unsigned char i2c_read_byte(int1 nack, int1 send_stop) {
  unsigned char byte_ = 0;
  unsigned bits;
  for (bits = 0; bits < 8; bits++) {
    byte_ = (byte_ << 1) | i2c_read_bit();
  }
  i2c_write_bit(nack);
  if (send_stop) {
    i2c_stop_cond();
  }
  return byte_;
}

int1 read_SCL(void)  // Set SCL as input and return current level of line, 0 or 1
{
  return input(SCL_PIN); 
}
int1 read_SDA(void)  // Set SDA as input and return current level of line, 0 or 1
{
  return input(SDA_PIN); 
}
void clear_SCL(void) // Actively drive SCL signal low
{
	output_low(SCL_PIN);
}
void clear_SDA(void) // Actively drive SDA signal low
{
	output_low(SDA_PIN);
}
void arbitration_lost(void) {  // 
	have_arbitration=false;
}
