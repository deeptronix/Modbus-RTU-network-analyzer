
void displayInit(){
  delay(100);
  display.init();
  display.setClock(400000);
  display.clear();
}


void drawPause(){
  display.setCursor(0, SSD1306_LCDWIDTH - 8);
  uint8_t symbol[8] = {0, 0, 127, 127, 0, 0, 127, 127};
  for(uint8_t x = 0; x < 8; x++){
    display.sendData(symbol[x]);
  }
}

void clearPause(){
  display.setCursor(0, SSD1306_LCDWIDTH - 8);
  for(uint8_t x = 0; x < 8; x++){
    display.sendData(0);
  }
}


void digitalWritePA2(bool enable){
  if(enable){
    PORTA.OUTSET = 0b00000100;
  }
  else{
    PORTA.OUTCLR = 0b00000100;
  }
}


void digitalWritePA3(bool enable){
  if(enable){
    PORTA.OUTSET = 0b00001000;
  }
  else{
    PORTA.OUTCLR = 0b00001000;
  }
}







// eof
