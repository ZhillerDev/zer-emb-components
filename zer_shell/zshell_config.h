/*
 *-------------------------------------------------------------------------
 * Author:    zhiller <zhiyiyimail@163.com>
 * Version:   v0.0.1
 * Brief:			zshell 组件全局配置文件
 *-------------------------------------------------------------------------
 */


#ifndef __ZSHELL_CONFIG_H__
#define __ZSHELL_CONFIG_H__


// ============================ 导入共用头文件 =================================

#include "stdio.h"
#include "stdarg.h"
#include "stdlib.h"
#include "stdint.h"
#include "string.h"

#include "usart.h"


// ============================ 环形缓冲区与FIFO配置 =================================

#define RING_BUFF_SIZE 									1024
#define RING_BUFF_TEMP_SIZE 						128


// ============================ 自定义通讯协议配置 =================================

// 帧头和尾定义
#define FRAME_START  		 								0xA5
#define FRAME_END 	 		 								0x5A

/**
 * @brief 校验和形式
 * 
 * 0 - 累加和  
 * 1 - 异或和  
 * 2 - CRC 校验
 */
#define FRAME_CHECKSUM_MODE 						0



// ============================ UART设备配置 =================================

// 最大可允许监控的UART设备数量
#define UART_DEVICE_MAX_COUNT 					6	
// UART输出缓冲区最大大小（使用DMA）
#define UART_PRINT_BUFFER_SIZE 					512

// printf重写时默认的输出设备（写设备ID）
#define UART_DEFAULT_PRINTF 						0



// ============================ 命令行快速配置 =================================

#define zs_cfgCONSOLE_DEFAULE_NAME 			"zShell /> "

#define zs_cfgCONSOLE_MAX_ARGV 					3
#define zs_cfgCONSOLE_MAX_ARGV_LEN 			10





// ============================ 组件信息 =================================

#define zs_infoAuthor										"zhiller"
#define zs_infoVersion									"0.0.1"


#endif /* __ZSHELL_CONFIG_H__ */
