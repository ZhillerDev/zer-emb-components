#ifndef __ZSHELL_MAIN_H__
#define __ZSHELL_MAIN_H__

#include "zshell_config.h"
#include "zshell_ringbuff.h"
#include "zshell_proto.h"



// ��������
#define ZSHELL_USE_RINGBUFF 
// #define ZSHELL_USE_FIFO 


// ============================ UART�豸�ӿڹ��� =================================



/**
 * @brief UART�豸״̬��
 * 
 * ZS_UART_STOP - ֹͣ  
 * ZS_UART_RUNNING - ����  
 * ZS_UART_IDLE - ����
 * ZS_UART_READABLE - �ɶ�ȡ����
 */
typedef enum {
	ZS_UART_STOP=0,
	ZS_UART_RUNNING,
	ZS_UART_IDLE,
	ZS_UART_READABLE
} ZS_UART_DEVICE_STATUS;

/**
 * @brief UART��ǰģʽ
 * 
 * ZS_UART_CONSOLE - ������ģʽ  
 * ZS_UART_FRAME - �Զ���֡ģʽ  
 */
typedef enum {
	ZS_UART_CONSOLE=0,
	ZS_UART_FRAME
} ZS_UART_DEVICE_MODE;

typedef struct _zs_uart_device {
	uint8_t id;
	uint8_t status;
	uint8_t mode;
	zsRingBuff* ringBuff;
	UART_HandleTypeDef* uart;
} zsUartDevice;

void zsUart_CreateDevice(
	uint8_t mode,
	UART_HandleTypeDef* uart
);
void zsUart_DeleteDevice(uint8_t id);

// �ⲿ��������
extern uint8_t zsCurrentUartDevice;
extern zsUartDevice uart_device[UART_DEVICE_MAX_COUNT];
extern zsRingBuff uart_ringbuff[UART_DEVICE_MAX_COUNT];


// ============================ UART���� =================================

// UART����
void zsUart_Init(void);
void zsUart_Listen(void);
void zsUart_ErrorHandler(void);

// UART�����շ�
void zsUart_Send(uint8_t id,uint8_t* data);
void zsUart_SendFormat(uint8_t id,uint8_t* data,...);
void zsUart_SendFrame(uint8_t id,uint8_t code,uint8_t* data);




// ���λ���������
void zsUart_RingBuffProcess(void);
void zsUart_RingBuffPrintLen(uint16_t len);
void zsUart_RingBuffPrintAll(void);
// ˫����������
void zsUart_FifoProcess(void);


// ============================ ������ģʽ��ز��� =================================

void zsConsole_CmdProcess(uint8_t id,uint8_t* data,uint16_t len);
void zsConsole_PrintDefault(uint8_t id);



// ============================ �Զ���֡ģʽ��ز��� =================================


void zsConsole_FrameProcess(uint8_t id,uint8_t* data,uint16_t len);








#endif
