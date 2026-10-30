//
///* CommandReg: 0x01
//ComIrqReg: 0x04  command Register
//ErrorReg: 0x06   Error Register
//FIFODataReg: 0x09 FIFO Data register for the data to be send or receive
//FIFOLevelReg: 0x0A   FIFO Level Register Tells the number of bytes arrived in FIFO  buffer
//BitFramingReg: 0x0D  Bit Framing Register for Bit Frame to 7 or 8 bit as required
//ModeReg: 0x11
//TxControlReg: 0x14
//TxASKReg: 0x15
//VersionReg: 0x37 */
//
#include "main.h"
#include<stdint.h>
#include "RC522.h"
#include <stdio.h>
#include <string.h>
//
user_t users[MAX_USERS] ;
uint8_t user_count =0;



#define MAX_USERS  10
#define NAME_LEN   16
//
//
//
//
#define CS_PORT GPIOB
#define CS_PIN GPIO_PIN_3
extern SPI_HandleTypeDef hspi2 ;
extern UART_HandleTypeDef huart1 ;
//
//
void reg_write (uint8_t reg, uint8_t value)
    {

	uint8_t buff[] = { (uint8_t)((reg << 1) & 0x7E), (uint8_t)value }; // 7th bit 1=read, 0=write here we are writing to register so we clear the bit 7  with the use of 0x7E and bit 0 is reserved 0 so it should stay 0

        HAL_GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_RESET) ;
        HAL_SPI_Transmit(&hspi2,buff,2, HAL_MAX_DELAY);
        HAL_GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_SET) ;
    }
//
uint8_t reg_read(uint8_t reg)
{
	uint8_t tx_buff[2] = { (uint8_t)(((reg << 1) & 0x7E) | 0x80), 0x00 }; // Left shift <<1 as RC522 accepts address on bit 1 to 6. Bit 0 is reserved and Bit 7 is Read or write bit so register address is left shifted by <<1
   uint8_t  rx_buff[2] = {0};

    HAL_GPIO_WritePin(CS_PORT ,CS_PIN , GPIO_PIN_RESET);
    HAL_SPI_TransmitReceive(&hspi2,tx_buff,rx_buff,2,100);
    HAL_GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_SET);
    return rx_buff[1];

}
//
 void init_rc522(void)
{
  reg_write(0x01, 0x0F);   // SoftReset, writing to CommandReg resets the MFRC522
   HAL_Delay(50);              // wait for reset to finish
//
 reg_write(0x15, 0x40);   // TxASKReg: 100% ASK modulation, set Bit 6 to Force a 100% ASK modulation indepenent of the MOdGsPReg register setting
   reg_write(0x11, 0x3D);   // ModeReg: CRC(cyclic redundancy check) preset 0x6363
//
  reg_write(0x14, reg_read(0x14) | 0x03);   // antenna ON, other bits unchanged, TxControlReg set bit 0 and 1 to turn on antenna while keeping other bits unchanged
}
//
static uint8_t wait_for_reply(void)
{
    uint8_t  irq;
    uint16_t guard = 2000;

    do {
        irq = reg_read(0x04);                          // ComIrqReg to check if the reciever has deteccted the end of a valid data stream bit 5 is set when detected
    } while (--guard && !(irq & 0x31));                // RxIRq | IdleIRq | TimerIRq

    reg_write(0x0D, 0x00);                             // clear StartSend

    if (guard == 0 || !(irq & 0x20)) return 1;         // no reply (RxIRq not set)
    if (reg_read(0x06) & 0x1B)       return 2;         // ErrorReg: ovfl/coll/parity/protocol
    return 0;
}


uint8_t Send_REQA(uint8_t atqa[2])
{
    reg_write(0x01, 0x00);     // command register - Idle
    reg_write(0x04, 0x7F);     // ComIrqReg Register-clear old flags
    reg_write(0x0A, 0x80);     // FIFOLevelReg- Clears the internal FIFO buffer/flush FIFO

    reg_write(0x09, 0x26);     // FIFODataReg register - REQA into FIFO
    reg_write(0x0D, 0x07);     // BitFramingregister - REQA command is 7 bit /TxLastBits = 7
    reg_write(0x01, 0x0C);     // Command Register transmits data from FIFO buffer to antenna and automatically activates the reciever  after transmission. Transceive
    reg_write(0x0D, 0x87);     // BitFramingRegister start the transmission of data valid only with transcieve command. StartSend + 7 bits

    uint8_t st = wait_for_reply();
    if (st != 0) return st;

    if ((reg_read(0x0A) & 0x7F) != 2) return 3;        // expected 2 bytes
    atqa[0] = reg_read(0x09);
    atqa[1] = reg_read(0x09); // REQA will give two bytes ATQA value if 0x04 0x00> 4 bytes or 0x44 0x00>  7 bytes storing it at atqa[0] and atqa[1]
    return 0;
}

uint8_t Send_Anticollision(uint8_t sel ,uint8_t uid[5])
{
    reg_write(0x01, 0x00); /*Sets the chip to Idle, which stops any command that is running. It is a clean starting point.*/
    reg_write(0x04, 0x7F); /*Clears the flags left over from before. 0x7F is 0111 1111. Bit 7 (Set1) is 0, so writing 1 to bits 6 to 0 clears those flags. Otherwise an old RxIRq could end your wait loop too early.*/
    reg_write(0x0A, 0x80); /*set bit 7 (FlushBuffer), which empties the FIFO. Old bytes can’t mix with the new ones.*/

    reg_write(0x09, sel); /*Puts the byte 0x93 into the FIFO. This is the “SEL” byte from ISO 14443-3 meaning anticollision, cascade level 1.*/
    reg_write(0x09, 0x20);  /* Puts 0x20 into the FIFO, after 0x93. 0x20 means “send me your full UID”. The FIFO now holds 93 20.*/
    reg_write(0x0D, 0x00);  /* Bits 2:0 (TxLastBits) = 0 means all 8 bits of the last byte are sent. REQA used 7 here, but anticollision uses full bytes. Bit 7 (StartSend) is 0, so nothing is sent yet.*/
    reg_write(0x01, 0x0C); /* Sets the command to Transceive (0x0C). The chip is now ready to send what is in the FIFO and then switch to receiving. It still waits for StartSend.*/
    reg_write(0x0D, 0x80); /*Sets bit 7 (StartSend). The chip transmits 93 20 to the card and then listens for the reply. The 5-byte reply goes into the FIFO.*/
    uint8_t st = wait_for_reply();
        if (st != 0) return st;

        if ((reg_read(0x0A) & 0x7F) != 5) return 3;        // expected 5 bytes
        for (uint8_t i = 0; i < 5; i++)
            uid[i] = reg_read(0x09); // card replies with its uid here and we store it here.

        /*if ((uid[0] ^ uid[1] ^ uid[2] ^ uid[3]) ^ uid[4] ^ uid[5]^ uid[6] != uid[7]) return 4; */   // Before sending the card XOR its 4ID bytes toghether and sends that result as the 5th byte we check that here
        return 0;

}
uint8_t Send_Select(uint8_t sel, uint8_t uid[5], uint8_t *sak)
{
    reg_write(0x01, 0x00);     // Idle
    reg_write(0x04, 0x7F);     // clear old flags
    reg_write(0x0A, 0x80);     // flush FIFO

    reg_write(0x09, sel);      // 0x93 or 0x95
    reg_write(0x09, 0x70);     // 0x70 = "I am selecting you", 7 bytes follow
    for (uint8_t i = 0; i < 5; i++)
        reg_write(0x09, uid[i]);   // the 4 ID bytes + BCC you got from anticollision

    reg_write(0x12, 0x80);     // TxModeReg: TxCRCEn, the chip adds the 2 CRC bytes itself
    reg_write(0x0D, 0x00);     // all 8 bits of last byte
    reg_write(0x01, 0x0C);     // Transceive
    reg_write(0x0D, 0x80);     // StartSend

    uint8_t st = wait_for_reply();
    reg_write(0x12, 0x00);     // CRC off again, REQA and anticollision need it off
    if (st != 0) return st;

    if ((reg_read(0x0A) & 0x7F) != 3) return 3;    // reply = SAK + 2 CRC bytes
    *sak = reg_read(0x09);
    return 0;
}

// id_size = 0;







/*Returns the index in the table, or -1 if the card is not stored */
 int8_t find_user(const uint8_t *id, uint8_t size)
{
    for (uint8_t i = 0; i < user_count; i++)
        if (users[i].size == size && memcmp(users[i].id, id, size) == 0)
            return (int8_t)i;
    return -1;
}

/* Returns 0 = added, -1 = table full */
int8_t add_user(const uint8_t *id, uint8_t size, const char *name)
{
    if (user_count >= MAX_USERS) return -1;

    users[user_count].size = size;
    memcpy(users[user_count].id, id, size);
    strncpy(users[user_count].name, name, NAME_LEN - 1);
    users[user_count].name[NAME_LEN - 1] = '\0';
    user_count++;
    return 0;
}

void uart_print(const char *s)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)s, strlen(s), 100);
}

/* Reads typed characters until Enter. Returns the number of characters,
   or 0 if nothing was typed within timeout_ms. */
uint8_t uart_read_line(char *buf, uint8_t max, uint32_t timeout_ms)
{
    uint8_t  n = 0;
    uint8_t  c;
    uint32_t start = HAL_GetTick();

    while (HAL_GetTick() - start < timeout_ms)
    {
        if (HAL_UART_Receive(&huart1, &c, 1, 10) == HAL_OK)
        {
            if (c == '\r' || c == '\n')
            {
                if (n > 0) break;          // Enter pressed after a name
                continue;                  // ignore Enter with nothing typed
            }
            if (n < max - 1)
            {
                buf[n++] = (char)c;
                HAL_UART_Transmit(&huart1, &c, 1, 10);   // echo what you type
            }
        }
    }
    buf[n] = '\0';
    return n;
}



