#ifndef __ZSHELL_RINGBUFF_H__
#define __ZSHELL_RINGBUFF_H__

#include "zshell_config.h"


#define RING_READ 		0
#define RING_WRITING 	1
#define RING_IDLE 		2
#define RING_ERROR 		3


typedef struct _zs_ringbuff{
	uint8_t buff[RING_BUFF_SIZE];
	uint8_t tempBuff[RING_BUFF_TEMP_SIZE];
	uint16_t head;
	uint16_t tail;
	uint8_t ringStatus;
} zsRingBuff;

void zsRingBuff_create(zsRingBuff* ring);	// 初始化环形队列
void zsRingBuff_delete(zsRingBuff* ring);	// 销毁环形队列
void zsRingBuff_reset(zsRingBuff* ring); 	// 重置环形队列（一般用于环形队列溢出时）

uint8_t zsRingBuff_isFull(zsRingBuff* ring);
uint8_t zsRingBuff_isEmpty(zsRingBuff* ring);

uint8_t zsRingBuff_write(zsRingBuff* ring, uint8_t data);
uint8_t zsRingBuff_read(zsRingBuff* ring, uint8_t* data);
uint8_t zsRingBuff_writeLen(zsRingBuff* ring, uint8_t* data,uint16_t len);
uint8_t zsRingBuff_readLen(zsRingBuff* ring, uint8_t* data,uint16_t len);

uint16_t zsRingBuff_getRemainLen(zsRingBuff* ring);
uint16_t zsRingBuff_getWriteLen(zsRingBuff* ring);


#endif // __ZSHELL_QUEUE_H__
