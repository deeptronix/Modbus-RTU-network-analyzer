
void drawTraffic(bool init_view, MB_DATA_t MB_data, MB_DATA_t MB_response){
  static uint8_t current_row = 0, current_char = 0;
  if(init_view){
    current_row = 0;
    current_char = 0;
  }

  MB_DATA_t data;
  for(uint8_t entry = 0; entry < 2; entry++){   // as if we "always" receive both master AND slave response; we will short circuit later, if necessary
    switch(entry){
      case 0:   data = MB_data;  break;
      case 1:   data = MB_response;  break;
      default:  data = MB_data;
    }

    if(data.slave_id > 0){
      String UI_msg = String(data.slave_id);
      if(isReadFC(data.func_code)){
        UI_msg = UI_msg + ".R";   // a READ query
      }
      else{
        UI_msg = UI_msg + ".W";   // a WRITE query
      }
      
      if(data.exception){
        UI_msg = UI_msg + ".x." + String(data.excep_code);  // add a 'x' symbol to show an exception has taken place
      }
      else{
        UI_msg = UI_msg + "." + String(data.reg_offs);
      }
      const char* aux_msg = UI_msg.c_str();
      
      // Calculate display alignment
      if(data.side_master){
        current_char = 0;   // Master on the left
      }
      else{
        current_char = (21 - UI_msg.length());    // Slave response on the right
      }

      display.setCursor(current_row, current_char*6);
      display.write(aux_msg);

      current_row++;
      if(current_row >= 8){
        current_row = 0;
      }

      display.fillRowRect(0, current_row, SSD1306_LCDWIDTH, false);   // Prepare next row to be used by clearing it
    }
  }
}


// eof
