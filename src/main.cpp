#include "Arduino.h"
#include "rom.h"

#define CHECK_BIT(var,pos) ((var) & (1<<(pos)))

const int CLOCK_PIN = 52;
const int RW_PIN = 53;
const int CLOCK_DELAY = 100;

const int NUM_ADDR_PINS = 16;
const int NUM_DATA_PINS = 8;
const int ADDRESS_PINS[] = {44,45,2,3,4,5,6,7,8,9,10,11,12,13,46,47};
const int DATA_PINS[] = {30,31,32,33,34,35,36,37};

const unsigned int ROM_ADDR = 0xFF00;
const unsigned int RAM_BANK1_ADDR = 0x0000;
const unsigned int RAM_BANK2_ADDR = 0xE000;
const unsigned int PIA_ADDR = 0xD000;

const unsigned int XAML = 0x24;
const unsigned int XAMH = 0x25;
const unsigned int STL =  0x26;
const unsigned int STH =  0x27;
const unsigned int L = 0x28;
const unsigned int H = 0x29;
const unsigned int YSAV = 0x2A;
const unsigned int MODE = 0x2B;

const unsigned int IN = 0x200;

const int RAM_BANK_1_SIZE=4096;
const int RAM_BANK_2_SIZE=4096;
unsigned char RAM_BANK_1[RAM_BANK_1_SIZE];

unsigned int address;
unsigned char bus_data=0;
int rw_state = HIGH;

const unsigned int KBD_ADDR = 0xD010;
const unsigned int KBDCR_ADDR = 0xD011;
const unsigned int DSP_ADDR = 0xD012;
const unsigned int DSPCR_ADDR = 0xD013;


unsigned char KBD = 0;
unsigned char KBDCR=0;
unsigned char DSP = 0;
unsigned char DSPCR = 0;

void setupAddressPins() {
  for (int i = 0; i < NUM_ADDR_PINS; ++i) {
    pinMode (ADDRESS_PINS[i], INPUT);
  }
}

void busMode(int mode) {
  for (int i = 0; i < NUM_DATA_PINS; ++i) {
    pinMode (DATA_PINS[i], mode);
  }
}

void readAddress() {
  address = 0;
  for (int i = 0; i < NUM_ADDR_PINS; ++i)
  {
    address = address << 1;
    address += (digitalRead (ADDRESS_PINS[NUM_ADDR_PINS-i-1]) == HIGH)?1:0;
  }
}

void readData() {
  bus_data = 0;
  for (int i = 0; i < NUM_DATA_PINS; ++i)
  {
    bus_data = bus_data << 1;
    bus_data += (digitalRead (DATA_PINS[NUM_DATA_PINS-i-1]) == HIGH)?1:0;
  }
}

void handleRWState() {
  int curent_rw_state=digitalRead(RW_PIN);
  if (rw_state != curent_rw_state) {
    rw_state=curent_rw_state;
    if (rw_state) {
      busMode(OUTPUT);
    } else {
      busMode(INPUT);
    }
  }
}

void byteToDataBus(unsigned char data) {
  for (int i = 0; i < NUM_DATA_PINS; i++) {
    digitalWrite(DATA_PINS[i], CHECK_BIT(data, i));
  }
}

void readFromDataBus() {
  readData();

  switch (address >> 12) {
    case 0x0:
      RAM_BANK_1[address-RAM_BANK1_ADDR]=bus_data;
      break;
    case 0xE:
      //RAM_BANK_2[address-RAM_BANK2_ADDR]=bus_data;
      RAM_BANK_2[address-RAM_BANK2_ADDR]=bus_data;
      break;
    case 0xD:
      // 6821
      switch (address) {
        case KBD_ADDR:
          KBD=bus_data;
          break;
        case KBDCR_ADDR:
          KBDCR=bus_data;
          break;
        case DSP_ADDR:
          DSP = bus_data;
          if (DSP == 0x8D) {
            Serial.write('\r');
            Serial.write('\n');
          } else {
            Serial.write(DSP & 0x7F);
          }
          bitClear(DSP, 7);
          break;
        case DSPCR_ADDR:
          DSPCR=bus_data;
          break;
      }
      break;
  }
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
      // 6821
      switch (address) {
        case KBD_ADDR:
          val=KBD;
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
      }
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
        return;
        break;
      case 0xD:
        tempKBD = 0x0D;
        break;
    }

    KBD = tempKBD;
    bitSet(KBD, 7);
    bitSet(KBDCR, 7);

  }
}

void setup() {
  pinMode (CLOCK_PIN, OUTPUT);
  pinMode (RW_PIN, INPUT);
  setupAddressPins();
  busMode(OUTPUT);
  Serial.begin (115200);

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
  Serial.println("----------------------------");

}

void loop () {

  // LOW CLOCK
  digitalWrite(CLOCK_PIN, LOW);
  delayMicroseconds(CLOCK_DELAY);

  // RW STATE
  handleRWState();

  // HIGH CLOCK
  digitalWrite (CLOCK_PIN, HIGH);
  delayMicroseconds(CLOCK_DELAY);

  // ALWAYS READ ADDR
  readAddress();

  // READ OR WRITE TO BUS?
  if (rw_state) {
    writeToDataBus();
  } else {
    readFromDataBus();
  }

  handleKeyboard();
}

// WOZ TEST
// 0:A9 9 AA 20 EF FF E8 8A 4C 2 0

// HELLO WORLD
// 280:A2 C BD 8B 2 20 EF FF CA D0 F7 60 8D C4 CC D2 CF D7 A0 CF CC CC C5 C8
