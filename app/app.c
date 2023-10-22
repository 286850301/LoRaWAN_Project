#include <string.h>
#include <stdlib.h>
#include "app.h"
#include "usart.h"
#include "gpio.h"
#include "lorawan_node_driver.h"
#include "hdc1000.h"
#include "opt3001.h"
#include "MPL3115.h"
#include "sensors_test.h"
#include "common.h"
#include "ST7789v.h"

extern DEVICE_MODE_T device_mode;
extern DEVICE_MODE_T *Device_Mode_str;
down_list_t *pphead = NULL;

uint16_t Tim3_Counter = 0;
uint8_t *DevEui = "009569000000F554";

extern uint8_t Tim3_Sensors_Delay_Secend = 3;
extern uint8_t SenSors_Data_Buf_Num = 10;
uint8_t SensorsCnt = 0;
SensorsData_t SensorsData; // �����ṹ��ʵ��

//-----------------Users application--------------------------
void LoRaWAN_Func_Process(void)
{
    static DEVICE_MODE_T dev_stat = NO_MODE;

    uint16_t temper = 0;

    if ((uint8_t)device_mode != PRO_TRAINING_MODE)
    {
        free(SensorsData.Data.Lux_OPT3001);
        free(SensorsData.Data.Pressure_MPL3115);
        free(SensorsData.Data.Temper_HDC1000);
        free(SensorsData.Data.Humidi_HDC1000);
    }

    switch ((uint8_t)device_mode)
    {
    /* ָ��ģʽ */
    case CMD_CONFIG_MODE:
    {
        /* �������command Configuration function, �����if���,ִֻ��һ�� */
        if (dev_stat != CMD_CONFIG_MODE)
        {
            dev_stat = CMD_CONFIG_MODE;
            debug_printf("\r\n[Command Mode]\r\n");
            LCD_Clear(WHITE);
            LCD_ShowString(5, 5, "[Command Mode]", BLUE);

            nodeGpioConfig(wake, wakeup);
            nodeGpioConfig(mode, command);
        }
        /* �ȴ�usart2�����ж� */
        if (UART_TO_PC_RECEIVE_FLAG)
        {
            UART_TO_PC_RECEIVE_FLAG = 0;
            lpusart1_send_data(UART_TO_PC_RECEIVE_BUFFER, UART_TO_PC_RECEIVE_LENGTH);
        }
        /* �ȴ�lpuart1�����ж� */
        if (UART_TO_LRM_RECEIVE_FLAG)
        {
            UART_TO_LRM_RECEIVE_FLAG = 0;
            usart2_send_data(UART_TO_LRM_RECEIVE_BUFFER, UART_TO_LRM_RECEIVE_LENGTH);
        }
    }
    break;

    /* ͸��ģʽ */
    case DATA_TRANSPORT_MODE:
    {
        /* �������data transport function,�����if���,ִֻ��һ�� */
        if (dev_stat != DATA_TRANSPORT_MODE)
        {
            dev_stat = DATA_TRANSPORT_MODE;
            debug_printf("\r\n[Transperant Mode]\r\n");
            LCD_Clear(WHITE);
            LCD_ShowString(5, 5, "[Transperant Mode]", BLUE);

            /* ģ�������ж� */
            if (nodeJoinNet(JOIN_TIME_120_SEC) == false)
            {
                return;
            }

            temper = HDC1000_Read_Temper() / 1000;

            nodeDataCommunicate((uint8_t *)&temper, sizeof(temper), &pphead);
        }

        /* �ȴ�usart2�����ж� */
        if (UART_TO_PC_RECEIVE_FLAG && GET_BUSY_LEVEL) // Ensure BUSY is high before sending data
        {
            UART_TO_PC_RECEIVE_FLAG = 0;
            nodeDataCommunicate((uint8_t *)UART_TO_PC_RECEIVE_BUFFER, UART_TO_PC_RECEIVE_LENGTH, &pphead);
        }

        /* ���ģ����æ, ����������Ч��������������Ϣ */
        else if (UART_TO_PC_RECEIVE_FLAG && (GET_BUSY_LEVEL == 0))
        {
            UART_TO_PC_RECEIVE_FLAG = 0;
            debug_printf("-. Warning: Don't send data now! Module is busy!\r\n");
        }

        /* �ȴ�lpuart1�����ж� */
        if (UART_TO_LRM_RECEIVE_FLAG)
        {
            UART_TO_LRM_RECEIVE_FLAG = 0;
            usart2_send_data(UART_TO_LRM_RECEIVE_BUFFER, UART_TO_LRM_RECEIVE_LENGTH);
        }
    }
    break;

    /*����ģʽ*/
    case PRO_TRAINING_MODE:
    {
        /* �������Class C��ƽ̨���ݲɼ�ģʽ, �����if���,ִֻ��һ�� */
        if (dev_stat != PRO_TRAINING_MODE)
        {
            dev_stat = PRO_TRAINING_MODE;
            debug_printf("\r\n[Project Mode]\r\n");
            LCD_Clear(WHITE);
            LCD_ShowString(5, 5, "[Project Mode]", BLUE);
            LCD_ShowString(5, 5 + (1 * 16), "[DevEui]:", BLUE);
            LCD_ShowString(5 + (9 * 8), 5 + (1 * 16), DevEui, BLUE);
        }

        /* ���ʵ�����λ�� */
        if (Tim3_Counter == Tim3_Sensors_Delay_Secend * 100)
        {
            if (SensorsCnt == 0)
            {
                // ��ʼ���ṹ��
                memset(&SensorsData, 0, sizeof(SensorsData));
                // Ϊ��������ڴ�
                SensorsData.Data.Lux_OPT3001 = (float *)malloc(SenSors_Data_Buf_Num * sizeof(float));
                SensorsData.Data.Pressure_MPL3115 = (float *)malloc(SenSors_Data_Buf_Num * sizeof(float));
                SensorsData.Data.Temper_HDC1000 = (uint32_t *)malloc(SenSors_Data_Buf_Num * sizeof(uint32_t));
                SensorsData.Data.Humidi_HDC1000 = (uint32_t *)malloc(SenSors_Data_Buf_Num * sizeof(uint32_t));
                // ��ʼ���ṹ�岿�ֱ���
                SensorsData.Min.Lux_OPT3001 = 0x7fffffff;
                SensorsData.Min.Pressure_MPL3115 = 0x7fffffff;
                SensorsData.Min.Temper_HDC1000 = 0xffff;
                SensorsData.Min.Humidi_HDC1000 = 0xffff;
            }

            SensorsData.Data.Lux_OPT3001[SensorsCnt] = OPT3001_Get_Lux();
            SensorsData.Data.Pressure_MPL3115[SensorsCnt] = MPL3115_ReadPressure();
            SensorsData.Data.Temper_HDC1000[SensorsCnt] = HDC1000_Read_Temper();
            SensorsData.Data.Humidi_HDC1000[SensorsCnt] = HDC1000_Read_Humidi();
            // �����ֵ
            // clang-format off
            SensorsData.Data.Lux_OPT3001[SensorsCnt]       >= SensorsData.Max.Lux_OPT3001 
                                                            ?  SensorsData.Max.Lux_OPT3001 = SensorsData.Data.Lux_OPT3001[SensorsCnt] 
                                                            :  NULL; 
            SensorsData.Data.Pressure_MPL3115[SensorsCnt]  >= SensorsData.Max.Pressure_MPL3115 
                                                            ?  SensorsData.Max.Pressure_MPL3115 = SensorsData.Data.Pressure_MPL3115[SensorsCnt] 
                                                            :  NULL; 
            SensorsData.Data.Temper_HDC1000[SensorsCnt]    >= SensorsData.Max.Temper_HDC1000 
                                                            ?  SensorsData.Max.Temper_HDC1000 = SensorsData.Data.Temper_HDC1000[SensorsCnt] 
                                                            :  NULL; 
            SensorsData.Data.Humidi_HDC1000[SensorsCnt]    >= SensorsData.Max.Humidi_HDC1000 
                                                            ?  SensorsData.Max.Humidi_HDC1000 = SensorsData.Data.Humidi_HDC1000[SensorsCnt] 
                                                            :  NULL; 
            // ����Сֵ
            SensorsData.Data.Lux_OPT3001[SensorsCnt]       <= SensorsData.Min.Lux_OPT3001 
                                                            ?  SensorsData.Min.Lux_OPT3001 = SensorsData.Data.Lux_OPT3001[SensorsCnt] 
                                                            :  NULL; 
            SensorsData.Data.Pressure_MPL3115[SensorsCnt]  <= SensorsData.Min.Pressure_MPL3115 
                                                            ?  SensorsData.Min.Pressure_MPL3115 = SensorsData.Data.Pressure_MPL3115[SensorsCnt] 
                                                            :  NULL; 
            SensorsData.Data.Temper_HDC1000[SensorsCnt]    <= SensorsData.Min.Temper_HDC1000 
                                                            ?  SensorsData.Min.Temper_HDC1000 = SensorsData.Data.Temper_HDC1000[SensorsCnt] 
                                                            :  NULL; 
            SensorsData.Data.Humidi_HDC1000[SensorsCnt]    <= SensorsData.Min.Humidi_HDC1000 
                                                            ?  SensorsData.Min.Humidi_HDC1000 = SensorsData.Data.Humidi_HDC1000[SensorsCnt] 
                                                            :  NULL;

            // clang-format on
            if (SensorsCnt == SenSors_Data_Buf_Num - 1)
            {

                for (int i = 0; i < 10; i++)
                {
                    SensorsData.Average.Lux_OPT3001 += SensorsData.Data.Lux_OPT3001[i];
                    SensorsData.Average.Pressure_MPL3115 += SensorsData.Data.Pressure_MPL3115[i];
                    SensorsData.Average.Temper_HDC1000 += SensorsData.Data.Temper_HDC1000[i];
                    SensorsData.Average.Humidi_HDC1000 += SensorsData.Data.Humidi_HDC1000[i];
                }
                SensorsData.Average.Lux_OPT3001 -= SensorsData.Max.Lux_OPT3001;
                SensorsData.Average.Pressure_MPL3115 -= SensorsData.Max.Pressure_MPL3115;
                SensorsData.Average.Temper_HDC1000 -= SensorsData.Max.Temper_HDC1000;
                SensorsData.Average.Humidi_HDC1000 -= SensorsData.Max.Humidi_HDC1000;

                SensorsData.Average.Lux_OPT3001 -= SensorsData.Min.Lux_OPT3001;
                SensorsData.Average.Pressure_MPL3115 -= SensorsData.Min.Pressure_MPL3115;
                SensorsData.Average.Temper_HDC1000 -= SensorsData.Min.Temper_HDC1000;
                SensorsData.Average.Humidi_HDC1000 -= SensorsData.Min.Humidi_HDC1000;

                SensorsData.Average.Lux_OPT3001 /= SenSors_Data_Buf_Num - 2;
                SensorsData.Average.Pressure_MPL3115 /= SenSors_Data_Buf_Num - 2;
                SensorsData.Average.Temper_HDC1000 /= SenSors_Data_Buf_Num - 2;
                SensorsData.Average.Humidi_HDC1000 /= SenSors_Data_Buf_Num - 2;

                debug_printf("ƽ�����ݣ�%f, %f, %d, %d",
                             SensorsData.Average.Lux_OPT3001,
                             SensorsData.Average.Pressure_MPL3115,
                             SensorsData.Average.Temper_HDC1000,
                             SensorsData.Average.Humidi_HDC1000);

                // �ͷŶ�̬������ڴ�
                free(SensorsData.Data.Lux_OPT3001);
                free(SensorsData.Data.Pressure_MPL3115);
                free(SensorsData.Data.Temper_HDC1000);
                free(SensorsData.Data.Humidi_HDC1000);
            }

            debug_printf("��ǰ���ݣ�%f, %f, %d, %d",
                         SensorsData.Data.Lux_OPT3001[SensorsCnt],
                         SensorsData.Data.Pressure_MPL3115[SensorsCnt],
                         SensorsData.Data.Temper_HDC1000[SensorsCnt],
                         SensorsData.Data.Humidi_HDC1000[SensorsCnt]);

            SensorsCnt == SenSors_Data_Buf_Num - 1 ? SensorsCnt = 0 : SensorsCnt++;
        }
    }
    break;

    default:
        break;
    }
}

/**
 * @brief   ������汾��Ϣ�Ͱ���ʹ��˵����Ϣ��ӡ
 * @details �ϵ����еƻ������100ms
 * @param   ��
 * @return  ��
 */
void LoRaWAN_Borad_Info_Print(void)
{
    debug_printf("\r\n\r\n");
    PRINT_CODE_VERSION_INFO("%s", CODE_VERSION);
    debug_printf("\r\n");
    debug_printf("-. Press Key1 to: \r\n");
    debug_printf("-.  - Enter command Mode\r\n");
    debug_printf("-.  - Enter Transparent Mode\r\n");
    debug_printf("-. Press Key2 to: \r\n");
    debug_printf("-.  - Enter Project Trainning Mode\r\n");
    LEDALL_ON;
    HAL_Delay(100);
    LEDALL_OFF;
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM3)
    {
        // �û�����
        Tim3_Counter == (Tim3_Sensors_Delay_Secend * 100) ? Tim3_Counter = 0 : Tim3_Counter++;
    }
}
