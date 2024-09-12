/*
 *-------------------------------------------------------------------------
 * Author:    zhiller <zhiyiyimail@163.com>
 * Version:   v0.0.1
 * Brief:			���ζ���&˫���涨��
 *-------------------------------------------------------------------------
 */

#include "zshell_ringbuff.h"

/**
 * @brief ��ʼ�����λ�������
 * 
 * �ú��������λ������� `head` �� `tail` ָ������Ϊ�㣬�������λ�������״̬����Ϊ `RING_IDLE`��ͬʱ�������������� `buff` ����ʱ������ `tempBuff` �������ֽ����㡣
 * 
 * @param ring ָ�� `zsRingBuff` �ṹ���ָ�룬��ʾҪ��ʼ���Ļ��λ�������
 */
void zsRingBuff_create(zsRingBuff* ring) {
    ring->head = 0;
    ring->tail = 0;
    ring->ringStatus = RING_IDLE;
    memset(ring->buff, 0, RING_BUFF_SIZE);
    memset(ring->tempBuff, 0, RING_BUFF_TEMP_SIZE);
}

/**
 * @brief ɾ�����λ���������ʵ�֣���
 * 
 * Ŀǰ�ú���Ϊ��ʵ�֣�δ�����Ը�����Ҫʵ�ֻ��������������Դ�ͷŹ��ܡ�
 * 
 * @param ring ָ�� `zsRingBuff` �ṹ���ָ�룬��ʾҪɾ���Ļ��λ�������
 */
void zsRingBuff_delete(zsRingBuff* ring) {
    // ����ʵ��
}

/**
 * @brief ���û��λ�������
 * 
 * �ú��������λ������� `head` �� `tail` ָ������Ϊ�㣬�������λ�������״̬����Ϊ `RING_ERROR`��ͬʱ�������������� `buff` ����ʱ������ `tempBuff` �������ֽ����㡣
 * 
 * @param ring ָ�� `zsRingBuff` �ṹ���ָ�룬��ʾҪ���õĻ��λ�������
 */
void zsRingBuff_reset(zsRingBuff* ring) {
    ring->head = 0;
    ring->tail = 0;
    ring->ringStatus = RING_ERROR;
    memset(ring->buff, 0, RING_BUFF_SIZE);
    memset(ring->tempBuff, 0, RING_BUFF_TEMP_SIZE);
}

/**
 * @brief ��黷�λ������Ƿ�������
 * 
 * �ú����жϻ������Ƿ��������� `head` ָ���Ƿ������ `tail` ָ��֮ǰ���γ�ѭ����
 * 
 * @param ring ָ�� `zsRingBuff` �ṹ���ָ�룬��ʾҪ���Ļ��λ�������
 * @return ���� 1 ��ʾ���������������� 0 ��ʾ������δ����
 */
uint8_t zsRingBuff_isFull(zsRingBuff* ring) {
    return (ring->head + 1) % RING_BUFF_SIZE == ring->tail;
}

/**
 * @brief ��黷�λ������Ƿ�Ϊ�ա�
 * 
 * �ú����жϻ������Ƿ�Ϊ�գ��� `head` ָ���Ƿ��� `tail` ָ����ͬ��
 * 
 * @param ring ָ�� `zsRingBuff` �ṹ���ָ�룬��ʾҪ���Ļ��λ�������
 * @return ���� 1 ��ʾ������Ϊ�գ����� 0 ��ʾ��������Ϊ�ա�
 */
uint8_t zsRingBuff_isEmpty(zsRingBuff* ring) {
    return ring->head == ring->tail;
}

/**
 * @brief ���λ�����д��һ���ֽڡ�
 * 
 * �ú�����һ���ֽ�����д�뻷�λ����������������������д�������ʧ�ܡ�
 * 
 * @param ring ָ�� `zsRingBuff` �ṹ���ָ�룬��ʾҪд�����ݵĻ��λ�������
 * @param data Ҫд����ֽ����ݡ�
 * @return ���� 1 ��ʾд��ɹ������� 0 ��ʾ������������д��ʧ�ܡ�
 */
uint8_t zsRingBuff_write(zsRingBuff* ring, uint8_t data) {
    if (zsRingBuff_isFull(ring)) return 0;
    ring->buff[ring->head] = data;
    ring->head = (ring->head + 1) % RING_BUFF_SIZE;
    return 1;
}

/**
 * @brief �ӻ��λ�������ȡһ���ֽڡ�
 * 
 * �ú����ӻ��λ�������ȡһ���ֽ����ݡ����������Ϊ�գ���ȡ������ʧ�ܡ�
 * 
 * @param ring ָ�� `zsRingBuff` �ṹ���ָ�룬��ʾҪ��ȡ���ݵĻ��λ�������
 * @param data ָ��һ���ֽڵ�ָ�룬���ڴ洢��ȡ�����ݡ�
 * @return ���� 1 ��ʾ��ȡ�ɹ������� 0 ��ʾ������Ϊ�գ���ȡʧ�ܡ�
 */
uint8_t zsRingBuff_read(zsRingBuff* ring, uint8_t* data) {
    if (zsRingBuff_isEmpty(ring)) return 0;
    *data = ring->buff[ring->tail];
    ring->tail = (ring->tail + 1) % RING_BUFF_SIZE;
    return 1;
}

/**
 * @brief ���λ�����д�����ֽڡ�
 * 
 * �ú���������ֽ�����д�뻷�λ����������������ʣ��ռ䲻���������������ݣ���д�������ʧ�ܡ�
 * 
 * @param ring ָ�� `zsRingBuff` �ṹ���ָ�룬��ʾҪд�����ݵĻ��λ�������
 * @param data ָ���ֽ������ָ�룬��ʾҪд������ݡ�
 * @param len Ҫд����ֽ�����
 * @return ���� 1 ��ʾд��ɹ������� 0 ��ʾ������ʣ��ռ䲻���д��ʧ�ܡ�
 */
uint8_t zsRingBuff_writeLen(zsRingBuff* ring, uint8_t* data, uint16_t len) {
    if (zsRingBuff_getRemainLen(ring) < len) {
        return 0;  // ������ʣ��ռ䲻��
    }
    for (uint16_t i = 0; i < len; ++i) {
        if (!zsRingBuff_write(ring, data[i])) {
            return 0;  // д�����ݳ���
        }
    }
    return 1;
}

/**
 * @brief �ӻ��λ�������ȡ����ֽڡ�
 * 
 * �ú����ӻ��λ�������ȡ����ֽ����ݡ�����������е����ݲ����������ȡҪ�����ȡ������ʧ�ܡ�
 * 
 * @param ring ָ�� `zsRingBuff` �ṹ���ָ�룬��ʾҪ��ȡ���ݵĻ��λ�������
 * @param data ָ���ֽ������ָ�룬���ڴ洢��ȡ�����ݡ�
 * @param len Ҫ��ȡ���ֽ�����
 * @return ���� 1 ��ʾ��ȡ�ɹ������� 0 ��ʾ�������е����ݲ�����ȡʧ�ܡ�
 */
uint8_t zsRingBuff_readLen(zsRingBuff* ring, uint8_t* data, uint16_t len) {
    if (zsRingBuff_getWriteLen(ring) < len) {
        return 0;  // �������е����ݲ���
    }
    for (uint16_t i = 0; i < len; ++i) {
        if (!zsRingBuff_read(ring, &data[i])) {
            return 0;  // ��ȡ���ݳ���
        }
    }
    return 1;
}

/**
 * @brief ��ȡ���λ�������ʣ��ռ䳤�ȡ�
 * 
 * �ú������㲢���ػ��λ�������ʣ��Ŀռ䳤�ȣ�������д�����ݵĳ��ȣ���
 * 
 * @param ring ָ�� `zsRingBuff` �ṹ���ָ�룬��ʾҪ����ʣ��ռ�Ļ��λ�������
 * @return ����ʣ��ռ���ֽ�����
 */
uint16_t zsRingBuff_getRemainLen(zsRingBuff* ring) {
    if (ring->head >= ring->tail) {
        return RING_BUFF_SIZE - (ring->head - ring->tail);
    } else {
        return ring->tail - ring->head - 1;
    }
}

/**
 * @brief ��ȡ���λ������е���Ч���ݳ��ȡ�
 * 
 * �ú������㲢���ػ��λ������е�ǰд������ݳ��ȣ��� `head` �� `tail` ֮�������������
 * 
 * @param ring ָ�� `zsRingBuff` �ṹ���ָ�룬��ʾҪ�������ݳ��ȵĻ��λ�������
 * @return ������Ч���ݵ��ֽ�����
 */
uint16_t zsRingBuff_getWriteLen(zsRingBuff* ring) {
    if (ring->head >= ring->tail) {
        return ring->head - ring->tail;
    } else {
        return RING_BUFF_SIZE - (ring->tail - ring->head - 1);
    }
}



