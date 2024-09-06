/*
 *-------------------------------------------------------------------------
 * Author:    zhiller <zhiyiyimail@163.com>
 * Version:   v0.0.1
 * Brief:			Э�鶨����������ȡ	
 *-------------------------------------------------------------------------
 */

#include "zshell_proto.h"

// ============================ ֡������ ================================

/**
 * @brief ��֤֡��������
 * 
 * ���ݲ�ͬ��У���ģʽ��ѡ���ʵ�����֤������У��֡��У��͡���֤�������֡����Ч�ԡ�
 * 
 * @param f ָ�� zsFrame �ṹ���ָ��
 * 
 * @return ���֡У��ɹ������� FRAME_VALID�����򣬷��� FRAME_INVALID��
 */
uint8_t zsFrame_Verify(zsFrame* f){
	uint8_t validRes;
#if FRAME_CHECKSUM_MODE==0
	validRes = zsFrame_VerifyCheckSum(f);
#elif FRAME_CHECKSUM_MODE==1
	validRes = zsFrame_VerifyXorSum(f);
#elif FRAME_CHECKSUM_MODE==2
	validRes = zsFrame_VerifyCRC(f);
#endif
	if(validRes) return FRAME_VALID;
	else return FRAME_INVALID;
}

/**
 * @brief �����ݻ��������� zsFrame �ṹ��
 * 
 * �������� `uint8_t` ���ݻ���������Ϊ `zsFrame` �ṹ�塣��������ɹ����ṹ����ֶν�����䡣
 * 
 * @param data ָ�����ݻ�������ָ�룬�û�����Ӧ�������� `zsFrame` �ṹ���ʽ��������
 * @param frame ָ�������� `zsFrame` �ṹ���ָ��
 * 
 * @return ��������ɹ����� `FRAME_VALID`�����򷵻� `FRAME_INVALID`
 */
uint8_t zsFrame_ParserFromPlainData(uint8_t* data,zsFrame* frame){
	if(!data || !frame) return FRAME_INVALID;
	// �����ṹ���ֶ�
	frame->frameStart = data[0];
	frame->commandCode = data[1];
	frame->dataLength = (data[2] << 8) | data[3]; // �������ֽں͵��ֽ�
	frame->data = (uint8_t*)(data + 4); // ����ָ��ָ��������ʼλ��
	frame->checkSum = data[4 + frame->dataLength]; // У���
	frame->frameEnd = data[5 + frame->dataLength]; // ֡������־
	return FRAME_VALID;
}

/**
 * @brief ������֡���л�Ϊԭʼ�ֽ�����
 * 
 * �ú����� `zsFrame` �ṹ���е��������л�Ϊһ���ֽ����飬�Ա��ڴ����洢�����л����̰�����֡����ʼ��־�������롢���ݳ��ȡ��������ݡ�У��ͺͽ�����־ת��Ϊһ���������ֽ�����
 * 
 * @param frame ָ��Ҫ���л��� `zsFrame` �ṹ���ָ�롣���뱣֤��ָ�벻Ϊ NULL������ָ����Ч�� `zsFrame` ���ݡ�
 * 
 * @return �ɹ�ʱ����һ��ָ�����л��ֽ������ָ�롣��������Ҫ�����ͷ����ָ����ռ�õ��ڴ档ʧ��ʱ�������ڴ����ʧ�ܻ� `frame` Ϊ NULL�������� NULL��
 * 
 * @note 
 * - ȷ���ڵ��ô˺���֮ǰ��`frame` �е� `data` ��Աָ����Ч�����ݻ����������� `dataLength` �ֶη�ӳ�����ݵ�ʵ�ʳ��ȡ�
 * - �ڴ����ʧ�ܿ��ܻᵼ�º������� NULL��������Ӧ��鷵��ֵ��ȷ���ɹ��������ڴ档
 * - ����ʹ�� `malloc` ��̬�����ڴ棬��˵�������ʹ���귵�ص��ֽ������������ `free` �ͷ��ڴ档
 * 
 * @warning 
 * - ��� `frame` Ϊ NULL��������ֱ�ӷ��� NULL��
 * - ��������� `dataLength` ��ֵ�Ƿ�������� `dataLength` �ֶ�����Ч��Χ�ڡ�
 */
uint8_t* zsFrame_ParserToPlainData(zsFrame* frame,uint16_t* len){
	if (frame == NULL) {
			return NULL; // ����ָ��
	}
	
	// �������ݲ��ֵĳ���
	size_t frameLength = frame->dataLength+6; // frameStart, commandCode, dataLength, data, checksum
	uint8_t *serializedFrame = (uint8_t*)malloc(frameLength * sizeof(uint8_t));
	if (serializedFrame == NULL) {
			return NULL; // ����ڴ�����Ƿ�ɹ�
	}
  
  // ���л�֡
  serializedFrame[0] = frame->frameStart;
  serializedFrame[1] = frame->commandCode;
  serializedFrame[2] = (frame->dataLength >> 8) & 0xFF; // ���ֽ�
  serializedFrame[3] = frame->dataLength & 0xFF;         // ���ֽ�
  memcpy(&serializedFrame[4], frame->data, frame->dataLength);
  serializedFrame[4 + frame->dataLength] = frame->checkSum;
  // ���֡������־
  serializedFrame[frameLength - 1] = frame->frameEnd;
	
	*len = frameLength;
	
	return serializedFrame;
}

/**
 * @brief ��ӡ zsFrame �ṹ�������
 * 
 * ��ӡ `zsFrame` �ṹ���и����ֶε�ֵ��������ʼ��־��������롢���ݳ��ȡ����ݡ�У��ͼ�������־��
 * 
 * @param frame ָ�� `zsFrame` �ṹ���ָ��
 */
void zsFrame_PrintSelf(zsFrame* frame){
	if (!frame) return;
	printf("Frame Start: 0x%02X\n", frame->frameStart);
	printf("Command Code: 0x%02X\n", frame->commandCode);
	printf("Data Length: %d\n", frame->dataLength);
	
	printf("Data: ");
	for (uint16_t i = 0; i < frame->dataLength; ++i) {
		printf("0x%02X ", frame->data[i]);
	}
	printf("\n");

	printf("Checksum: 0x%02X\n", frame->checkSum);
	printf("Frame End: 0x%02X\n", frame->frameEnd);
}

void zsFrame_Builder(zsFrame* frame,uint8_t code,uint8_t* data){
	frame->frameStart = FRAME_START;
	frame->commandCode = code;
	frame->dataLength = strlen((char*)data);
	frame->data = data;
	
	#if FRAME_CHECKSUM_MODE==0
		frame->checkSum = zsFrame_GenerateCheckSum(frame);
	#elif FRAME_CHECKSUM_MODE==1
		frame->checkSum = zsFrame_GenerateXorSum(frame);
	#elif FRAME_CHECKSUM_MODE==2
		frame->checkSum = zsFrame_GenerateCRC(frame);
	#endif
	
	frame->frameEnd = FRAME_END;
	
	zsFrame_PrintSelf(frame);
}


// ============================ ֡У�� ================================

/**
 * @brief У���ۼӺ�
 * 
 * @param frame ָ�� zsFrame �ṹ���ָ��
 * 
 * @return У��ɹ����� 1��ʧ�ܷ��� 0
 */
uint8_t zsFrame_VerifyCheckSum(zsFrame* frame){
	uint8_t calculatedChecksum = zsFrame_GenerateCheckSum(frame);
	return calculatedChecksum == frame->checkSum;
}

/**
 * @brief �����ۼӺ�У��ֵ
 * 
 * @param frame ָ�� zsFrame �ṹ���ָ��
 * 
 * @return �������У���
 */
uint8_t zsFrame_GenerateCheckSum(zsFrame* frame){
	uint8_t checksum = 0;
	checksum += frame->frameStart;
	checksum += frame->commandCode;
	checksum += (uint8_t)(frame->dataLength >> 8); // ���ֽ�
	checksum += (uint8_t)(frame->dataLength & 0xFF); // ���ֽ�
	for (uint16_t i = 0; i < frame->dataLength; ++i) {
			checksum += frame->data[i];
	}
	checksum += frame->frameEnd;
	frame->checkSum=checksum;
	return checksum;
}


/**
 * @brief У��֡������
 * 
 * ����֡�����Ͳ���֡�е�У��ͽ��бȽϡ�
 * 
 * @param frame ָ�� zsFrame �ṹ���ָ��
 * 
 * @return �������У����Ƿ���֡�е�У�����ȣ������ȷ��� 1�����򷵻� 0
 */
uint8_t zsFrame_VerifyXorSum(zsFrame* frame){
	uint8_t calculatedChecksum = zsFrame_GenerateXorSum(frame);
	return calculatedChecksum == frame->checkSum;
}

/**
 * @brief ����֡������
 * 
 * ����֡�����Ͳ�����֡�е�У����ֶΡ�
 * 
 * @param frame ָ�� zsFrame �ṹ���ָ��
 * 
 * @return �����������
 */
uint8_t zsFrame_GenerateXorSum(zsFrame* frame){
	uint8_t checksum = 0;
	checksum ^= frame->frameStart;
	checksum ^= frame->commandCode;
	checksum ^= (uint8_t)(frame->dataLength >> 8); // ���ֽ�
	checksum ^= (uint8_t)(frame->dataLength & 0xFF); // ���ֽ�
	for (uint16_t i = 0; i < frame->dataLength; ++i) {
			checksum ^= frame->data[i];
	}
	checksum ^= frame->frameEnd;
	frame->checkSum=checksum;
	return checksum;
}

/**
 * @brief ��֤֡�� CRC У��
 * 
 * ����֡�� CRC ֵ����֡�е�У��ͽ��бȽϣ�����֤֡�������ԡ�
 * 
 * @param frame ָ�� zsFrame �ṹ���ָ��
 * 
 * @return ���������� CRC ֵ��֡�е�У�����ȣ����� 1�����򷵻� 0��
 */
uint8_t zsFrame_VerifyCRC(zsFrame* frame){
	uint8_t crcRes = zsFrame_GenerateCRC(frame);
	return crcRes == frame->checkSum;
}

/**
 * @brief ����֡�� CRC ֵ
 * 
 * ��֡������ת��Ϊһ���������������� CRC ֵ��������֡�е�У����ֶΡ�
 * 
 * @param frame ָ�� zsFrame �ṹ���ָ��
 * 
 * @return ������� CRC ֵ
 */
uint8_t zsFrame_GenerateCRC(zsFrame* frame){
  uint8_t buff[sizeof(zsFrame)]; // ����һ���㹻��Ļ�����
  uint8_t *ptr = buff; // ����һ��ָ��ָ�򻺳����Ŀ�ʼ
  
  *ptr++ = frame->frameStart;
  *ptr++ = frame->commandCode;
  *ptr++ = (uint8_t)(frame->dataLength >> 8); // ���� dataLength �� big-endian
  *ptr++ = (uint8_t)(frame->dataLength & 0xFF);
  memcpy(ptr, frame->data, frame->dataLength); // ��������
  ptr += frame->dataLength; // �ƶ�ָ�뵽����ĩβ
  
  uint32_t res = zsCRC_Generate(CRC8, buff, ptr - buff); // ���� CRC
	frame->checkSum = (uint8_t)res;
  return (uint8_t)res;
}




// ============================ CRCУ�鶨�� =================================


uint32_t zsCRC_BitsReverse(uint32_t inVal, uint8_t bits){
	unsigned int outVal = 0;
	unsigned char i;
	for (i = 0; i < bits; i++) {
		if (inVal & (1 << i)) outVal |= 1 << (bits - 1 - i);
	}
	return outVal;
}

uint32_t zsCRC_Generate(uint8_t type, uint8_t *buf, uint32_t bufLen){
	unsigned char width  = s_crcInfoTab[type].Width; //��ȣ���CRC��������
	unsigned int  crc    = s_crcInfoTab[type].CrcInit; //��ʼֵ,�����㷨��ʼʱ�Ĵ�����crc���ĳ�ʼ��Ԥ��ֵ��ʮ�����Ʊ�ʾ��
	unsigned int  xorout = s_crcInfoTab[type].XorOut; //��������˲�������õ����յ�CRCֵ��
	unsigned char refin  = s_crcInfoTab[type].RefIn; //�������ݵ�ÿ���ֽ��Ƿ�λ��ת��E_TRUE��E_FALSE��
	unsigned char refout = s_crcInfoTab[type].RefOut; //�ڼ����֮��������֮ǰ�����������Ƿ�λ��ת��E_TRUE��E_FALSE��
	unsigned char high;

	if (refin) { //���� LSB ����
		crc = zsCRC_BitsReverse(crc, width); //init ������;
		if (width > 8) { //Ϊ�˼�����λ�Ȳ�����width����8��С��8�ķֿ�����
			while (bufLen--) {
				crc = (crc >> 8) ^ s_crcTab[(crc & 0xFF) ^ *buf++];
			}
		} else {
			while (bufLen--) {
				crc = s_crcTab[crc ^ *buf++];
			}
		}
	} else { //���� MSB ����
		if (width > 8) { //Ϊ�˼�����λ�Ȳ�����width����8��С��8�ķֿ�����
			while (bufLen--) {
				high = crc >> (width - 8);
				crc = (crc << 8) ^ s_crcTab[high ^ *buf++];
			}
		} else {
			crc = crc << (8 - width);
			while (bufLen--) {
				crc = s_crcTab[crc ^ *buf++];
			}
			crc >>= 8 - width; //λ��С��8ʱ��crc�ڸ�widthλ��Ҫ���Ƶ�ԭλ
		}
	}

	if (refout != refin) { //�������
		crc = zsCRC_BitsReverse(crc, width);
	}

	crc ^= xorout; //������
	return crc & ((2 << (width - 1)) - 1);
}

void zsCRC_CrcTableCalculate(uint8_t type){
	unsigned char width  = s_crcInfoTab[type].Width; //��ȣ���CRC��������
	unsigned int  poly   = s_crcInfoTab[type].Poly; //���ɶ���ʽ�ļ�д����16���Ʊ�ʾ�����磺CRC-32����0x04C11DB7�����������λ��"1"������������������0x104C11DB7��
	unsigned char refIn  = s_crcInfoTab[type].RefIn; //�������ݵ�ÿ���ֽ��Ƿ�λ��ת��E_TRUE��E_FALSE��
	unsigned int validBits = (2 << (width - 1)) - 1;
	unsigned int value;
	unsigned int bit;
	unsigned int i;
	unsigned char j;

	if (refIn) { //����LSB����
		poly = zsCRC_BitsReverse(poly, width); //poly������, s_crcTab �� CrcInit = 0;
		for (i = 0; i < 256; i++) {
			value = i;
			for (j = 0; j < 8; j++) {
				if (value & 1) {
					value = (value >> 1) ^ poly;
				} else {
					value = (value >> 1);
				}
			}
			s_crcTab[i] = value & validBits;
		}
	} else { //����MSB����
		poly = (width < 8) ? (poly << (8 - width)) : poly; //���λ��С��8��polyҪ���Ƶ����λ
		bit = (width > 8) ?  (1 << (width - 1)) : 0x80;
		for (i = 0; i < 256; i++) {
			value = (width > 8) ? (i << (width - 8)) : i;
			for (j = 0; j < 8; j++) {
				if (value & bit) {
					value = (value << 1) ^ poly;
				} else {
					value = (value << 1);
				}
			}
			s_crcTab[i] = value &  ((width < 8) ? 0xFF : validBits);
		}
	}
}




