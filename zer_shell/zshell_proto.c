/*
 *-------------------------------------------------------------------------
 * Author:    zhiller <zhiyiyimail@163.com>
 * Version:   v0.0.1
 * Brief:			协议定义与数据提取	
 *-------------------------------------------------------------------------
 */

#include "zshell_proto.h"

// ============================ 帧处理函数 ================================

/**
 * @brief 验证帧的完整性
 * 
 * 根据不同的校验和模式，选择适当的验证函数来校验帧的校验和。验证结果决定帧的有效性。
 * 
 * @param f 指向 zsFrame 结构体的指针
 * 
 * @return 如果帧校验成功，返回 FRAME_VALID；否则，返回 FRAME_INVALID。
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
 * @brief 从数据缓冲区解析 zsFrame 结构体
 * 
 * 将给定的 `uint8_t` 数据缓冲区解析为 `zsFrame` 结构体。如果解析成功，结构体的字段将被填充。
 * 
 * @param data 指向数据缓冲区的指针，该缓冲区应包含按照 `zsFrame` 结构体格式填充的数据
 * @param frame 指向解析后的 `zsFrame` 结构体的指针
 * 
 * @return 如果解析成功返回 `FRAME_VALID`，否则返回 `FRAME_INVALID`
 */
uint8_t zsFrame_ParserFromPlainData(uint8_t* data,zsFrame* frame){
	if(!data || !frame) return FRAME_INVALID;
	// 解析结构体字段
	frame->frameStart = data[0];
	frame->commandCode = data[1];
	frame->dataLength = (data[2] << 8) | data[3]; // 解析高字节和低字节
	frame->data = (uint8_t*)(data + 4); // 数据指针指向数据起始位置
	frame->checkSum = data[4 + frame->dataLength]; // 校验和
	frame->frameEnd = data[5 + frame->dataLength]; // 帧结束标志
	return FRAME_VALID;
}

/**
 * @brief 将数据帧序列化为原始字节数组
 * 
 * 该函数将 `zsFrame` 结构体中的数据序列化为一个字节数组，以便于传输或存储。序列化过程包括将帧的起始标志、命令码、数据长度、数据内容、校验和和结束标志转换为一个连续的字节流。
 * 
 * @param frame 指向要序列化的 `zsFrame` 结构体的指针。必须保证该指针不为 NULL，并且指向有效的 `zsFrame` 数据。
 * 
 * @return 成功时返回一个指向序列化字节数组的指针。调用者需要负责释放这个指针所占用的内存。失败时（例如内存分配失败或 `frame` 为 NULL），返回 NULL。
 * 
 * @note 
 * - 确保在调用此函数之前，`frame` 中的 `data` 成员指向有效的数据缓冲区，并且 `dataLength` 字段反映了数据的实际长度。
 * - 内存分配失败可能会导致函数返回 NULL。调用者应检查返回值以确保成功分配了内存。
 * - 函数使用 `malloc` 动态分配内存，因此调用者在使用完返回的字节数组后必须调用 `free` 释放内存。
 * 
 * @warning 
 * - 如果 `frame` 为 NULL，函数将直接返回 NULL。
 * - 函数不检查 `dataLength` 的值是否合理，假设 `dataLength` 字段在有效范围内。
 */
uint8_t* zsFrame_ParserToPlainData(zsFrame* frame,uint16_t* len){
	if (frame == NULL) {
			return NULL; // 检查空指针
	}
	
	// 计算数据部分的长度
	size_t frameLength = frame->dataLength+6; // frameStart, commandCode, dataLength, data, checksum
	uint8_t *serializedFrame = (uint8_t*)malloc(frameLength * sizeof(uint8_t));
	if (serializedFrame == NULL) {
			return NULL; // 检查内存分配是否成功
	}
  
  // 序列化帧
  serializedFrame[0] = frame->frameStart;
  serializedFrame[1] = frame->commandCode;
  serializedFrame[2] = (frame->dataLength >> 8) & 0xFF; // 高字节
  serializedFrame[3] = frame->dataLength & 0xFF;         // 低字节
  memcpy(&serializedFrame[4], frame->data, frame->dataLength);
  serializedFrame[4 + frame->dataLength] = frame->checkSum;
  // 添加帧结束标志
  serializedFrame[frameLength - 1] = frame->frameEnd;
	
	*len = frameLength;
	
	return serializedFrame;
}

/**
 * @brief 打印 zsFrame 结构体的内容
 * 
 * 打印 `zsFrame` 结构体中各个字段的值，包括起始标志、命令代码、数据长度、数据、校验和及结束标志。
 * 
 * @param frame 指向 `zsFrame` 结构体的指针
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


// ============================ 帧校验 ================================

/**
 * @brief 校验累加和
 * 
 * @param frame 指向 zsFrame 结构体的指针
 * 
 * @return 校验成功返回 1，失败返回 0
 */
uint8_t zsFrame_VerifyCheckSum(zsFrame* frame){
	uint8_t calculatedChecksum = zsFrame_GenerateCheckSum(frame);
	return calculatedChecksum == frame->checkSum;
}

/**
 * @brief 生成累加和校验值
 * 
 * @param frame 指向 zsFrame 结构体的指针
 * 
 * @return 计算出的校验和
 */
uint8_t zsFrame_GenerateCheckSum(zsFrame* frame){
	uint8_t checksum = 0;
	checksum += frame->frameStart;
	checksum += frame->commandCode;
	checksum += (uint8_t)(frame->dataLength >> 8); // 高字节
	checksum += (uint8_t)(frame->dataLength & 0xFF); // 低字节
	for (uint16_t i = 0; i < frame->dataLength; ++i) {
			checksum += frame->data[i];
	}
	checksum += frame->frameEnd;
	frame->checkSum=checksum;
	return checksum;
}


/**
 * @brief 校验帧的异或和
 * 
 * 计算帧的异或和并与帧中的校验和进行比较。
 * 
 * @param frame 指向 zsFrame 结构体的指针
 * 
 * @return 计算出的校验和是否与帧中的校验和相等，如果相等返回 1，否则返回 0
 */
uint8_t zsFrame_VerifyXorSum(zsFrame* frame){
	uint8_t calculatedChecksum = zsFrame_GenerateXorSum(frame);
	return calculatedChecksum == frame->checkSum;
}

/**
 * @brief 生成帧的异或和
 * 
 * 计算帧的异或和并更新帧中的校验和字段。
 * 
 * @param frame 指向 zsFrame 结构体的指针
 * 
 * @return 计算出的异或和
 */
uint8_t zsFrame_GenerateXorSum(zsFrame* frame){
	uint8_t checksum = 0;
	checksum ^= frame->frameStart;
	checksum ^= frame->commandCode;
	checksum ^= (uint8_t)(frame->dataLength >> 8); // 高字节
	checksum ^= (uint8_t)(frame->dataLength & 0xFF); // 低字节
	for (uint16_t i = 0; i < frame->dataLength; ++i) {
			checksum ^= frame->data[i];
	}
	checksum ^= frame->frameEnd;
	frame->checkSum=checksum;
	return checksum;
}

/**
 * @brief 验证帧的 CRC 校验
 * 
 * 计算帧的 CRC 值并与帧中的校验和进行比较，以验证帧的完整性。
 * 
 * @param frame 指向 zsFrame 结构体的指针
 * 
 * @return 如果计算出的 CRC 值与帧中的校验和相等，返回 1；否则返回 0。
 */
uint8_t zsFrame_VerifyCRC(zsFrame* frame){
	uint8_t crcRes = zsFrame_GenerateCRC(frame);
	return crcRes == frame->checkSum;
}

/**
 * @brief 生成帧的 CRC 值
 * 
 * 将帧的内容转换为一个缓冲区，计算其 CRC 值，并更新帧中的校验和字段。
 * 
 * @param frame 指向 zsFrame 结构体的指针
 * 
 * @return 计算出的 CRC 值
 */
uint8_t zsFrame_GenerateCRC(zsFrame* frame){
  uint8_t buff[sizeof(zsFrame)]; // 创建一个足够大的缓冲区
  uint8_t *ptr = buff; // 创建一个指针指向缓冲区的开始
  
  *ptr++ = frame->frameStart;
  *ptr++ = frame->commandCode;
  *ptr++ = (uint8_t)(frame->dataLength >> 8); // 假设 dataLength 是 big-endian
  *ptr++ = (uint8_t)(frame->dataLength & 0xFF);
  memcpy(ptr, frame->data, frame->dataLength); // 复制数据
  ptr += frame->dataLength; // 移动指针到数据末尾
  
  uint32_t res = zsCRC_Generate(CRC8, buff, ptr - buff); // 计算 CRC
	frame->checkSum = (uint8_t)res;
  return (uint8_t)res;
}




// ============================ CRC校验定义 =================================


uint32_t zsCRC_BitsReverse(uint32_t inVal, uint8_t bits){
	unsigned int outVal = 0;
	unsigned char i;
	for (i = 0; i < bits; i++) {
		if (inVal & (1 << i)) outVal |= 1 << (bits - 1 - i);
	}
	return outVal;
}

uint32_t zsCRC_Generate(uint8_t type, uint8_t *buf, uint32_t bufLen){
	unsigned char width  = s_crcInfoTab[type].Width; //宽度，即CRC比特数。
	unsigned int  crc    = s_crcInfoTab[type].CrcInit; //初始值,这是算法开始时寄存器（crc）的初始化预置值，十六进制表示。
	unsigned int  xorout = s_crcInfoTab[type].XorOut; //计算结果与此参数异或后得到最终的CRC值。
	unsigned char refin  = s_crcInfoTab[type].RefIn; //待测数据的每个字节是否按位反转，E_TRUE或E_FALSE。
	unsigned char refout = s_crcInfoTab[type].RefOut; //在计算后之后，异或输出之前，整个数据是否按位反转，E_TRUE或E_FALSE。
	unsigned char high;

	if (refin) { //逆序 LSB 输入
		crc = zsCRC_BitsReverse(crc, width); //init 先逆序;
		if (width > 8) { //为了减少移位等操作，width大于8和小于8的分开处理
			while (bufLen--) {
				crc = (crc >> 8) ^ s_crcTab[(crc & 0xFF) ^ *buf++];
			}
		} else {
			while (bufLen--) {
				crc = s_crcTab[crc ^ *buf++];
			}
		}
	} else { //正序 MSB 输入
		if (width > 8) { //为了减少移位等操作，width大于8和小于8的分开处理
			while (bufLen--) {
				high = crc >> (width - 8);
				crc = (crc << 8) ^ s_crcTab[high ^ *buf++];
			}
		} else {
			crc = crc << (8 - width);
			while (bufLen--) {
				crc = s_crcTab[crc ^ *buf++];
			}
			crc >>= 8 - width; //位数小于8时，crc在高width位，要右移到原位
		}
	}

	if (refout != refin) { //逆序输出
		crc = zsCRC_BitsReverse(crc, width);
	}

	crc ^= xorout; //异或输出
	return crc & ((2 << (width - 1)) - 1);
}

void zsCRC_CrcTableCalculate(uint8_t type){
	unsigned char width  = s_crcInfoTab[type].Width; //宽度，即CRC比特数。
	unsigned int  poly   = s_crcInfoTab[type].Poly; //生成多项式的简写，以16进制表示。例如：CRC-32即是0x04C11DB7，忽略了最高位的"1"，即完整的生成项是0x104C11DB7。
	unsigned char refIn  = s_crcInfoTab[type].RefIn; //待测数据的每个字节是否按位反转，E_TRUE或E_FALSE。
	unsigned int validBits = (2 << (width - 1)) - 1;
	unsigned int value;
	unsigned int bit;
	unsigned int i;
	unsigned char j;

	if (refIn) { //逆序LSB输入
		poly = zsCRC_BitsReverse(poly, width); //poly先逆序, s_crcTab 的 CrcInit = 0;
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
	} else { //正序MSB输入
		poly = (width < 8) ? (poly << (8 - width)) : poly; //如果位数小于8，poly要左移到最高位
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




