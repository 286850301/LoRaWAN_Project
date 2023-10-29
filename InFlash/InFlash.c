#include "InFlash.h"
#include <stdio.h>


uint8_t EraseFlash(uint32_t page, uint32_t end_Add)
{
    uint32_t UserStartSector;
    uint32_t SectorError;
    FLASH_EraseInitTypeDef pEraseInit;
 
    // �����ڲ�FLASH
    HAL_FLASH_Unlock();
 
    // ��ȡ��ʼ��ַ������
    pEraseInit.TypeErase = FLASH_TYPEERASE_PAGES;
		pEraseInit.Page = page;
		pEraseInit.Banks = FLASH_BANK_2;
		pEraseInit.NbPages = 1;
 
    if (HAL_FLASHEx_Erase(&pEraseInit, &SectorError) != HAL_OK)
        return (HAL_ERROR);

    return (HAL_OK);
}

uint8_t WriteFlash(uint8_t *src, uint8_t *dest, uint32_t Len)
{
		uint32_t i = 0;

		if (EraseFlash((uint32_t)dest, (uint32_t)(dest + Len)) == HAL_OK)
		{
			for(i = 0; i < Len; i+=8)
			{
					// ��Դ��ѹ��2.7-3.6V��Χʱ֧��FLASHд�����
					if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, (uint32_t)(dest+i), *(uint64_t*)(src+i)) == HAL_OK)
					{
							// ���д���Ƿ���Ч
							if(*(uint32_t *)(src + i) != *(uint32_t*)(dest+i))
							{
									// У��ʧ�ܣ����ش������2
									return 2;
							}
					}
					else
					{
							// д��ʧ�ܣ����ش������1
							return HAL_ERROR;
					}
			}
			return HAL_OK;
		}
		else
			return HAL_ERROR;
}

uint8_t ReadFlash (uint8_t *src, uint8_t *dest, uint32_t Len)
{
    uint32_t i = 0;
 
    for(i = 0; i < Len; i++)
        dest[i] = *(__IO uint8_t*)(src+i);
    return HAL_OK;
}
