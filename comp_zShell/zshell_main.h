#ifndef __ZSHELL_MAIN_H__
#define __ZSHELL_MAIN_H__

#include "zshell_config.h"
#include "zshell_ringbuff.h"
#include "zshell_proto.h"



// 主配置项
#define ZSHELL_USE_RINGBUFF 
// #define ZSHELL_USE_FIFO 


// ============================ UART设备接口管理 =================================



/**
 * @brief UART设备状态码
 * 
 * ZS_UART_STOP - 停止  
 * ZS_UART_RUNNING - 运行  
 * ZS_UART_IDLE - 空闲
 * ZS_UART_READABLE - 可读取数据
 */
typedef enum {
	ZS_UART_STOP=0,
	ZS_UART_RUNNING,
	ZS_UART_IDLE,
	ZS_UART_READABLE
} ZS_UART_DEVICE_STATUS;

/**
 * @brief UART当前模式
 * 
 * ZS_UART_CONSOLE - 命令行模式  
 * ZS_UART_FRAME - 自定义帧模式  
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

// 外部变量定义
extern uint8_t zsCurrentUartDevice;
extern zsUartDevice uart_device[UART_DEVICE_MAX_COUNT];
extern zsRingBuff uart_ringbuff[UART_DEVICE_MAX_COUNT];


// ============================ UART操作 =================================

// UART主控
void zsUart_Init(void);
void zsUart_Listen(void);
void zsUart_ErrorHandler(void);

// UART数据收发
void zsUart_Send(uint8_t id,uint8_t* data);
void zsUart_SendFormat(uint8_t id,uint8_t* data,...);
void zsUart_SendFrame(uint8_t id,uint8_t code,uint8_t* data);




// 环形缓冲区控制
void zsUart_RingBuffProcess(void);
void zsUart_RingBuffPrintLen(uint16_t len);
void zsUart_RingBuffPrintAll(void);
// 双缓冲区控制
void zsUart_FifoProcess(void);


// ============================ 命令行模式相关操作 =================================

void zsConsole_CmdProcess(uint8_t id,uint8_t* data,uint16_t len);
void zsConsole_PrintDefault(uint8_t id);



// ============================ 自定义帧模式相关操作 =================================


void zsConsole_FrameProcess(uint8_t id,uint8_t* data,uint16_t len);








#endif
