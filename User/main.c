/********************************** (C) COPYRIGHT *******************************
 * File Name          : main.c
 * Author             : Tauno Erik
 * Started            : 2026/02/23
 * Edited             : 2026/02/24
 * Description        : DC1307 RTC Module
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

/*
 * Hardware connection:
 * PD5 -- Rx
 * PD6 -- Tx
 * PC1 -- SDA
 * PC2 -- SCL
 *
 */

#include "debug.h"

// DS1307 I2C Address is 0x68 (0xD0 for Write, 0xD1 for Read)
#define DS1307_ADDR 0xD0


// Global buffer for time data: [0]=Sec, [1]=Min, [2]=Hour, [3]=Day, [4]=Date, [5]=Month, [6]=Year
uint8_t time_buf[7];

volatile unsigned long ms_ticks = 0;

/* SysTick Interrupt Handler - Must match the name in startup_ch32v00x.S */
void SysTick_Handler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void SysTick_Handler(void) {
    ms_ticks++;
    SysTick->SR = 0; // Clear interrupt flag
    
    /* Set next comparison for 1ms later */
    SysTick->CMP += (SystemCoreClock / 8) / 1000;
}

/* Custom SysTick Initialization */
void SysTick_NonBlocking_Init(void) {
    /* Reset Counter */
    SysTick->CTLR = 0;
    SysTick->CNT = 0;
    
    /* Calculate 1ms interval (assuming 48MHz HCLK) */
    /* HCLK/8 = 6,000,000. 6,000,000 / 1000 = 6000 ticks per ms */
    uint32_t ticks_per_ms = (SystemCoreClock / 8) / 1000;
    SysTick->CMP = ticks_per_ms;
    
    /* CTLR: Bit0=Enable, Bit1=Interrupt, Bit2=HCLK/8 */
    SysTick->CTLR = 0x03; 
    
    NVIC_EnableIRQ(SysTick_IRQn);
}

/*********************************************************************
 * @fn      USARTx_CFG
 *
 * @brief   Initializes the USART2 & USART3 peripheral.
 *
 * @return  none
 */
void USARTx_CFG(void)
{
    GPIO_InitTypeDef  GPIO_InitStructure = {0};
    USART_InitTypeDef USART_InitStructure = {0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD | RCC_APB2Periph_USART1, ENABLE);

    /* USART1 TX-->D.5   RX-->D.6 */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_30MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOD, &GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOD, &GPIO_InitStructure);

    USART_InitStructure.USART_BaudRate = 115200;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;

    USART_Init(USART1, &USART_InitStructure);
    USART_Cmd(USART1, ENABLE);
}


/*
 * I2C seadistamine
 */
void I2C_Config_Init(void) {
    GPIO_InitTypeDef GPIO_InitStructure = {0};
    I2C_InitTypeDef I2C_InitTSturcture = {0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC | RCC_APB2Periph_AFIO, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);

    /* PC1=SDA, PC2=SCL (Standard CH32V003 I2C pins) */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    I2C_InitTSturcture.I2C_ClockSpeed = 100000;
    I2C_InitTSturcture.I2C_Mode = I2C_Mode_I2C;
    I2C_InitTSturcture.I2C_DutyCycle = I2C_DutyCycle_2;
    I2C_InitTSturcture.I2C_OwnAddress1 = 0x00;
    I2C_InitTSturcture.I2C_Ack = I2C_Ack_Enable;
    I2C_InitTSturcture.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    I2C_Init(I2C1, &I2C_InitTSturcture);

    I2C_Cmd(I2C1, ENABLE);
}

/*
 * Converts BCD to DEC
 */
uint8_t BCD2DEC(uint8_t val) {
    return ((val >> 4) * 10) + (val & 0x0F);
}

/*
 * Converts DEC to BCD
 */
uint8_t DEC2BCD(uint8_t val) {
    return ((val / 10) << 4) | (val % 10);
}

/*
 * Writes time to RTC
 */
void DS1307_SetTime(uint8_t sec, uint8_t min, uint8_t hour, uint8_t date, uint8_t month, uint8_t year) {
    while(I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY));
    I2C_GenerateSTART(I2C1, ENABLE);
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT));

    I2C_Send7bitAddress(I2C1, DS1307_ADDR, I2C_Direction_Transmitter);
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));

    I2C_SendData(I2C1, 0x00); // Start at Reg 0
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED));

    I2C_SendData(I2C1, DEC2BCD(sec)); // Bit 7 of Sec is 0 to start oscillator
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED));
    I2C_SendData(I2C1, DEC2BCD(min));
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED));
    I2C_SendData(I2C1, DEC2BCD(hour));
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED));
    I2C_SendData(I2C1, 0x01); // Day of week (unused here)
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED));
    I2C_SendData(I2C1, DEC2BCD(date));
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED));
    I2C_SendData(I2C1, DEC2BCD(month));
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED));
    I2C_SendData(I2C1, DEC2BCD(year));
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED));

    I2C_GenerateSTOP(I2C1, ENABLE);
}

/*
 * Reads the current time from RTC
 */
void DS1307_GetTime(void) {
    while(I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY));
    I2C_GenerateSTART(I2C1, ENABLE);
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT));

    I2C_Send7bitAddress(I2C1, DS1307_ADDR, I2C_Direction_Transmitter);
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));

    I2C_SendData(I2C1, 0x00); // Reset pointer to 0
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED));

    I2C_GenerateSTART(I2C1, ENABLE);
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT));

    I2C_Send7bitAddress(I2C1, DS1307_ADDR, I2C_Direction_Receiver);
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED));

    for(int i = 0; i < 7; i++) {
        if(i == 6) I2C_AcknowledgeConfig(I2C1, DISABLE);
        while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_RECEIVED));
        time_buf[i] = BCD2DEC(I2C_ReceiveData(I2C1));
    }
    I2C_GenerateSTOP(I2C1, ENABLE);
    I2C_AcknowledgeConfig(I2C1, ENABLE);
}

/*
The DS1307 features 56 bytes of non-volatile SRAM (if a backup battery is connected). 
These bytes are located at addresses 0x08 to 0x3F.
*/
void DS1307_WriteRAM(uint8_t addr, uint8_t data) {
    if (addr < 0x08 || addr > 0x3F) return; // Guard: Only SRAM range

    while(I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY));
    I2C_GenerateSTART(I2C1, ENABLE);
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT));

    I2C_Send7bitAddress(I2C1, DS1307_ADDR, I2C_Direction_Transmitter);
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));

    I2C_SendData(I2C1, addr); // Set internal pointer to SRAM address
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED));

    I2C_SendData(I2C1, data); // Send the data byte
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED));

    I2C_GenerateSTOP(I2C1, ENABLE);
}

uint8_t DS1307_ReadRAM(uint8_t addr) {
    uint8_t data = 0;
    if (addr < 0x08 || addr > 0x3F) return 0;

    while(I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY));
    I2C_GenerateSTART(I2C1, ENABLE);
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT));

    I2C_Send7bitAddress(I2C1, DS1307_ADDR, I2C_Direction_Transmitter);
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));

    I2C_SendData(I2C1, addr);
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED));

    // Repeated Start to switch to Read mode
    I2C_GenerateSTART(I2C1, ENABLE);
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT));

    I2C_Send7bitAddress(I2C1, DS1307_ADDR, I2C_Direction_Receiver);
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED));

    I2C_AcknowledgeConfig(I2C1, DISABLE); // NACK for single byte read
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_RECEIVED));
    data = I2C_ReceiveData(I2C1);

    I2C_GenerateSTOP(I2C1, ENABLE);
    I2C_AcknowledgeConfig(I2C1, ENABLE); // Restore ACK
    
    return data;
}


/**
 * @brief Reads a block of data from the DS1307 SRAM.
 * @param buffer: Pointer to the array where data will be stored.
 * @param len: Number of bytes to read (Max 56 for full SRAM).
 */
void DS1307_ReadRAMBurst(uint8_t *buffer, uint8_t len) {
    if (len > 56) len = 56; // Limit to actual SRAM size

    // 1. Wait for I2C bus to be free
    while(I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY));

    // 2. Set the pointer to the start of SRAM (0x08)
    I2C_GenerateSTART(I2C1, ENABLE);
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT));

    I2C_Send7bitAddress(I2C1, DS1307_ADDR, I2C_Direction_Transmitter);
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));

    I2C_SendData(I2C1, 0x08); // Starting address of SRAM
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED));

    // 3. Repeated Start to switch to Read Mode
    I2C_GenerateSTART(I2C1, ENABLE);
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT));

    I2C_Send7bitAddress(I2C1, DS1307_ADDR, I2C_Direction_Receiver);
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED));

    // 4. Read Loop
    for(int i = 0; i < len; i++) {
        /* If this is the LAST byte we want to receive, 
           we must disable ACK (send NACK) to tell the DS1307 
           to stop sending data.
        */
        if(i == (len - 1)) {
            I2C_AcknowledgeConfig(I2C1, DISABLE);
        }

        // Wait for data to arrive in the DR register
        while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_RECEIVED));
        
        // Read the data
        buffer[i] = I2C_ReceiveData(I2C1);
    }

    // 5. End Communication
    I2C_GenerateSTOP(I2C1, ENABLE);

    // Re-enable ACK for future operations
    I2C_AcknowledgeConfig(I2C1, ENABLE);
}


/**
 * @brief Writes a block of data to the DS1307 SRAM.
 * @param buffer: Pointer to the data array to be written.
 * @param len: Number of bytes to write (Max 56).
 */
void DS1307_WriteRAMBurst(uint8_t *buffer, uint8_t len) {
    if (len > 56) len = 56; // Cap at max SRAM size

    // 1. Wait for I2C bus to be free
    while(I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY));

    // 2. Start and Address Device
    I2C_GenerateSTART(I2C1, ENABLE);
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT));

    I2C_Send7bitAddress(I2C1, DS1307_ADDR, I2C_Direction_Transmitter);
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));

    // 3. Set starting pointer to SRAM beginning (0x08)
    I2C_SendData(I2C1, 0x08);
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED));

    // 4. Stream Data
    for(int i = 0; i < len; i++) {
        I2C_SendData(I2C1, buffer[i]);
        // Wait until the byte is shifted out
        while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED));
    }

    // 5. Stop condition
    I2C_GenerateSTOP(I2C1, ENABLE);
}

// The most powerful use of this is saving a configuration struct 
// so your device remembers its state after a reboot.
typedef struct {
    uint16_t set_point;     // 2 bytes
    uint8_t  mode;          // 1 byte
    uint8_t  alarm_enabled; // 1 byte
    uint8_t  magic_key;     // 1 byte (to check if RAM is valid)
} DeviceConfig;

DeviceConfig mySettings = {2500, 1, 0x54, 0xA5}; // Example data

void Save_Settings(DeviceConfig *cfg) {
    // Cast the struct pointer to a uint8_t pointer
    DS1307_WriteRAMBurst((uint8_t*)cfg, sizeof(DeviceConfig));
}

void Load_Settings(DeviceConfig *cfg) {
    DS1307_ReadRAMBurst((uint8_t*)cfg, sizeof(DeviceConfig));
}

/*********************************************************************
 * @fn      main
 *
 * @brief   Main program.
 *
 * @return  none
 */
int main(void)
{
    uint32_t last_print_1 = 0;
    uint32_t last_print_2 = 0;
    uint8_t boot_count = 0;
    uint8_t sram_dump[56];

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
    SystemCoreClockUpdate();
    Delay_Init();
    USART_Printf_Init(115200);

    printf("SystemClk:%d\r\n",SystemCoreClock);
    printf( "ChipID:%08x\r\n", DBGMCU_GetCHIPID() );

    USARTx_CFG();

    I2C_Config_Init();
    SysTick_NonBlocking_Init();

    // 1. Read existing boot count from SRAM address 0x08
    boot_count = DS1307_ReadRAM(0x08);
    boot_count++;
    
    // 2. Write updated count back to SRAM
    DS1307_WriteRAM(0x08, boot_count);

    printf("System Booted! Total boots: %d\r\n", boot_count);

    // Write current settings to the RTC battery-backed RAM
    Save_Settings(&mySettings);

    printf("Settings saved to RTC SRAM.\r\n");

    // Later, you can wipe the local struct and reload it from RTC
    DeviceConfig loadedSettings = {0};
    Load_Settings(&loadedSettings);

    if(loadedSettings.magic_key == 0xA5) {
        printf("Config loaded! Mode: %d, SetPoint: %d\r\n", 
               loadedSettings.mode, loadedSettings.set_point);
    } else {
        printf("RAM data invalid or battery low.\r\n");
    }

    while(1)
    {
        /*
        // Blocking way to print time
        DS1307_GetTime();
        printf("Date: 20%02d-%02d-%02d  Time: %02d:%02d:%02d\r\n", 
                time_buf[6], time_buf[5], time_buf[4], 
                time_buf[2], time_buf[1], time_buf[0]);
        Delay_Ms(1000);
        */

        // Non-blocking
        // Check if 1 second (1000ms) has passed
        if (ms_ticks - last_print_1 >= 1000) {
            last_print_1 = ms_ticks;

            DS1307_GetTime();
            
            printf("[%lu ms] Date: 20%02d-%02d-%02d  Time: %02d:%02d:%02d\r\n", 
                    ms_ticks,
                    time_buf[6], time_buf[5], time_buf[4], 
                    time_buf[2], time_buf[1], time_buf[0]);
            // 1. Read boot count from SRAM address 0x08
            boot_count = DS1307_ReadRAM(0x08);
            printf("boot_count: %d\r\n", boot_count);
        }

        if (ms_ticks - last_print_2 >= 10000) { // Every 10 seconds
            last_print_2 = ms_ticks;

            printf("--- DS1307 SRAM DUMP ---\r\n");
            DS1307_ReadRAMBurst(sram_dump, 56);

            for(int i = 0; i < 56; i++) {
                printf("%02X ", sram_dump[i]);
                if((i + 1) % 8 == 0) printf("\r\n"); // New line every 8 bytes
            }
            printf("------------------------\r\n");
        }

        // You can do other things here (like blink an LED) 
        // without waiting for the 1 second to finish.
    }
}
