#Apple 1 REPLICA

Raw but full working implementation of Apple 1 replica, based on real W6502C MPU (modern 6502) and Arduino Due 4KB+4KB RAM.

Arduino Due just listen and write/write the 6502 pins (RW/Address/Data Bus), emulate the PIA (6821) Keyboard & Display + handle the 6502 clock.

The project was originally implemented on an Arduino UNO that is in any case limited in terms of RAM and also less performant. Arduino Due is a perfect fit instead.

The project is build over platform.io but you should be able to compile it in the Arduino standard IDE too, without any change.

## Serial client recommended settings (I'm using Serial under OSX):
- BAUD RATE: 115200
- Data Bits: 8
- Parity: None
- Stop Bits: 1
- Flow Control: (everything OFF)
- Emulation XTerm
- Send Mode: Immediate
- Return Key: CR
- Delete Key: DEL
- Text Pacing: Normal
- Uppercase Typed Characters: ON

## 6502 to Arduino Pin schema:

                        W65C02S <->  Arduino Due

        3.3v      GND                          10uf ---- GND
         |        |       +------\/------+      |
         |        +----  1| VPB     /RES |40 ---+------- +RST BTN- ---- GND
         +--- 3k3 -----  2| RDY    PHI2O |39
         |               3| PHI1O    SOB |38
         +--- 3k3 -----  4| IRQ     PHI2 |37 -------- 52
         |               5| MLB       BE |36---3k3--------3.3v
         +--- 3k3 -----  6| /NMI      NC |35
         |               7| SYNC     R/W |34 -------- 53
         +-------------  8| VDD       D0 |33 -------- 30
           44 ---------  9| A0        D1 |32 -------- 31
           45 --------- 10| A1        D2 |31 -------- 32
            2 --------- 11| A2        D3 |30 -------- 33
            3 --------- 12| A3        D4 |29 -------- 34
            4 --------- 13| A4        D5 |28 -------- 35
            5 --------- 14| A5        D6 |27 -------- 36
            6 --------- 15| A6        D7 |26 -------- 37
            7 --------- 16| A7       A15 |25 -------- 47
            8 --------- 17| A8       A14 |24 -------- 46
            9 --------- 18| A9       A13 |23 -------- 13
           10 --------- 19| A10      A12 |22 -------- 12
           11 --------- 20| A11      VSS |21 ---+
                          +--------------+      |
                                               GND

    Note: You may want to put a 100Uf capacitor near the 3.3v & GND lines too.

## Auto loaded Program & extended Memory
The Arduino sketch (main.cpp) automatically load the original Apple 1 Basic in the extended RAM at E000 address (was loaded via Tape in the original version).

Same is done with a Program included in AUTLOAD variable (look into programs.h).
You can comment & uncomment other programs as needed to get them auto loaded in the related RAM location.
First word in the source is the location in RAM when the program should be loaded. This is not part of any original logic, it's just a convenient way to fast load programs without manually inserting them via Woz Monitor.

You can easily comment out this logic or extend it as needed.

## MASM version of WAZ MONITOR (Official Apple 1 ROM)
Inside ASM folder you can find the woz_monitor_masm.asm source listing. This is the original ROM listing found in the apple 1 manual converted to be compatible with masm.

If you are under OSX - you can install MASM via homebrew and use it to compile any asm source included the one above.

## Apple 1 Address space
   $0000-$0FFF ------------- 4KB RAM
      $0024-$002B ---------- WOZ MONITOR STORE (better to not overwrite)
      $0200-$027F ---------- INPUT BUFFER (as the one above)
   $D010-$D013 ------------- PIA (6821) [KBD & DSP]
   $E000-$EFFF ------------- 4KB extended RAM (Usually for BASIC prog)
   $FF00-$FFFF ------------- 256 Bytes ROM (crazy! with just 2 bytes unused.)


## Resources
- http://dave.cheney.net/2014/12/26/make-your-own-apple-1-replica (I used this as a base reference)
- https://coronax.wordpress.com/projects/project65/ (Good to understand by steps how a 6502 works)
- http://archive.computerhistory.org/resources/text/Apple/Apple.AppleI.1976.102646518.pdf (apple 1 official manual)
- https://en.wikipedia.org/wiki/WDC_65C02
- http://www.westerndesigncenter.com/wdc/w65c21-chip.cfm
