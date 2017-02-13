#include "Arduino.h"
#include "rom.h"
#include "programs.h"

#define CHECK_BIT(var,pos) ((var) & (1<<(pos)))

// General Control settings
const int SERIAL_SPEED = 115200; // Arduino Serial Speed
const int CLOCK_DELAY_PIN = A0;  // Clock delay PIN (potentiometer)
int CLOCK_DELAY = 5;  // HIGH / LOW CLOCK STATE DELAY (You can slow down it as much as you want)

const char SERIAL_BS = 0x08;

// 6502 to Arduino Pin Mapping
const int CLOCK_PIN   = 52; // TO 6502 CLOCK
const int RW_PIN      = 53; // TO 6502 R/W
const int ADDRESS_PINS[]  = {44,45,2,3,4,5,6,7,8,9,10,11,12,13,46,47}; // TO ADDRESS PIN 1-15 6502
const int DATA_PINS[]     = {33, 34, 35, 36, 37,38, 39, 40}; // TO DATA BUS PIN 0-7 6502

const unsigned int ROM_ADDR       = 0xFF00; // ROM
const unsigned int RAM_BANK1_ADDR = 0x0000; // RAM
const unsigned int RAM_BANK2_ADDR = 0xE000; // EXTENDED RAM

const unsigned int XAML = 0x24;   // Last "opened" location Low
const unsigned int XAMH = 0x25;   // Last "opened" location High
const unsigned int STL  = 0x26;   // Store address Low
const unsigned int STH  = 0x27;   // Store address High
const unsigned int L    = 0x28;   // Hex value parsing Low
const unsigned int H    = 0x29;   // Hex value parsing High
const unsigned int YSAV = 0x2A;   // Used to see if hex value is given
const unsigned int MODE = 0x2B;   // $00=XAM, $7F=STOR, $AE=BLOCK XAM
const unsigned int IN   = 0x200;  // Input buffer ($0200,$027F)

const int RAM_BANK_1_SIZE = 4096;
const int RAM_BANK_2_SIZE = 4096;
unsigned char RAM_BANK_1[RAM_BANK_1_SIZE];
unsigned char RAM_BANK_2[RAM_BANK_2_SIZE];

// PIA MAPPING 6821
const unsigned int PIA_ADDR   = 0xD000; // PIA 6821 ADDR BASE SPACE
const unsigned int KBD_ADDR   = 0xD010; // Keyb Char - B7 High on keypress
const unsigned int KBDCR_ADDR = 0xD011; // Keyb Status - B7 High on keypress / Low when ready
const unsigned int DSP_ADDR   = 0xD012; // DSP Char
const unsigned int DSPCR_ADDR = 0xD013; // DSP Status - B7 Low if VIDEO ready
unsigned char KBD   = 0;
unsigned char KBDCR = 0;
unsigned char DSP   = 0;
unsigned char DSPCR = 0;

const unsigned char BS      = 0xDF;  // Backspace key, arrow left key (B7 High)
const unsigned char CR      = 0x8D;  // Carriage Return (B7 High)
const unsigned char ESC     = 0x9B;  // ESC key (B7 High)

// 6502 States buffer
unsigned int  address;    // Current address (from 6502)
unsigned char bus_data;   // Data Bus value (from 6502)
int rw_state;             // Current R/W state (from 6502)

// 6502 previous States  buffer
// We use them to optimize the performance a bit
unsigned int  pre_address;    // Previous address (from 6502)
unsigned char pre_bus_data;   // Previous Bus value (from 6502)
int pre_rw_state;             // Previous R/W state (from 6502)


// Set Arduino Address connected PINS as INPUT
void setupAddressPins() {
  for (int i = 0; i < 16; ++i) {
    pinMode(ADDRESS_PINS[i], INPUT);
  }
}

// Set Arduino Bus conneced pins mode as IN or OUT
void setBusMode(int mode) {
  for (int i = 0; i < 8; ++i) {
    pinMode(DATA_PINS[i], mode);
  }
}

// Read 6502 Address PINS and store the WORD in our address var
void readAddress() {
  address = 0;
  for (int i = 0; i < 16; ++i)
  {
    address = address << 1;
    address += (digitalRead(ADDRESS_PINS[16-i-1]) == HIGH)?1:0;
  }
}

// Read 6502 Data PINS and store the BYTE in our bus_data var
void readData() {
  bus_data = 0;
  for (int i = 0; i < 8; ++i)
  {
    bus_data = bus_data << 1;
    bus_data += (digitalRead(DATA_PINS[8-i-1]) == HIGH)?1:0;
  }
}

// Read RW_PIN state and set the busMode (aruduino related PINS) to OUTPUT or INPUT
void handleRWState() {
  int tmp_rw_state=digitalRead(RW_PIN);

  if (rw_state != tmp_rw_state) {
    rw_state=tmp_rw_state;
    rw_state ? setBusMode(OUTPUT) : setBusMode(INPUT);
  }
}

// Send a byte to the 6502 DATA BUS
void byteToDataBus(unsigned char data) {
  for (int i = 0; i < 8; i++) {
    digitalWrite(DATA_PINS[i], CHECK_BIT(data, i));
  }
}

void PIAWrite() {
  switch (address) {

    case KBD_ADDR:
      KBD=bus_data;
      break;

    case KBDCR_ADDR:
      KBDCR=bus_data;
      break;

    case DSP_ADDR:
      DSP = bus_data;

      switch(DSP) {
        case CR:
          Serial.write('\r');
          Serial.write('\n');
          break;
        case BS:
          Serial.write(SERIAL_BS);
          break;
        default:
          Serial.write(DSP & 0x7F);
          break;
      }

      bitClear(DSP, 7);
      break;

    case DSPCR_ADDR:
      DSPCR=bus_data;
      break;
  }
}

// READ FROM DATA BUS - STORE AT RELATED ADDRESS
void readFromDataBus() {
  readData();

  switch (address >> 12) {
    case 0x0:
      RAM_BANK_1[address-RAM_BANK1_ADDR]=bus_data;
      break;
    case 0xE:
      RAM_BANK_2[address-RAM_BANK2_ADDR]=bus_data;
      break;
    case 0xD:
      PIAWrite();
      break;
  }
}

unsigned char PIARead() {
  unsigned char val;
  // PIA 6821
  switch (address) {

    case KBD_ADDR:
      val=KBD;
      // We'v read the char, clear B7
      bitClear(KBDCR, 7);
      break;

    case KBDCR_ADDR:
      val=KBDCR;
      break;

    case DSP_ADDR:
      val=DSP;
      break;

    case DSPCR_ADDR:
      val=DSPCR;
      break;
    default:
      val=0;
      break;
  }

  return val;
}

void writeToDataBus() {
  unsigned char val=0;

  switch (address >> 12) {
    case 0x0:
      val=RAM_BANK_1[address-RAM_BANK1_ADDR];
      break;
    case 0xE:
      val=RAM_BANK_2[address-RAM_BANK2_ADDR];
      break;
    case 0xF:
      val=ROM[address-ROM_ADDR];
      break;
    case 0xD:
      val=PIARead();
      break;
    default:
      val=0;
      break;
  }
  byteToDataBus(val);
}

void handleKeyboard() {
  // KEYBOARD INPUT
  if (Serial.available() > 0) {
    char tempKBD = Serial.read();
    switch (tempKBD) {
      case 0xA:
        // Not expected from KEYB
        // Just ignore
        return;
        break;
      case 0x8:
      case 0x7F:
        // BS
        tempKBD = 0x5F;
        break;
    }

    KBD = tempKBD;

    bitSet(KBD, 7);
    bitSet(KBDCR, 7);
  }
}

void loadBASIC() {
  // LOAD BASIC in E000
  for (unsigned int i = 0; i < sizeof(BASIC) ; i++) {
    RAM_BANK_2[i] = BASIC[i];
  }
  Serial.println("BASIC LOADED");
}

void loadPROG() {
  // LOAD A PROG
  unsigned int prg_addr = AUTLOAD[1] | AUTLOAD[0] << 8;
  Serial.print("PROGRAM AT: ");
  Serial.println(prg_addr, HEX);

  for (unsigned int i = 0; i < sizeof(AUTLOAD)-2 ; i++) {
    RAM_BANK_1[prg_addr+i] = AUTLOAD[i+2];
  }
}

void setup() {
  pinMode(CLOCK_PIN, OUTPUT);
  pinMode(RW_PIN, INPUT);
  pinMode(RW_PIN, INPUT);

  // You can remove the PIN input here and just set CLOCK_DELAY as const
  // Remove also the analogRead on CLOCK_DELAY_PIN in step() below.
  pinMode(CLOCK_DELAY_PIN, INPUT);
  CLOCK_DELAY=analogRead(CLOCK_DELAY_PIN);
  // End of remove


  setupAddressPins();
  setBusMode(OUTPUT);

  Serial.begin(SERIAL_SPEED);

  Serial.println("----------------------------");
  Serial.println("APPLE 1 REPLICA by =STID=");
  Serial.println("----------------------------");
  Serial.print("ROM:  ");
  Serial.print(sizeof(ROM));
  Serial.println(" BYTE");
  Serial.print("RAM:  ");
  Serial.print(sizeof(RAM_BANK_1));
  Serial.println(" BYTE");
  Serial.print("ERAM: ");
  Serial.print(sizeof(RAM_BANK_2));
  Serial.println(" BYTE");
  Serial.print("CLOCK DELAY: ");
  Serial.println(CLOCK_DELAY);

  loadBASIC();
  loadPROG();

  Serial.println("----------------------------");
}

void handleClock() {
  // LOW CLOCK
  digitalWrite(CLOCK_PIN, LOW);
  delayMicroseconds(CLOCK_DELAY);

  // RW STATE
  handleRWState();

  // HIGH CLOCK
  digitalWrite(CLOCK_PIN, HIGH);
  delayMicroseconds(CLOCK_DELAY);
}

void handleBusRW() {

  // If nothing changed from the last cycle, we don't need to upadte anything
  if (pre_address != address || pre_rw_state != rw_state) {
    // READ OR WRITE TO BUS?
    rw_state ? writeToDataBus() : readFromDataBus();
    pre_address = address;
    pre_rw_state = rw_state;
  }
}

void step() {

  CLOCK_DELAY=analogRead(CLOCK_DELAY_PIN); // Can be removed, see setup()
  handleClock();
  readAddress();
  handleBusRW();
  handleKeyboard();
}

void loop () {
    step();
}
