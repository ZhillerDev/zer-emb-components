/*
 *-------------------------------------------------------------------------
 * Author:    zhiller <zhiyiyimail@163.com>
 * Version:   v0.0.1
 * Brief:			zshell ���ȫ�������ļ�
 *-------------------------------------------------------------------------
 */


#ifndef __ZSHELL_CONFIG_H__
#define __ZSHELL_CONFIG_H__


// ============================ ���빲��ͷ�ļ� =================================

#include "stdio.h"
#include "stdarg.h"
#include "stdlib.h"
#include "stdint.h"
#include "string.h"

#include "usart.h"


// ============================ ���λ�������FIFO���� =================================

#define RING_BUFF_SIZE 									1024
#define RING_BUFF_TEMP_SIZE 						128


// ============================ �Զ���ͨѶЭ������ =================================

// ֡ͷ��β����
#define FRAME_START  		 								0xA5
#define FRAME_END 	 		 								0x5A

/**
 * @brief У�����ʽ
 * 
 * 0 - �ۼӺ�  
 * 1 - ����  
 * 2 - CRC У��
 */
#define FRAME_CHECKSUM_MODE 						0



// ============================ UART�豸���� =================================

// ���������ص�UART�豸����
#define UART_DEVICE_MAX_COUNT 					6	
// UART�������������С��ʹ��DMA��
#define UART_PRINT_BUFFER_SIZE 					512

// printf��дʱĬ�ϵ�����豸��д�豸ID��
#define UART_DEFAULT_PRINTF 						0



// ============================ �����п������� =================================

#define zs_cfgCONSOLE_DEFAULE_NAME 			"zShell /> "

#define zs_cfgCONSOLE_MAX_ARGV 					3
#define zs_cfgCONSOLE_MAX_ARGV_LEN 			10





// ============================ �����Ϣ =================================

#define zs_infoAuthor										"zhiller"
#define zs_infoVersion									"0.0.1"


#endif /* __ZSHELL_CONFIG_H__ */
