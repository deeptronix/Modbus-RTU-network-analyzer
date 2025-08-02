
bool MB_decode(char* message, uint8_t length, MB_DATA_t &MB_message, MB_DATA_t &MB_response){
  MB_message.slave_id = uint8_t(message[0]);
  MB_message.func_code = uint8_t(message[1]);
  MB_message.crc = (uint16_t(message[length - 2]) << 8) + uint8_t(message[length - 1]);   // CRC is a 16bit wide value

  if(isReadFC(MB_message.func_code)){
    MB_message.side_master = (length == 8);   // a READ function code is signifies a query of exactly 8 characters
  }
  else{
    MB_message.side_master = (length >= 10);  // a WRITE function code signifies a query of more than 10 characters (telegram includes data to be written)
  }

  MB_message.exception = bool((MB_message.func_code & 0x80) >> 7);    // MB exception when 8th (highest) bit of FC is set

  if(MB_message.exception){
    MB_message.func_code = (MB_message.func_code & 0x7F);   // overwrite FC because in exception the MS bit is set
    MB_message.excep_code = uint8_t(message[2]);
    MB_message.reg_offs = 0;   // offset not available in exception
  }
  else{
    MB_message.excep_code = 0;
    if(MB_message.side_master  ||  !isReadFC(MB_message.func_code)){
      MB_message.reg_offs = (uint16_t(message[2]) << 8) + uint8_t(message[3]);    // offset register is a 16bit wide value
    }
    else{
      MB_message.reg_offs = 0;   // offset not available in READ SLAVE response
    }
  }

  // Detect if we received a slave response as well (the message may have not been split correctly, or the response delay is shorter than the timeout)
  if(MB_message.side_master){
    uint8_t response_start = 0;
    uint8_t hypoth_sl_id, hypoth_fc;
    for(uint8_t search_ind = 8; search_ind < length - 1; search_ind++){   // Master sends AT LEAST 8 characters: no need to check before that point
      hypoth_sl_id = uint8_t(message[search_ind]);
      hypoth_fc = (uint8_t(message[search_ind + 1]) & 0x7F);   // overwrite FC because in exception the MS bit is set
      if((MB_message.slave_id == hypoth_sl_id)  &&  (MB_message.func_code == hypoth_fc)){   // To find if the response is in the message, try to match slave ID and function code with those of the master
        response_start = search_ind;
        break;
      }
    }
    
    if(response_start > 0){   // the query response is embedded in the master query as well: decode response data
      MB_response.side_master = false;
      MB_response.slave_id = uint8_t(message[response_start]);
      MB_response.func_code = uint8_t(message[response_start + 1]);
      MB_response.reg_offs = 0;   // offset not available in READ SLAVE response
      MB_response.crc = (uint16_t(message[length - 2]) << 8) + uint8_t(message[length - 1]);
      MB_response.exception = bool((MB_response.func_code & 0x80) >> 7);
      if(MB_response.exception){
        MB_response.excep_code = uint8_t(message[response_start + 2]);
      }
      else{
        MB_response.excep_code = 0;
      }
      
      MB_message.crc = (uint16_t(message[response_start - 2]) << 8) + uint8_t(message[response_start - 1]);   // FIX query (master) CRC, because beforehand we took the slave response crc instead
    }
    else{
      MB_response.slave_id = 0;
    }
  }
  else{
    MB_response.slave_id = 0;
  }

  bool is_valid_MB_telegram = isValidFC(MB_message.func_code)  &&  MB_message.side_master;
  return is_valid_MB_telegram;
}

inline bool isReadFC(uint8_t FC){
  FC = (FC & 0x7F);   // remove exception bit if set
  // true = any of these function codes
  return ((FC <= 4)  ||  (FC == 7)  ||  (FC == 8));
}

inline bool isWriteFC(uint8_t FC){
  FC = (FC & 0x7F);   // remove exception bit if set
  // true = any of these function codes
  return ((FC == 5)  ||  (FC == 6)  ||  (FC == 15)  ||  (FC == 16)  ||  (FC == 22)  ||  (FC == 23));
}


bool isValidFC(uint8_t FC){
  // true = either a READ or a WRITE function code (we ignore diagnostic and other FC, as they are rarely used)
  return (isReadFC(FC)  ||  isWriteFC(FC));
}


// eof
