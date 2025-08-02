
// Pin declarations
#define PAGE_CHANGE_PIN PIN_PA4
#define FREEZE_PIN PIN_PA5
#define SERIAL_TX_PIN PIN_PA6   // TX not used, but needs to be assigned
#define SERIAL_RX_PIN PIN_PA7
#define RMS_ADC_PIN PIN_PA1
#define LED_MODBUS_PIN PIN_PA2
#define LED_TRAFFIC_PIN PIN_PA3


// Peripheral configuration
#define MAX_SAMPDUR 255
#define ADC_OVERSAMPLING 50
#define ADC_FSR_VOLTAGE_V 5.0
#define ADC_OFFSET_COMP 0
#define INP_ATTENUATION_adim 1.0


// Structs & enums
enum struct page_view_e : uint8_t {
  traffic,
  table,
  noise
};

typedef struct MB_DATA_t {
  uint8_t slave_id;
  uint8_t func_code;
  uint16_t reg_offs;
  uint16_t crc;
  bool exception;
  uint8_t excep_code;
  bool side_master;
} MB_DATA_t;


// Display definitions
#define OLED_RESET 4

#define SLAVES_PER_TABLE_ROW 5
#define SLAVE_SPACING_PX 8


// Operative definitions
#define MAX_MSG_LEN 128

#define MESSAGE_TIMEOUT_us 3000
#define TRAFFIC_MSG_SLOW_RATE_us 451753   // about 500ms - but not exactly! This way we will avoid syncing to a precise serial master
#define TABLE_MSG_SLOW_RATE_us 83671      // about 100ms - but not exactly! This way we will avoid syncing to a precise serial master
#define NOISE_FRAMERATE_ms 100

#define LONGPRESS_DELAY_ms 1000
#define LED_DECODE_OFF_DELAY_ms 10


// Persistent memory
#define PAGE_EEADDR 10





// eof
