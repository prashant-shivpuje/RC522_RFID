#ifndef RC522_H
#define RC522_H

#include "main.h"
#include <stdint.h>

#define RC522_CS_PORT  GPIOB
#define RC522_CS_PIN   GPIO_PIN_3

extern SPI_HandleTypeDef hspi1;

#define MAX_USERS  10
#define NAME_LEN   16

typedef struct {
    uint8_t size;           // 4 or 7
    uint8_t id[7];
    char name[NAME_LEN];
} user_t;
 extern user_t  users[MAX_USERS];
extern uint8_t user_count ;

void    reg_write(uint8_t reg, uint8_t value);
uint8_t reg_read(uint8_t reg);
void    init_rc522(void);
uint8_t Send_REQA(uint8_t atqa[2]);
uint8_t Send_Anticollision(uint8_t sel, uint8_t uid[5]);
uint8_t Send_Select(uint8_t sel, uint8_t uid[5], uint8_t *sak);
int8_t find_user(const uint8_t *id, uint8_t size);
int8_t add_user(const uint8_t *id, uint8_t size, const char *name);
void uart_print(const char *s);
uint8_t uart_read_line(char *buf, uint8_t max, uint32_t timeout_ms);
#endif
