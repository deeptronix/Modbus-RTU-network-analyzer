// some of this code was written by <cstone@pobox.com> originally; 
// it is in the public domain.
/**
 *  Adafruit SSD1306 library modified by William Greiman for
 *  unbuffered LiquidCrystal character mode.
 *
 * -- Further modified by JBoyton to support character scaling
 * and to allow horizontal positioning of text to any pixel.
 * Vertical text position is still limited to a row.
 * Edited to make specific to Uno R3 and 128x64 size.
 * Also added H/W SPI and I2C support to the existing soft SPI.
 */
#include <avr/pgmspace.h>
#include "SSD1306_text.h"
#include "ssdfont.h"
//#include <SPI.h>
#include <Wire.h>

//------------------------------------------------------------------------------
void SSD1306_text::init() {
  col_ = 0;
  row_ = 0;
  textSize_ = 1;
  textSpacing_ = 1;

  // set pin directions
#if !I2C
  pinMode(dc_, OUTPUT);
  pinMode(cs_, OUTPUT);
  csport      = portOutputRegister(digitalPinToPort(cs_));
  cspinmask   = digitalPinToBitMask(cs_);
  dcport      = portOutputRegister(digitalPinToPort(dc_));
  dcpinmask   = digitalPinToBitMask(dc_);
#endif
  pinMode(rst_, OUTPUT);

#if I2C
    Wire.begin();
#else
#if  HW_SPI
  SPI.begin();
  SPI.setClockDivider(SPI_CLOCK_DIV2); // 8 MHz
#else // bit twiddle SPI
  pinMode(data_, OUTPUT);
  pinMode(clk_, OUTPUT);
  clkport     = portOutputRegister(digitalPinToPort(clk_));
  clkpinmask  = digitalPinToBitMask(clk_);
  mosiport    = portOutputRegister(digitalPinToPort(data_));
  mosipinmask = digitalPinToBitMask(data_);
#endif
#endif

  // Reset
  digitalWrite(rst_, HIGH);
  delay(1);
  digitalWrite(rst_, LOW);
  delay(10);
  digitalWrite(rst_, HIGH);

  // Init sequence for 128x64 OLED module
  sendCommand(SSD1306_DISPLAYOFF);          // 0xAE
  sendCommand(SSD1306_SETDISPLAYCLOCKDIV);  // 0xD5
  sendCommand(0x80);                        // the suggested ratio 0x80
  sendCommand(SSD1306_SETMULTIPLEX);        // 0xA8
  sendCommand(0x3F);
  sendCommand(SSD1306_SETDISPLAYOFFSET);    // 0xD3
  sendCommand(0x0);                         // no offset
  sendCommand(SSD1306_SETSTARTLINE | 0x0);  // line #0
  sendCommand(SSD1306_CHARGEPUMP);          // 0x8D
  sendCommand(0x14);
  sendCommand(SSD1306_MEMORYMODE);          // 0x20
  sendCommand(0x00);			  // was: 0x2 page mode
  sendCommand(SSD1306_SEGREMAP | 0x1);
  sendCommand(SSD1306_COMSCANDEC);
  sendCommand(SSD1306_SETCOMPINS);          // 0xDA
  sendCommand(0x12);
  sendCommand(SSD1306_SETCONTRAST);         // 0x81
  sendCommand(0xCF);
  sendCommand(SSD1306_SETPRECHARGE);        // 0xd9
  sendCommand(0xF1);
  sendCommand(SSD1306_SETVCOMDETECT);       // 0xDB
  sendCommand(0x40);
  sendCommand(SSD1306_DISPLAYALLON_RESUME); // 0xA4
  sendCommand(SSD1306_NORMALDISPLAY);       // 0xA6
  
  sendCommand(SSD1306_DISPLAYON);//--turn on oled panel
}

void SSD1306_text::setClock(uint32_t clock_Hz){
  Wire.setClock(clock_Hz);
}

//------------------------------------------------------------------------------
// clear the screen
void SSD1306_text::clear() {
  sendCommand(SSD1306_COLUMNADDR);
  sendCommand(0);   // Column start address (0 = reset)
  sendCommand(SSD1306_LCDWIDTH-1); // Column end address (127 = reset)

  sendCommand(SSD1306_PAGEADDR);
  sendCommand(0); // Page start address (0 = reset)
  sendCommand(7); // Page end address

#if I2C
  for (uint16_t i=0; i<(SSD1306_LCDWIDTH*SSD1306_LCDHEIGHT/8); i++) {
    Wire.beginTransmission(SSD1306_I2C_ADDRESS);
    Wire.write(0x40);
    for (uint8_t x=0; x<16; x++) {
      Wire.write(0x00);
      i++;
    }
    i--;
    Wire.endTransmission();
  }
#else
  *csport |= cspinmask;
  *dcport |= dcpinmask;
  *csport &= ~cspinmask;

  for (uint16_t i=0; i<(SSD1306_LCDWIDTH*SSD1306_LCDHEIGHT/8); i++) {
    spiWrite(0x00);
  }
  *csport |= cspinmask;
#endif
}
//------------------------------------------------------------------------------
void SSD1306_text::setCursor(uint8_t row, uint8_t col) {
  if (row >= SSD1306_LCDHEIGHT/8) {
    row = SSD1306_LCDHEIGHT/8 - 1;
  }
  if (col >= SSD1306_LCDWIDTH) {
    col = SSD1306_LCDWIDTH - 1;
  }
  row_ = row;	// row is 8 pixels tall; must set to byte sized row
  col_ = col;	// col is 1 pixel wide; can set to any pixel column

  sendCommand(SSD1306_SETLOWCOLUMN | (col & 0XF));
  sendCommand(SSD1306_SETHIGHCOLUMN | (col >> 4));
  sendCommand(SSD1306_SETSTARTPAGE | row);
}
//------------------------------------------------------------------------------

size_t SSD1306_text::write(const char* s)
{
  size_t n = strlen(s);
  size_t cn = 0;
  while (n > 0)
  {
    int num_chars = min(5, n); // hardcoding 5 chars max before the WIRE library coughs up
    if ((col_ / 6) + num_chars >= SSD1306_LCDWIDTH/6)
    {
      num_chars = (SSD1306_LCDWIDTH - col_)/6;
      if (num_chars <= 0) break;
    }
    
    byte byte_buf[30]; // max 5 chars
    for (int i=0;i<num_chars;i++,cn++)
    {
      byte ch = s[cn];
      ch -=32;
      uint8_t *base = font + 5 * ch;
      int j=0;
      for (j=0;j<5;j++)  byte_buf[i*6+j] = pgm_read_byte(base + j);
      byte_buf[i*6+5]=0;
    }
    sendData(byte_buf, num_chars*6);
    n-=num_chars;
    col_ += num_chars;
  }
  return cn;
}


size_t SSD1306_text::write(uint8_t c) {
  if (textSize_ == 1) {		// dedicated code since it's 4x faster than scaling
      col_ += 7;	// x7 font
      if (c < 32 || c > 127) c = 127;
      c -= 32;
      uint8_t *base = font + 5 * c;
      for (uint8_t i = 0; i < 5; i++ ) {
        uint8_t b = pgm_read_byte(base + i);
        sendData(b);
      }
      for (uint8_t i=0; i<textSpacing_; i++) {
        col_++; 
        sendData(0);	// textSpacing_ pixels of blank space between characters
      }
  } else {                      // scale characters (up to 8X)

    uint8_t sourceSlice, targetSlice, sourceBitMask, targetBitMask, extractedBit, targetBitCount;
    uint8_t startRow = row_;
    uint8_t startCol = col_;
      
    for (uint8_t irow = 0; irow < textSize_; irow++) {
      if (row_+irow > SSD1306_LCDWIDTH - 1) break;
      if (irow > 0) setCursor(startRow+irow, startCol);
      for (uint8_t iSlice=0; iSlice<5; iSlice++) {
        sourceSlice = pgm_read_byte(font + 5 * (c-32) + iSlice);
        targetSlice = 0;
        targetBitMask = 0x01;
        sourceBitMask = 0x01 << (irow*8/textSize_);
        targetBitCount = textSize_*7 - irow*8;
        do {
          extractedBit = sourceSlice & sourceBitMask;
          for (uint8_t i=0; i<textSize_; i++) {
            if (extractedBit != 0) targetSlice |= targetBitMask;
            targetBitMask <<= 1;
            targetBitCount--;
            if (targetBitCount % textSize_ == 0) {
              sourceBitMask <<= 1;
              break;
            }
            if (targetBitMask == 0) break;
          }
        } while (targetBitMask != 0);
#if I2C
        Wire.beginTransmission(SSD1306_I2C_ADDRESS);
        Wire.write(0x40);
        for (uint8_t i=0; i<textSize_; i++) {
          Wire.write(targetSlice);
        }

        Wire.endTransmission();
#else
        *csport |= cspinmask;
        *dcport |= dcpinmask;
        *csport &= ~cspinmask;
        for (uint8_t i=0; i<textSize_; i++) {
          spiWrite(targetSlice);
        }
        *csport |= cspinmask;
#endif
      }
    }
    setCursor(startRow, startCol + 5*textSize_ + textSpacing_);
  }

  return 1;
}

//------------------------------------------------------------------------------
/*
size_t SSD1306_text::write(const char* s) {
  size_t n = strlen(s);
  for (size_t i = 0; i < n; i++) {
    write(s[i]);
  }
  return n;
}
*/
//------------------------------------------------------------------------------
void SSD1306_text::writeInt(int i) {	// slightly smaller than system print()
    char buffer[7];
    itoa(i, buffer, 10);
    write(buffer);
}


size_t SSD1306_text::writeHorizCentered(const char* s)
{
  size_t n = strlen(s);
  uint8_t start_x;
  start_x = (SSD1306_LCDWIDTH - (n*(5+textSpacing_)))/2;
  setCursor(row_, start_x);
  write(s);
}

//------------------------------------------------------------------------------
#define BUFFER_CHUNK_SIZE 10
void SSD1306_text::fillRowRect(uint8_t x, uint8_t row_start, uint8_t w, bool white){
  bool inside = (x < SSD1306_LCDWIDTH)  &&  (row_start < SSD1306_LCDHEIGHT / 8);
  if(inside){
    uint8_t width_chunk = (w/BUFFER_CHUNK_SIZE);
    setCursor(row_start, x);
    uint8_t last_x = min((x + w), (SSD1306_LCDWIDTH - 1));
    
    uint8_t color = (white ? 0xFF : 0x00);
    uint8_t color_buf[BUFFER_CHUNK_SIZE];
    for(uint8_t ind = 0; ind < BUFFER_CHUNK_SIZE; ind++){
      color_buf[ind] = color;
    }
    uint8_t x_chunk;
    for(x_chunk = 0; x_chunk < width_chunk; x_chunk++){
      sendData(color_buf, BUFFER_CHUNK_SIZE);
    }
    setCursor(row_start, x + (x_chunk*8));
    for(uint8_t x_pos = x + (x_chunk*8); x_pos < last_x; x_pos++){
      sendData(color);
    }
  }
}


void SSD1306_text::drawPixel(uint8_t x, uint8_t y)
{
  setCursor((y/8), x);
  uint8_t pixel_byte = 0, y_offset;
  y_offset = y & 0x07;

  #if USE_FRAME_BUFFER
    uint32_t index = x + (y/8)*SSD1306_LCDWIDTH;
    pixel_byte = frame_buf[index] | (1 << y_offset);
    frame_buf[index] = pixel_byte;
  #else
    pixel_byte = 1 << y_offset;
  #endif
  
  sendData(pixel_byte);
}



//------------------------------------------------------------------------------
void SSD1306_text::sendCommand(uint8_t c) {
#if I2C
  uint8_t control = 0x00;   // Co = 0, D/C = 0
  Wire.beginTransmission(SSD1306_I2C_ADDRESS);
  Wire.write(control);
  Wire.write(c);
  Wire.endTransmission();
#else
  *csport |= cspinmask;
  *dcport &= ~dcpinmask;
  *csport &= ~cspinmask;
  spiWrite(c);
  *csport |= cspinmask;
#endif
}
//------------------------------------------------------------------------------

void SSD1306_text::sendData(uint8_t *pbytes, uint8_t len)
{
#if I2C
  uint8_t control = 0x40; // Co = 0, D/C = 1
  Wire.beginTransmission(SSD1306_I2C_ADDRESS);
  Wire.write(control);
  Wire.write(pbytes,len);
  Wire.endTransmission();
#else
  *csport |= cspinmask;
  *dcport |= dcpinmask;
  *csport &= ~cspinmask;
  spiWrite(c);
  *csport |= cspinmask;
#endif
}


void SSD1306_text::sendData(uint8_t c) {
#if I2C
    uint8_t control = 0x40;   // Co = 0, D/C = 1
    Wire.beginTransmission(SSD1306_I2C_ADDRESS);
    Wire.write(control);
    Wire.write(c);
    Wire.endTransmission();
#else
  *csport |= cspinmask;
  *dcport |= dcpinmask;
  *csport &= ~cspinmask;
  spiWrite(c);
  *csport |= cspinmask;
#endif
}

//------------------------------------------------------------------------------
#if !I2C
inline void SSD1306_text::spiWrite(uint8_t c) {

#if HW_SPI
    (void)SPI.transfer(c);
#else // bit twiddle SPI
    for(uint8_t bit = 0x80; bit; bit >>= 1) {
      *clkport &= ~clkpinmask;
      if(c & bit) *mosiport |=  mosipinmask;
      else        *mosiport &= ~mosipinmask;
      *clkport |=  clkpinmask;
    }
#endif
}
#endif
//------------------------------------------------------------------------------
