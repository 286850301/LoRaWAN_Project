#ifndef __INFLASH_H
#define __INFLASH_H

#include "stm32l4xx_hal.h"

#define ADDR_FLASH_PAGE_256 0x08080000UL

uint8_t EraseFlash(uint32_t page, uint32_t end_Add);				// FLASH����
uint8_t WriteFlash(uint8_t *src, uint8_t *dest, uint32_t Len);	// FLASHд��
uint8_t ReadFlash (uint8_t *src, uint8_t *dest, uint32_t Len);	// FLASH��ȡ


#endif /* __INFLASH_H */
