
void drawTableView(bool init_view, MB_DATA_t MB_data, MB_DATA_t MB_response){
  static uint8_t table_current_col[8] = {0};
  if(init_view){
    for(uint8_t index = 0; index < 8; index++){
      table_current_col[index] = 0;
    }
  }
  
  MB_DATA_t data;
  bool render = false;
  if(!MB_data.side_master  &&  (MB_data.slave_id > 0)){   // we only show answers from slaves of the network. Here, queries are not interesting. Priority to direct data
    data = MB_data;
    render = true;
  }
  else if(!MB_response.side_master  &&  (MB_response.slave_id > 0)){    // we only show answers from slaves of the network. Here, queries are not interesting
    data = MB_response;
    render = true;
  }

  if(render){
    uint8_t slot_row = 0;
    slot_row = ((data.slave_id - 1) / 32);   // up to 32 slaves per row (8 rows available on this display)

    String slave_str = String(data.slave_id);
    if(data.exception){
      slave_str = slave_str + "x";    // add a 'x' symbol at the end of the slave ID to show an exception has taken place
    }
    const char* aux_slaveid = slave_str.c_str();
    
    display.setCursor(slot_row, table_current_col[slot_row]*(6*3 + SLAVE_SPACING_PX));
    display.write(aux_slaveid);

    table_current_col[slot_row] = (table_current_col[slot_row] + 1) % SLAVES_PER_TABLE_ROW;

    display.fillRowRect(table_current_col[slot_row]*(6*3 + SLAVE_SPACING_PX), slot_row, (6*3 + SLAVE_SPACING_PX), false);
  }
}


// eof
