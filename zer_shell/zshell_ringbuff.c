/*
 *-------------------------------------------------------------------------
 * Author:    zhiller <zhiyiyimail@163.com>
 * Version:   v0.0.1
 * Brief:			环形队列&双缓存定义
 *-------------------------------------------------------------------------
 */

#include "zshell_ringbuff.h"

/**
 * @brief 初始化环形缓冲区。
 * 
 * 该函数将环形缓冲区的 `head` 和 `tail` 指针重置为零，并将环形缓冲区的状态设置为 `RING_IDLE`。同时，它还将缓冲区 `buff` 和临时缓冲区 `tempBuff` 的所有字节清零。
 * 
 * @param ring 指向 `zsRingBuff` 结构体的指针，表示要初始化的环形缓冲区。
 */
void zsRingBuff_create(zsRingBuff* ring) {
    ring->head = 0;
    ring->tail = 0;
    ring->ringStatus = RING_IDLE;
    memset(ring->buff, 0, RING_BUFF_SIZE);
    memset(ring->tempBuff, 0, RING_BUFF_TEMP_SIZE);
}

/**
 * @brief 删除环形缓冲区（空实现）。
 * 
 * 目前该函数为空实现，未来可以根据需要实现缓冲区的清理或资源释放功能。
 * 
 * @param ring 指向 `zsRingBuff` 结构体的指针，表示要删除的环形缓冲区。
 */
void zsRingBuff_delete(zsRingBuff* ring) {
    // 暂无实现
}

/**
 * @brief 重置环形缓冲区。
 * 
 * 该函数将环形缓冲区的 `head` 和 `tail` 指针重置为零，并将环形缓冲区的状态设置为 `RING_ERROR`。同时，它还将缓冲区 `buff` 和临时缓冲区 `tempBuff` 的所有字节清零。
 * 
 * @param ring 指向 `zsRingBuff` 结构体的指针，表示要重置的环形缓冲区。
 */
void zsRingBuff_reset(zsRingBuff* ring) {
    ring->head = 0;
    ring->tail = 0;
    ring->ringStatus = RING_ERROR;
    memset(ring->buff, 0, RING_BUFF_SIZE);
    memset(ring->tempBuff, 0, RING_BUFF_TEMP_SIZE);
}

/**
 * @brief 检查环形缓冲区是否已满。
 * 
 * 该函数判断缓冲区是否已满，即 `head` 指针是否跟随在 `tail` 指针之前，形成循环。
 * 
 * @param ring 指向 `zsRingBuff` 结构体的指针，表示要检查的环形缓冲区。
 * @return 返回 1 表示缓冲区已满，返回 0 表示缓冲区未满。
 */
uint8_t zsRingBuff_isFull(zsRingBuff* ring) {
    return (ring->head + 1) % RING_BUFF_SIZE == ring->tail;
}

/**
 * @brief 检查环形缓冲区是否为空。
 * 
 * 该函数判断缓冲区是否为空，即 `head` 指针是否与 `tail` 指针相同。
 * 
 * @param ring 指向 `zsRingBuff` 结构体的指针，表示要检查的环形缓冲区。
 * @return 返回 1 表示缓冲区为空，返回 0 表示缓冲区不为空。
 */
uint8_t zsRingBuff_isEmpty(zsRingBuff* ring) {
    return ring->head == ring->tail;
}

/**
 * @brief 向环形缓冲区写入一个字节。
 * 
 * 该函数将一个字节数据写入环形缓冲区。如果缓冲区已满，写入操作将失败。
 * 
 * @param ring 指向 `zsRingBuff` 结构体的指针，表示要写入数据的环形缓冲区。
 * @param data 要写入的字节数据。
 * @return 返回 1 表示写入成功，返回 0 表示缓冲区已满，写入失败。
 */
uint8_t zsRingBuff_write(zsRingBuff* ring, uint8_t data) {
    if (zsRingBuff_isFull(ring)) return 0;
    ring->buff[ring->head] = data;
    ring->head = (ring->head + 1) % RING_BUFF_SIZE;
    return 1;
}

/**
 * @brief 从环形缓冲区读取一个字节。
 * 
 * 该函数从环形缓冲区读取一个字节数据。如果缓冲区为空，读取操作将失败。
 * 
 * @param ring 指向 `zsRingBuff` 结构体的指针，表示要读取数据的环形缓冲区。
 * @param data 指向一个字节的指针，用于存储读取的数据。
 * @return 返回 1 表示读取成功，返回 0 表示缓冲区为空，读取失败。
 */
uint8_t zsRingBuff_read(zsRingBuff* ring, uint8_t* data) {
    if (zsRingBuff_isEmpty(ring)) return 0;
    *data = ring->buff[ring->tail];
    ring->tail = (ring->tail + 1) % RING_BUFF_SIZE;
    return 1;
}

/**
 * @brief 向环形缓冲区写入多个字节。
 * 
 * 该函数将多个字节数据写入环形缓冲区。如果缓冲区剩余空间不足以容纳所有数据，则写入操作将失败。
 * 
 * @param ring 指向 `zsRingBuff` 结构体的指针，表示要写入数据的环形缓冲区。
 * @param data 指向字节数组的指针，表示要写入的数据。
 * @param len 要写入的字节数。
 * @return 返回 1 表示写入成功，返回 0 表示缓冲区剩余空间不足或写入失败。
 */
uint8_t zsRingBuff_writeLen(zsRingBuff* ring, uint8_t* data, uint16_t len) {
    if (zsRingBuff_getRemainLen(ring) < len) {
        return 0;  // 缓冲区剩余空间不足
    }
    for (uint16_t i = 0; i < len; ++i) {
        if (!zsRingBuff_write(ring, data[i])) {
            return 0;  // 写入数据出错
        }
    }
    return 1;
}

/**
 * @brief 从环形缓冲区读取多个字节。
 * 
 * 该函数从环形缓冲区读取多个字节数据。如果缓冲区中的数据不足以满足读取要求，则读取操作将失败。
 * 
 * @param ring 指向 `zsRingBuff` 结构体的指针，表示要读取数据的环形缓冲区。
 * @param data 指向字节数组的指针，用于存储读取的数据。
 * @param len 要读取的字节数。
 * @return 返回 1 表示读取成功，返回 0 表示缓冲区中的数据不足或读取失败。
 */
uint8_t zsRingBuff_readLen(zsRingBuff* ring, uint8_t* data, uint16_t len) {
    if (zsRingBuff_getWriteLen(ring) < len) {
        return 0;  // 缓冲区中的数据不足
    }
    for (uint16_t i = 0; i < len; ++i) {
        if (!zsRingBuff_read(ring, &data[i])) {
            return 0;  // 读取数据出错
        }
    }
    return 1;
}

/**
 * @brief 获取环形缓冲区的剩余空间长度。
 * 
 * 该函数计算并返回环形缓冲区中剩余的空间长度（即可以写入数据的长度）。
 * 
 * @param ring 指向 `zsRingBuff` 结构体的指针，表示要计算剩余空间的环形缓冲区。
 * @return 返回剩余空间的字节数。
 */
uint16_t zsRingBuff_getRemainLen(zsRingBuff* ring) {
    if (ring->head >= ring->tail) {
        return RING_BUFF_SIZE - (ring->head - ring->tail);
    } else {
        return ring->tail - ring->head - 1;
    }
}

/**
 * @brief 获取环形缓冲区中的有效数据长度。
 * 
 * 该函数计算并返回环形缓冲区中当前写入的数据长度（即 `head` 和 `tail` 之间的数据量）。
 * 
 * @param ring 指向 `zsRingBuff` 结构体的指针，表示要计算数据长度的环形缓冲区。
 * @return 返回有效数据的字节数。
 */
uint16_t zsRingBuff_getWriteLen(zsRingBuff* ring) {
    if (ring->head >= ring->tail) {
        return ring->head - ring->tail;
    } else {
        return RING_BUFF_SIZE - (ring->tail - ring->head - 1);
    }
}



