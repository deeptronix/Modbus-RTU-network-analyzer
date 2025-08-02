
void calculateNoiseFigures(uint32_t AD8361_ADC_raw, uint8_t resolution_bits, float ADC_offset_compensation_V, float input_attenuation_adim, float &RMS_V, float &RMS_dB){
  float AD8361_V;
  if(input_attenuation_adim > 0){
    AD8361_V = ADC_FSR_VOLTAGE_V*(float(AD8361_ADC_raw) / float((1 << resolution_bits)))/input_attenuation_adim;
    AD8361_V = max(0, (AD8361_V - ADC_offset_compensation_V));
    
    RMS_V = 0.133333*AD8361_V;    // AD8361 uses a x7.5 (/0.133) gain to pre-multiply the measured RMS value (see datasheet); we have to un-do this gain operation to obtain the true RMS measurement

    float log_arg;
    log_arg = (AD8361_V/ADC_FSR_VOLTAGE_V);
    if(log_arg > 0){
      RMS_dB = 20.0*log10(log_arg);
    }
    else{
      RMS_dB = -100.0;
    }
  }
}


void drawNoise(bool init_view, float RMS_V, float RMS_dB){
  static uint8_t x_pos = 0;
  if(init_view){
    x_pos = 0;
    display.setCursor(0, 5);
    display.write("RMS [V]");

    display.setCursor(0, 80);
    display.write("RMS [dB]");
  }
  
  display.fillRowRect(0, 1, SSD1306_LCDWIDTH, false);
  display.setCursor(1, 10);
  String rmsV_str = String(RMS_V, 3);   // 3 digits after the decimal point
  const char* auxV = rmsV_str.c_str();
  display.write(auxV);

  display.setCursor(1, 85);
  String rmsdB_str = String(RMS_dB, 1);
  const char* auxdB = rmsdB_str.c_str();
  display.write(auxdB);

  uint8_t noise_power_perct;
  noise_power_perct = max(0, min(map(RMS_dB, -30, -5, 0, 100), 100));
  
  uint8_t symbol;
  symbol = thermometricSymbol(noise_power_perct, 0, 20);
  display.setCursor(7, x_pos);
  display.sendData(symbol);
  symbol = thermometricSymbol(noise_power_perct, 20, 40);
  display.setCursor(6, x_pos);
  display.sendData(symbol);
  symbol = thermometricSymbol(noise_power_perct, 40, 60);
  display.setCursor(5, x_pos);
  display.sendData(symbol);
  symbol = thermometricSymbol(noise_power_perct, 60, 80);
  display.setCursor(4, x_pos);
  display.sendData(symbol);
  symbol = thermometricSymbol(noise_power_perct, 80, 100);
  display.setCursor(3, x_pos);
  display.sendData(symbol);
  
  x_pos = (x_pos + 1) % SSD1306_LCDWIDTH;
}


uint8_t thermometricSymbol(uint8_t value, uint8_t lower, uint8_t upper){
  uint8_t output = 0;
  if(upper > lower){
    if(value >= upper){
      output = 0xFF;
    }
    else if(value <= lower){
      output = 0x00;
    }
    else{
      uint8_t cmp_bit[8];
      uint8_t diff_div8;
      diff_div8 = (upper - lower) >> 3;
      cmp_bit[0] = diff_div8*8;
      cmp_bit[1] = diff_div8*7;
      cmp_bit[2] = diff_div8*6;
      cmp_bit[3] = diff_div8*5;
      cmp_bit[4] = diff_div8*4;
      cmp_bit[5] = diff_div8*3;
      cmp_bit[6] = diff_div8*2;
      cmp_bit[7] = diff_div8;
      uint8_t cmp_value;
      cmp_value = value - lower;
      output = ((cmp_value >= cmp_bit[7]) << 7)
             | ((cmp_value >= cmp_bit[6]) << 6)
             | ((cmp_value >= cmp_bit[5]) << 5)
             | ((cmp_value >= cmp_bit[4]) << 4)
             | ((cmp_value >= cmp_bit[3]) << 3)
             | ((cmp_value >= cmp_bit[2]) << 2)
             | ((cmp_value >= cmp_bit[1]) << 1)
             | ((cmp_value >= cmp_bit[0]))
           ;
    }
  }
  
  return output;
}





// eof
