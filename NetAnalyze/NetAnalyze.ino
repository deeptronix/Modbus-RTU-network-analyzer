#include "definitions.h"

#include <SoftwareSerial.h>
#include <Wire.h>
#include <EEPROM.h>
#include "TON.h"
#include "TOF.h"
#include "R_TRIG.h"
#include "F_TRIG.h"
#include "RT_TOGGLE.h"
#include "DEBOUNCE.h"

#include "SSD1306_text.h"
SSD1306_text display(OLED_RESET);

SoftwareSerial RS485_Serial(SERIAL_RX_PIN, SERIAL_TX_PIN, false);  // RX, TX, invert

R_TRIG RT_PAGE_VIEW;
DEBOUNCE PAGE_VIEW_DEBOUNCE;
TON LONGPRESS;
R_TRIG RT_FILTER;

R_TRIG RT_SERIAL_FREEZE;
DEBOUNCE SERIAL_FREEZE_DEBOUNCE;

TON FILTER_TIMER;

TON MSG_OVER;
F_TRIG FT_END_MSG;

TON NOISE_DISPLAY_RATE;

TOF LED_DECODE_TIMER;


void setup() {
  // Serial init
  RS485_Serial.begin(38400);
  RS485_Serial.setTimeout(1);

  // GPIO init
  pinMode(LED_TRAFFIC_PIN, OUTPUT);
  pinMode(LED_MODBUS_PIN, OUTPUT);
  // Briefly turn on LEDs to show them working
  digitalWritePA2(true);
  digitalWritePA3(true);
  delay(50);
  digitalWritePA2(false);
  digitalWritePA3(false);
  
  pinMode(PAGE_CHANGE_PIN, INPUT_PULLUP);
  PAGE_VIEW_DEBOUNCE.CNT_SP(30);
  PAGE_VIEW_DEBOUNCE.set_default = true;

  pinMode(FREEZE_PIN, INPUT_PULLUP);
  SERIAL_FREEZE_DEBOUNCE.CNT_SP(30);
  SERIAL_FREEZE_DEBOUNCE.set_default = true;

  // Timers & triggers init
  LONGPRESS.timestamp_handle = millis;
  LONGPRESS.PT(LONGPRESS_DELAY_ms);

  RT_PAGE_VIEW.CLK(true);   // initialize to true in order not to trigger falsely at boot
  RT_SERIAL_FREEZE.CLK(true);   // initialize to true in order not to trigger falsely at boot
  
  MSG_OVER.timestamp_handle = micros;
  MSG_OVER.PT(MESSAGE_TIMEOUT_us);

  FILTER_TIMER.timestamp_handle = micros;

  NOISE_DISPLAY_RATE.timestamp_handle = millis;
  NOISE_DISPLAY_RATE.PT(NOISE_FRAMERATE_ms);

  LED_DECODE_TIMER.timestamp_handle = millis;
  LED_DECODE_TIMER.PT(LED_DECODE_OFF_DELAY_ms);

  // ADC init
  analogReadResolution(ADC_NATIVE_RESOLUTION);
  analogSampleDuration(MAX_SAMPDUR);

  // Display init
  displayInit();
}

void loop() {

  // ****************** PERSISTENT RESTORE ******************
  static page_view_e page = page_view_e::traffic;

  static bool at_boot = true;
  if(at_boot){
    EEPROM.get(PAGE_EEADDR, page);
    at_boot = false;
  }
  

  // ******************* GENERAL SCRIPT VARIABLES *******************
  static bool update_display = true;
  static bool init_page = true;



  // ******************* BUTTONS *******************
  static bool inhibit_page_change = false, freeze = false, update_freeze = false;
  static bool serial_filter_active = false;
  bool stop_serial;
  
  PAGE_VIEW_DEBOUNCE.update(digitalRead(PAGE_CHANGE_PIN));
  LONGPRESS.IN(!PAGE_VIEW_DEBOUNCE.Q);
  LONGPRESS.update();

  RT_FILTER.CLK(LONGPRESS.Q);
  if(RT_FILTER.Q){
    serial_filter_active = !serial_filter_active;
  }
  inhibit_page_change = inhibit_page_change  ||  LONGPRESS.Q;
  
  RT_PAGE_VIEW.CLK(PAGE_VIEW_DEBOUNCE.Q);
  if(RT_PAGE_VIEW.Q){
    if(inhibit_page_change){
      inhibit_page_change = false;
    }
    else{
      change_page(page);
      update_display = true;
      init_page = true;
    }
  }

  SERIAL_FREEZE_DEBOUNCE.update(digitalRead(FREEZE_PIN));
  RT_SERIAL_FREEZE.CLK(SERIAL_FREEZE_DEBOUNCE.Q);
  if(RT_SERIAL_FREEZE.Q){
    freeze = !freeze;
    update_freeze = true;
  }
  
  stop_serial = freeze  ||  (page == page_view_e::noise);   // During noise measurement, decoding is not active (it would be pointless)

  

  // **************** SERIAL POLLING ****************
  static char message[MAX_MSG_LEN];
  static bool msg_incoming = false;
  static uint16_t ind = 0, recvd_chars = 0;

  static bool msg_discard = false, enable_decoding = true, msg_timer_rst = false, filter_timer_rst = false;
  static uint32_t aux_PT_us = 0;
  switch(page){
    case page_view_e::traffic:  aux_PT_us = serial_filter_active ? TRAFFIC_MSG_SLOW_RATE_us : 0;  break;
    case page_view_e::table:    aux_PT_us = serial_filter_active ? TABLE_MSG_SLOW_RATE_us : 0;    break;
    default:                    aux_PT_us = 0;
  }

  // optional filter to slow down message detection rate (this usually causes messages drop)
  FILTER_TIMER.PT(aux_PT_us);
  FILTER_TIMER.IN(!filter_timer_rst);
  FILTER_TIMER.update();
  filter_timer_rst = false;
  
  enable_decoding = !msg_discard  &&  (!serial_filter_active  ||  FILTER_TIMER.Q)  &&  (MSG_OVER.Q  ||  msg_incoming)  &&  !stop_serial;
  if(enable_decoding){
    while(RS485_Serial.available()){
      msg_incoming = true;
      msg_timer_rst = true;
      message[ind] = RS485_Serial.read();
      ind++;
      digitalWritePA2(true);    // turn ON led that shows serial activity
    }
  }
  else{
    // while not decoding flush & discard serial buffer
    while(RS485_Serial.available()){
      RS485_Serial.read();
      msg_timer_rst = true;
    }
    digitalWritePA2(false);    // turn OFF led that shows serial activity
  }

  MSG_OVER.IN(!msg_timer_rst);
  MSG_OVER.update();
  msg_timer_rst = false;

  if(MSG_OVER.Q){
    recvd_chars = ind;
    ind = 0;
    msg_incoming = false;
    msg_discard = false;
  }


  // **************** MODBUS DECODING ****************
  static MB_DATA_t MB_data, MB_response;
  static bool valid_MB_telegram = false;    // a VALID modbus RTU message has beed decoded

  FT_END_MSG.CLK(msg_incoming);
  if(FT_END_MSG.Q){
    valid_MB_telegram = MB_decode(message, recvd_chars, MB_data, MB_response);

    if(page == page_view_e::traffic){
      update_display = MB_data.side_master;    // In traffic mode, update display only if message starts from a master request
    }
    else if(page == page_view_e::table){
      update_display = (!MB_data.side_master)  ||  ((MB_response.slave_id > 0)  &&  (!MB_response.side_master));   // update for SLAVE RESPONSE, or a response embedded into master query
    }
    
    if(update_display){
      msg_discard = true;   // disable temporarily message decoding
      filter_timer_rst = true;
    }
  }

  LED_DECODE_TIMER.IN(valid_MB_telegram);
  LED_DECODE_TIMER.update();
  digitalWritePA3(LED_DECODE_TIMER.Q);    // turn ON led that shows valid MB messages
  valid_MB_telegram = false;


  // **************** NOISE MEASUREMENT ****************
  static bool noise_rate_rst = false;
  NOISE_DISPLAY_RATE.IN((page == page_view_e::noise)  &&  !noise_rate_rst);
  NOISE_DISPLAY_RATE.update();
  noise_rate_rst = false;

  float RMS_V = 0, RMS_dB = -100;
  if(NOISE_DISPLAY_RATE.Q){
    uint32_t ADC_value = 0;
    for(uint8_t ovrs = 0; ovrs < ADC_OVERSAMPLING; ovrs++){
      ADC_value = ADC_value + analogRead(RMS_ADC_PIN);
    }
    ADC_value = ADC_value / ADC_OVERSAMPLING;
    calculateNoiseFigures(ADC_value, ADC_NATIVE_RESOLUTION, ADC_OFFSET_COMP, INP_ATTENUATION_adim, RMS_V, RMS_dB);
    update_display = true;
    noise_rate_rst = true;
  }
  

  // *************** HMI DISPLAY ***************  
  if(update_display){
    update_display = false;
    
    if(init_page){
      display.clear();
      String page_title;
      switch(page){
        case page_view_e::traffic:   page_title = "TRAFFIC MONITOR";  break;
        case page_view_e::table:     page_title = "TABLE MONITOR";    break;
        case page_view_e::noise:     page_title = "NOISE MEASURE";     break;
        default:                     page_title = "";
      }
      const char* aux_title = page_title.c_str();
      display.setCursor(3, 0);
      display.writeHorizCentered(aux_title);
      delay(1000);
      display.clear();
      update_freeze = true;
    }

    if(page == page_view_e::traffic){
      drawTraffic(init_page, MB_data, MB_response);
    }
    else if(page == page_view_e::table){
      drawTableView(init_page, MB_data, MB_response);
    }
    else if(page == page_view_e::noise){
      drawNoise(init_page, RMS_V, RMS_dB);
    }

    init_page = false;
  }

  // Show/hide "pause" symbol if in/out of freeze mode
  if(update_freeze  &&  (page != page_view_e::noise)){
    if(freeze){
      drawPause();
    }
    else{
      clearPause();
    }
  }
  update_freeze = false;


  // ******************* PERSISTENT SAVE *******************
  EEPROM.put(PAGE_EEADDR, page);

}


void change_page(page_view_e &page){
  switch(page){
    case page_view_e::traffic:  page = page_view_e::table;    break;
    case page_view_e::table:    page = page_view_e::noise;    break;
    case page_view_e::noise:    page = page_view_e::traffic;  break;
    default:  page = page_view_e::traffic;
  }
}
