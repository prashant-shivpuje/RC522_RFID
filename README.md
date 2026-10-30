**RC522 RFID Card Reader with STM32F103**

An RFID card reader built on an STM32F103C8T6 (Blue Pill) and an MFRC522 (RC522) module. The RC522 driver is written from scratch in Embedded C using the STM32 HAL library, with direct register read/write over SPI.

When a card is tapped, the firmware reads its UID and prints it on a serial terminal. Registered cards are greeted by name. Unknown cards can be registered by typing a name over UART.

Features : 
- RC522 communication over SPI2 with register-level read/write functions
- ISO 14443A card detection: REQA, anti-collision and select
- Handles both 4-byte and 7-byte UIDs (cascade levels 1 and 2)
- Prints the card UID over UART (USART1)
- Matches the UID against a list of registered users and shows Welcome, <name>
- For unknown cards, asks for a name over UART (10 second timeout) and adds the user
- Prints the date and time on a card tap using the STM32 internal RTC

Hardware :
- Microcontroller	STM32F103C8T6 (ARM Cortex-M3, "Blue Pill")
- RFID reader	MFRC522 (RC522) module, 13.56 MHz
- Cards	MIFARE-type tags/cards
- Serial terminal USB-to-TTL

Wiring :

The RC522 runs at 3.3 V. Do not connect it to 5 V.

RC522 pin	STM32F103 pin
SDA         (SS / CS)	GPIO output of your choice (chip select)
SCK	      PB13 (SPI2_SCK)
MOSI	      PB15 (SPI2_MOSI)
MISO	      PB14 (SPI2_MISO)
RST	      GPIO output of your choice (optional hard reset)
3.3V	      3.3 V
GND	      GND


Software and tools :
- Embedded C
- STM32CubeIDE and STM32CubeMX
- Serial terminal (PuTTY, Tera Term, etc.). I used puTTY

Build and run
Clone this repository:
bash
   git clone https://github.com/prashant-shivpuje/RC522_RFID.git
Open STM32CubeIDE and choose File -> Open Projects from File System, then select the cloned folder.
Build the project and flash it to the Blue Pill (ST-Link).
Connect the serial adapter to USART1 and open a terminal at the baud rate set in the .ioc file (8 data bits, no parity, 1 stop bit).
Tap a card on the RC522.
How it works
Card detection: The firmware sends a REQA command. A card in range replies with its ATQA.
UID read: Anti-collision returns the UID. If the first byte is 0x88 (cascade tag), the card has a 7-byte UID, so the card is selected and a second anti-collision cycle reads the remaining bytes.
User lookup: The UID is compared against the stored user list.
Known card: The terminal shows a welcome message with the user's name.
Unknown card: The terminal asks for a name. If one is entered within 10 seconds, the card is added to the user list.
Example serial output
Card tapped: 24 8C F2 06
Unknown card. Type a name and press Enter to add it (10 s, or wait to skip): Rahul
Card tapped: 24 8C F2 06
Welcome, Rahul
Date: 30/10/2026  Time: 15:30:12
Known limitations
The user list is kept in RAM, so registered users are lost on reset.
The Blue Pill has no battery on VBAT, so the RTC resets on every power-up and the time must be set at startup. A DS3231 module, or a coin cell wired to VBAT, would keep the time through power loss.
Cards are identified by UID only. UIDs can be cloned, so this is not suitable for real security applications.
Possible improvements
Store registered users in flash so they survive a reset
Keep time across power loss with a DS3231 or a VBAT battery
Log taps with timestamps
Add an LCD display and a relay/buzzer for door-lock style use
Read and write MIFARE data blocks with authentication

Author :
Prashant Shivpuje
