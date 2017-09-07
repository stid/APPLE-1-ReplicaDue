#Apple 1 REPLICA
Raw but full working implementation of Apple 1 replica (4KB+4KB RAM), based on real W65C02S MPU (modern 6502) and Arduino Due as IO interface.

Arduino Due just listen and read/write the 6502 pins (RW/Address/Data Bus), emulate the PIA (6821) Keyboard & Display, handle the 6502 clock and provide RAM / ROM emulation.

The project was originally implemented on an Arduino UNO that is in any case limited in terms of RAM and also less performant. Arduino Due is a perfect fit in this case.

The project is build over Platformio (http://platformio.org/frameworks) but you should be able to compile the sketch in the Arduino standard IDE too, without any change.

## How it works
A 65C02S (modern version of the original 6502) is wired on a breadboard. The 6502 is a very simple kind of CPU in terms of I/O and the modern version allow us to suspend the clock at any time in LOW / HIGH state.

There are 16 Address Pins (A0-A15) each mapping a bit (HIGH/LOW) of an address word. This is how 6502 tell us which address he want to read or write to (RAM/ROM, IO dedicated address spaces). The state of this pins may change at every clock cycle.

Other 8 pins will carry or expect the 1 Byte data to be stored or read to / from the address above.

The CPU tell the external world if the data is in read or write state via R/W pin (HIGH=Expect data IN, LOW=Send data OUT).

That's it. This is all you need to interact with a 6502. The CPU expect a clock signal (HIGH/LOW) on pin 37 (PHI2). The modern version of 6502 is able to suspend any activity for an unlimited period of time during a clock cycle, both in LOW or HIGH state (was a little bit more hard with the original CPU). This make a perfect fit to let us drive the CPU via something different from a crystal, in our case, an Arduino, debug any single clock cycle and keep things in sync.

Knowing the address space where the Apple 1 was mapping the different IO  (Ram / ROM / KEYB / DSP), we can simulate the external interfaces (in fact a PIA 6821) and send back & forth as needed the related data via 6502 data bus. That's it.

There are in fact 4 bytes dedicated to the PIA interface at $D010-$D013, two of them dedicated to the keyboard I/O and two of them dedicated to the Display. The Arduino sketch just hook on this addresses and react as needed (printing out to the serial monitor or sending in the pressed key), simulating the external interfaces.

The WOZ monitor asm source and the original apple 1 operation manual are two very good resources to understand how all this works.

## 6502 to Arduino Pin schema:

                        W65C02S <--->  Arduino Due

        3.3v      GND                          10uf ---- GND
         |        |       +------\/------+      |
         |        +----  1| VPB     /RES |40 ---+------- +RST BTN- ---- GND
         +--- 3k3 -----  2| RDY    PHI2O |39
         |               3| PHI1O    SOB |38
         +--- 3k3 -----  4| IRQ     PHI2 |37 -------- 52
         |               5| MLB       BE |36---3k3--------3.3v
         +--- 3k3 -----  6| /NMI      NC |35
         |               7| SYNC     R/W |34 -------- 53
         +-------------  8| VDD       D0 |33 -------- 33
           44 ---------  9| A0        D1 |32 -------- 34
           45 --------- 10| A1        D2 |31 -------- 35
            2 --------- 11| A2        D3 |30 -------- 36
            3 --------- 12| A3        D4 |29 -------- 37
            4 --------- 13| A4        D5 |28 -------- 38
            5 --------- 14| A5        D6 |27 -------- 39
            6 --------- 15| A6        D7 |26 -------- 40
            7 --------- 16| A7       A15 |25 -------- 47
            8 --------- 17| A8       A14 |24 -------- 46
            9 --------- 18| A9       A13 |23 -------- 13
           10 --------- 19| A10      A12 |22 -------- 12
           11 --------- 20| A11      VSS |21 ---+
                          +--------------+      |
                                               GND

    CLOCK_DELAY: A0 - you should connect a potentiometer to A0, this will let you manually sed the clock delay of the 6502.

    Note: You may want to put a 100Uf capacitor near the 3.3v & GND lines too.


## Serial client recommended settings:
You should be able to use the standard Serial Monitor in the Arduino IDE or or Platformio (Atom) or any other basic serial client.

    - BAUD RATE: 115200
    - Data Bits: 8
    - Parity: None
    - Stop Bits: 1
    - Flow Control: (everything OFF)
    - Emulation XTerm (almost all other common protocols should work)
    - Send Mode: Immediate
    - Return Key: CR
    - Delete Key: DEL
    - Text Pacing: Normal
    - Uppercase Typed Characters: ON

## Auto loaded Program & extended Memory
The Arduino sketch (main.cpp) automatically load the original Apple 1 Basic in the extended RAM at E000 address (was loaded via Tape in the original version).

Same is done with a Program included in AUTLOAD array (look into programs.h).
You can comment & uncomment other programs as needed to swap them and get them auto loaded in the related RAM location.
First word in the AUTLOAD is the location in RAM when the program need to be loaded. This is not part of any original logic, it's just a convenient way to fast load programs without manually inserting them via Woz Monitor.

You can easily comment out this logic or extend it as needed.

## MASM version of WOZ MONITOR (Official Apple 1 ROM)
Inside ASM folder you can find the woz_monitor_masm.asm source listing. This is the original ROM listing found in the apple 1 manual converted to be compatible with MASM and loaded in Apple 1 ROM.

The other sources you find there may be useful to debug the 6502 basic operations as you wire it.

If you are under OSX - you can install MASM via HomeBrew and use it to compile any asm source included the one above:

    brew install masm

## Apple 1 Address Space
          $0000-$0FFF ------------- 4KB Standard RAM
             $0024-$002B ---------- WOZ MONITOR STORE (better to not overwrite it)
             $0200-$027F ---------- INPUT BUFFER (as the one above)
          $D010-$D013 ------------- PIA (6821) [KBD & DSP]
          $E000-$EFFF ------------- 4KB extended RAM (Usually for BASIC prog)
          $FF00-$FFFF ------------- 256 Bytes ROM (crazy! with just 2 bytes unused.)

## Resources
- http://dave.cheney.net/2014/12/26/make-your-own-apple-1-replica (I used this as a base reference)
- https://coronax.wordpress.com/projects/project65/
- http://archive.computerhistory.org/resources/text/Apple/Apple.AppleI.1976.102646518.pdf (apple 1 official manual)
- https://en.wikipedia.org/wiki/WDC_65C02
- http://www.westerndesigncenter.com/wdc/w65c21-chip.cfm