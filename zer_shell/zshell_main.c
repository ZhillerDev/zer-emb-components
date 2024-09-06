/*
 *-------------------------------------------------------------------------
 * Author:    zhiller <zhiyiyimail@163.com>
 * Version:   v0.0.1
 * Brief:			Shell主控与全局配置
 *-------------------------------------------------------------------------
 */

#include "zshell_main.h"

uint8_t zsCurrentUartDevice = 0;

zsUartDevice uart_device[UART_DEVICE_MAX_COUNT] = {0};
zsRingBuff uart_ringbuff[UART_DEVICE_MAX_COUNT] = {0};


void zsUart_RingBuffProcess(void){
	
}
void zsUart_FifoProcess(void){
	
}

// ============================ 重写 printf =================================

#if 1             
#pragma import(__use_no_semihosting) // 禁用半主机模式，通常用于嵌入式系统的调试。
// 定义FILE结构体，通常用于标准输入输出库。
struct __FILE 
{ 
    int handle; // 文件句柄，用于标识文件。
}; 

FILE __stdout;   // 声明标准输出流，通常用于printf等函数。

// 重定义_sys_exit()函数，防止程序退出时调用系统退出函数。
void _sys_exit(int x) 
{ 
    x = x;     // 此处x被赋值后不再使用，可能是为了避免编译器警告。
} 

// 重定义fputc函数，用于标准输出。
int fputc(int ch,FILE *f)
{
	HAL_UART_Transmit(uart_device[UART_DEFAULT_PRINTF].uart, (uint8_t *)&ch, 1, 0xFFFF);
	return ch;
}
#endif


// ============================ UART 管理 =================================

/**
 * @brief 创建一个新的 UART 设备实例。
 * 
 * 此函数用于初始化并配置一个新的 UART 设备实例。它会根据传入的模式设置设备的工作模式，并为设备分配一个环形缓冲区。函数还会更新当前 UART 设备的状态和其他相关信息。
 * 
 * @param mode 设备的工作模式。根据实际需要，传入不同的模式值。
 * @param uart 指向 UART_HandleTypeDef 结构体的指针，表示要配置的 UART 外设。请确保该指针指向一个有效的 UART 句柄。
 * 
 * @note 此函数会更新全局变量 `zsCurrentUartDevice`，该变量用于跟踪当前的 UART 设备索引。
 * 
 * @warning 确保 `zsCurrentUartDevice` 在调用该函数之前已正确初始化且在有效范围内。否则，可能会导致数组越界或未定义行为。
 * 
 * @pre 必须先调用 `zsRingBuff_create` 函数来初始化环形缓冲区。
 * 
 * @post 在函数调用后，`uart_device` 数组中的设备实例将被填充相应的配置信息，并且 `zsCurrentUartDevice` 将递增以指向下一个可用的设备槽。
 * 
 * @see zsRingBuff_create
 * @see UART_HandleTypeDef
 */
void zsUart_CreateDevice(uint8_t mode,UART_HandleTypeDef* uart){
	
	uint8_t id = zsCurrentUartDevice;
	zsUartDevice* device = &uart_device[id];
	zsRingBuff_create(&uart_ringbuff[id]);
	
	device->id=zsCurrentUartDevice;
	device->status = ZS_UART_IDLE;
	device->mode = mode;
	device->ringBuff = &uart_ringbuff[id];
	device->uart = uart;
	
	zsCurrentUartDevice++;
}

void zsUart_DeleteDevice(uint8_t id){
	if(zsCurrentUartDevice==0) return;
	zsCurrentUartDevice--;
}

/**
 * @brief 初始化UART通信
 *
 * 该函数负责初始化UART通信，包括配置UART相关的硬件参数，
 * 如波特率、字长、停止位等。如果定义了ZSHELL_USE_RINGBUFF宏，
 * 则还会创建一个环形缓冲区用于接收数据，并且启动DMA接收。
 *
 * @note 该函数假设UART和DMA已经在其他地方进行了基本的硬件初始化。
 */
void zsUart_Init(){
	if(zsCurrentUartDevice==0) return;
	for(int i=0;i<zsCurrentUartDevice;i++){
		HAL_UARTEx_ReceiveToIdle_DMA(uart_device[i].uart,uart_device[i].ringBuff->tempBuff,RING_BUFF_TEMP_SIZE);	
	}
}


/**
 * @brief 监听UART接收状态并处理
 *
 * 根据环形缓冲区的状态，执行相应的处理函数。
 */
void zsUart_Listen(void){
	for(int i=0;i<zsCurrentUartDevice;i++){
		zsUartDevice* device = &uart_device[i];
		if(device->status == ZS_UART_READABLE){
			if(device->mode == ZS_UART_CONSOLE){
				uint16_t len = zsRingBuff_getWriteLen(device->ringBuff);
				uint8_t* temp=(uint8_t*)malloc(len);
				zsRingBuff_readLen(device->ringBuff,temp,len);
				zsConsole_CmdProcess(i,temp,len);
				device->ringBuff->ringStatus = RING_IDLE;
				free(temp);

			}else if(device->mode == ZS_UART_FRAME){
				
			}
			
			device->status = ZS_UART_IDLE;
		}
	}
}


void zsUart_ErrorHandler(void){
//#ifdef ZSHELL_USE_RINGBUFF
//	ringBuff.ringStatus=RING_IDLE;
//#endif
}


/**
 * @brief 通过 UART 发送数据
 * 
 * 该函数使用 DMA 方式将数据发送到指定的 UART 设备。函数会等待直到数据传输完成。
 * 
 * @param id UART 设备的索引，索引应该在有效范围内。
 * @param data 指向待发送数据的指针，数据的类型为 uint8_t 数组。
 * 
 * @note 该函数假设 `uart_device` 是一个包含 UART 设备配置的全局数组。
 *       确保在调用此函数前已正确初始化 UART 设备。
 */
void zsUart_Send(uint8_t id,uint8_t* data){
	size_t len = strlen((char*)data);
	HAL_UART_Transmit_DMA(uart_device[id].uart,data,len);
	while (__HAL_UART_GET_FLAG(uart_device[id].uart, UART_FLAG_TC) == RESET);
	// 清除传输完成标志
	__HAL_UART_CLEAR_FLAG(uart_device[id].uart, UART_FLAG_TC);	
}

/**
 * @brief 通过 UART 发送格式化数据
 * 
 * 该函数使用 DMA 方式将格式化的数据发送到指定的 UART 设备。函数会等待直到数据传输完成。
 * 
 * @param id UART 设备的索引，索引应该在有效范围内。
 * @param data 格式化字符串的格式，类似于 printf 的格式化字符串。
 * @param ... 变长参数，根据格式化字符串提供相应的数据。
 * 
 * @note 该函数使用 `vsnprintf` 来格式化字符串，确保格式化后的字符串能够适应缓冲区大小。
 *       如果缓冲区不足，函数会清空缓冲区并返回。
 *       确保在调用此函数前已正确初始化 UART 设备。
 */
void zsUart_SendFormat(uint8_t id,uint8_t* data,...){
  va_list args;
  int length;
	uint8_t buffer[UART_PRINT_BUFFER_SIZE];
  
  if (buffer == NULL || data == NULL) return;
  
  // 初始化 va_list
  va_start(args, data);
  
  // 使用 vsnprintf 来格式化字符串
  length = vsnprintf((char *)buffer, UART_PRINT_BUFFER_SIZE, (char*)data, args);
  
  // 清理 va_list
  va_end(args);
  
  // 确保字符串以 null 结尾
  if (length > 0 && length < UART_PRINT_BUFFER_SIZE) {
    buffer[length] = '\0';
  } else {
    // 如果缓冲区太小，无法容纳整个字符串
    memset(buffer, 0, UART_PRINT_BUFFER_SIZE);
    length = -1;
  }
	
	HAL_UART_Transmit_DMA(uart_device[id].uart,buffer,length+1);
	while (__HAL_UART_GET_FLAG(uart_device[id].uart, UART_FLAG_TC) == RESET);
	// 清除传输完成标志
	__HAL_UART_CLEAR_FLAG(uart_device[id].uart, UART_FLAG_TC);	
	return;
}

void zsUart_SendFrame(uint8_t id,uint8_t code,uint8_t* data){
	zsFrame* frame=(zsFrame*)malloc(sizeof(zsFrame));
	uint16_t len=0;
	
	zsFrame_Builder(frame,code,data);
	uint8_t* output = zsFrame_ParserToPlainData(frame,&len);
	
	// 为每个字节的十六进制表示分配足够的空间
	// 每个字节需要2个字符 + 1个空格，加上最后的空字符
	uint8_t* hexString = (uint8_t*)malloc((len * 3 + 1) * sizeof(uint8_t));
	if (hexString == NULL) {
			free(frame);
			free(output);
			return;
	}

	// 将字节转换为十六进制字符串
	size_t hexPos = 0;
	for (size_t i = 0; i < len; ++i) {
		hexPos += snprintf((char*)&hexString[hexPos], 3, "%02X", output[i]);
	}
	hexString[hexPos] = '\0'; // 确保字符串以空字符结尾
	
	HAL_UART_Transmit_DMA(uart_device[id].uart,hexString,len*3+1);
	while (__HAL_UART_GET_FLAG(uart_device[id].uart, UART_FLAG_TC) == RESET);
	// 清除传输完成标志
	__HAL_UART_CLEAR_FLAG(uart_device[id].uart, UART_FLAG_TC);	
	
	free(frame);
	free(output);
	free(hexString);
	return;
}


// ============================ UART 回调与ISR处理 =================================

void zs_uart_ringbuff_data_process(uint8_t id){
	zsUartDevice* device = &uart_device[id];
	if(device->mode == ZS_UART_CONSOLE){
		device->ringBuff->ringStatus = RING_READ;
		device->status = ZS_UART_READABLE;
	}
}
void zs_uart_fifo_data_process(uint8_t id){
	
}

/**
 * @brief UART接收完成回调函数
 * 
 * 当UART接收DMA传输完成时，此回调函数被调用。它负责将接收到的数据
 * 写入环形缓冲区，并处理环形缓冲区的状态。
 * 
 * @param huart: 指向UART_HandleTypeDef结构体的指针，包含UART配置信息
 * @param Size: 接收到的数据大小
 */
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
	for(int i=0;i<zsCurrentUartDevice;i++){
		zsUartDevice* device = &uart_device[i]; 
		if(huart==device->uart){
			__HAL_UNLOCK(huart);
			device->ringBuff->ringStatus=RING_WRITING;
			
			uint16_t remainSize = zsRingBuff_getRemainLen(device->ringBuff);
			if(remainSize<Size){
			zsRingBuff_reset(device->ringBuff);
				return;
			}
			if(device->ringBuff->ringStatus==RING_READ || device->ringBuff->ringStatus==RING_ERROR){
				return;
			}
			if(Size>=RING_BUFF_TEMP_SIZE){
				device->ringBuff->ringStatus=RING_ERROR;
				return;
			}
			
			zsRingBuff_writeLen(device->ringBuff,device->ringBuff->tempBuff,Size);
		
			// 处理环形数组，进行帧校验或者丢弃帧
			zs_uart_ringbuff_data_process(i);
		}
		
		memset(uart_device[i].ringBuff->tempBuff,0,RING_BUFF_TEMP_SIZE);
		HAL_UARTEx_ReceiveToIdle_DMA(uart_device[i].uart,uart_device[i].ringBuff->tempBuff,RING_BUFF_TEMP_SIZE);
	}
	
	
}

/**
 * @brief UART错误回调函数
 * 
 * 当UART发生错误时，此回调函数被调用。它负责重置环形缓冲区，
 * 清空临时缓冲区，并重新启动DMA接收。
 * 
 * @param huart: 指向UART_HandleTypeDef结构体的指针，包含UART配置信息
 */
void HAL_UART_ErrorCallback(UART_HandleTypeDef * huart)
{
	for(int i=0;i<zsCurrentUartDevice;i++){
		zsUartDevice* device = &uart_device[i]; 
		if(huart==device->uart){
			memset(device->ringBuff->tempBuff, 0, sizeof(device->ringBuff->tempBuff));
			HAL_UARTEx_ReceiveToIdle_DMA(&huart1, device->ringBuff->tempBuff, RING_BUFF_TEMP_SIZE);
			__HAL_DMA_DISABLE_IT(&hdma_usart1_rx, DMA_IT_HT);		   // 手动关闭DMA_IT_HT中断
			printf("串口发生错误了！！！ \r\n");
			break;
		}
	}
}



// ============================ RingBuff 环形缓冲区处理 =================================

/**
 * @brief 从环形缓冲区读取指定长度的数据并通过UART发送
 *
 * 该函数从环形缓冲区中读取指定长度的数据，并通过UART使用DMA方式发送。
 * 发送完成后，它会等待直到传输完成标志被设置，然后清除该标志。
 * 最后，释放用于存储数据的临时内存。
 *
 * @param len 要读取并发送的数据长度
 */
void zsUart_RingBuffPrintLen(uint16_t len){
//	u8* temp=(u8*)malloc(len);
//	zsRingBuff_readLen(&ringBuff,temp,len);
//	HAL_UART_Transmit_DMA(&UART_CURRENT,temp,len);
//	while (__HAL_UART_GET_FLAG(&UART_CURRENT, UART_FLAG_TC) == RESET);
//	// 清除传输完成标志
//	__HAL_UART_CLEAR_FLAG(&UART_CURRENT, UART_FLAG_TC);	
//	free(temp);
}

/**
 * @brief 从环形缓冲区读取所有数据并通过UART发送
 *
 * 该函数获取环形缓冲区中所有已写入的数据长度，分配相应大小的内存，
 * 从环形缓冲区中读取数据，并通过UART使用DMA方式发送这些数据。
 * 发送完成后，它会等待直到传输完成标志被设置，然后清除该标志。
 * 最后，释放用于存储数据的临时内存。
 */
void zsUart_RingBuffPrintAll(void){
//	uint16_t len = zsRingBuff_getWriteLen(&ringBuff);
//	u8* temp=(u8*)malloc(len);
//	zsRingBuff_readLen(&ringBuff,temp,len);
//	HAL_UART_Transmit_DMA(&UART_CURRENT,temp,len);
//	while (__HAL_UART_GET_FLAG(&UART_CURRENT, UART_FLAG_TC) == RESET);
//	// 清除传输完成标志
//	__HAL_UART_CLEAR_FLAG(&UART_CURRENT, UART_FLAG_TC);	
//	free(temp);
}



// ============================ Console 命令行控制 =================================

// 定义一个常量字符串，用作命令行提示符
static uint8_t* zs_console_name = (uint8_t*)zs_cfgCONSOLE_DEFAULE_NAME;

// 定义一个二维数组，用于存储分割后的命令行参数
static uint8_t zs_console_argv_arr[zs_cfgCONSOLE_MAX_ARGV][zs_cfgCONSOLE_MAX_ARGV_LEN] = {0};

// 定义一个函数，用于将接收到的字符串分割成多个参数
static uint16_t zs_console_string_split(uint8_t *string, uint16_t len, uint8_t *argv, uint16_t size) {	
    uint16_t argc = 0;  // 参数计数器
    uint8_t *ch = malloc(len + 1);  // 动态分配内存，用于存储复制的字符串（+1 用于 '\0'）
    
    if (ch == NULL) {
        // 内存分配失败，处理错误（可以是打印错误消息或其他处理）
        return 0;
    }

    memcpy(ch, string, len);  // 复制字符串到分配的内存
    ch[len] = '\0';  // 添加字符串结束符

    uint8_t *str = (uint8_t*) strtok((char *) ch, " ");  // 使用 strtok 分割字符串
    
    while (str != NULL) {
        // 确保参数不会超出数组边界
        if (argc < zs_cfgCONSOLE_MAX_ARGV) {
            strncpy((char *)argv + (argc * size), (char *)str, size - 1);  // 复制分割后的参数到 argv 数组
            ((char *)argv)[(argc * size) + size - 1] = '\0';  // 确保每个参数以 '\0' 结尾
            argc++;  // 增加参数计数
        } else {
            break;  // 如果超出最大参数数量，退出循环
        }
        str = (uint8_t*)strtok(NULL, " ");  // 获取下一个参数
    }

    free(ch);  // 释放动态分配的内存
    return argc;  // 返回参数的数量
}

void zs_console_cmd_layer1(uint8_t id){
	if (strcmp((char *) zs_console_argv_arr[0], "zs") == 0) {
		zsUart_Send(id,"zShell 帮助手册 (1/1) \r\n");
		zsUart_Send(id,"zs info - 查看shell详细信息 \r\n");
		zsUart_Send(id,"zs version - 查看shell版本信息 \r\n");
	} else {
		zsUart_Send(id,"Sorry ,but i cannot understand the command!!! \r\n");
	}
}
void zs_console_cmd_layer2(uint8_t id){
	if (strcmp((char *) zs_console_argv_arr[0], "zs") == 0) {
		if (strcmp((char *) zs_console_argv_arr[1], "info") == 0){
			zsUart_SendFormat(id,"%s \r\n",(uint8_t*)zs_infoAuthor);
		}else if (strcmp((char *) zs_console_argv_arr[1], "version") == 0)
			zsUart_SendFormat(id,"%s \r\n",(uint8_t*)zs_infoVersion);
		else{
			zsUart_Send(id,"Sorry ,but i cannot understand the command!!! \r\n");
		}
	}
}
void zs_console_cmd_layer3(uint8_t id){
	
}

void zs_console_process_main(uint8_t id,uint16_t argc){
	switch(argc){
		case 1:{
			zs_console_cmd_layer1(id);
			break;
		}
		case 2:{
			zs_console_cmd_layer2(id);
			break;
		}
		case 3:{
			zs_console_cmd_layer3(id);
			break;
		}
		default:
			zsUart_Send(id,"Sorry ,but i cannot understand the command!!! \r\n");
	}
}

void zsConsole_CmdProcess(uint8_t id,uint8_t* data,uint16_t len){
	zsUart_SendFormat(id,"%s\r\n",data);
	uint16_t layer = zs_console_string_split(data,len,(uint8_t*)zs_console_argv_arr,zs_cfgCONSOLE_MAX_ARGV_LEN);
	zs_console_process_main(id,layer);
	memset(zs_console_argv_arr,0,zs_cfgCONSOLE_MAX_ARGV*zs_cfgCONSOLE_MAX_ARGV_LEN);
	zsUart_SendFormat(id,"\r\n%s",zs_console_name);
}

void zsConsole_PrintDefault(uint8_t id){
	zsUart_Send(id,"                            \r\n");
	zsUart_Send(id,"      _____  _         _  _ \r\n");
	zsUart_Send(id," ___ |   __|| |_  ___ | || |\r\n");
	zsUart_Send(id,"|- _||__   ||   || -_|| || |\r\n");
	zsUart_Send(id,"|___||_____||_|_||___||_||_|\r\n");
	zsUart_Send(id,"                            \r\n");
	zsUart_Send(id,zs_console_name);
}



// ============================ Frame 自定义帧控制 =================================

void zsConsole_FrameProcess(uint8_t id,uint8_t* data,uint16_t len){
	
}

