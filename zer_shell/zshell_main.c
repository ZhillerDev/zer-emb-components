/*
 *-------------------------------------------------------------------------
 * Author:    zhiller <zhiyiyimail@163.com>
 * Version:   v0.0.1
 * Brief:			Shell������ȫ������
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

// ============================ ��д printf =================================

#if 1             
#pragma import(__use_no_semihosting) // ���ð�����ģʽ��ͨ������Ƕ��ʽϵͳ�ĵ��ԡ�
// ����FILE�ṹ�壬ͨ�����ڱ�׼��������⡣
struct __FILE 
{ 
    int handle; // �ļ���������ڱ�ʶ�ļ���
}; 

FILE __stdout;   // ������׼�������ͨ������printf�Ⱥ�����

// �ض���_sys_exit()��������ֹ�����˳�ʱ����ϵͳ�˳�������
void _sys_exit(int x) 
{ 
    x = x;     // �˴�x����ֵ����ʹ�ã�������Ϊ�˱�����������档
} 

// �ض���fputc���������ڱ�׼�����
int fputc(int ch,FILE *f)
{
	HAL_UART_Transmit(uart_device[UART_DEFAULT_PRINTF].uart, (uint8_t *)&ch, 1, 0xFFFF);
	return ch;
}
#endif


// ============================ UART ���� =================================

/**
 * @brief ����һ���µ� UART �豸ʵ����
 * 
 * �˺������ڳ�ʼ��������һ���µ� UART �豸ʵ����������ݴ����ģʽ�����豸�Ĺ���ģʽ����Ϊ�豸����һ�����λ�����������������µ�ǰ UART �豸��״̬�����������Ϣ��
 * 
 * @param mode �豸�Ĺ���ģʽ������ʵ����Ҫ�����벻ͬ��ģʽֵ��
 * @param uart ָ�� UART_HandleTypeDef �ṹ���ָ�룬��ʾҪ���õ� UART ���衣��ȷ����ָ��ָ��һ����Ч�� UART �����
 * 
 * @note �˺��������ȫ�ֱ��� `zsCurrentUartDevice`���ñ������ڸ��ٵ�ǰ�� UART �豸������
 * 
 * @warning ȷ�� `zsCurrentUartDevice` �ڵ��øú���֮ǰ����ȷ��ʼ��������Ч��Χ�ڡ����򣬿��ܻᵼ������Խ���δ������Ϊ��
 * 
 * @pre �����ȵ��� `zsRingBuff_create` ��������ʼ�����λ�������
 * 
 * @post �ں������ú�`uart_device` �����е��豸ʵ�����������Ӧ��������Ϣ������ `zsCurrentUartDevice` ��������ָ����һ�����õ��豸�ۡ�
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
 * @brief ��ʼ��UARTͨ��
 *
 * �ú��������ʼ��UARTͨ�ţ���������UART��ص�Ӳ��������
 * �粨���ʡ��ֳ���ֹͣλ�ȡ����������ZSHELL_USE_RINGBUFF�꣬
 * �򻹻ᴴ��һ�����λ��������ڽ������ݣ���������DMA���ա�
 *
 * @note �ú�������UART��DMA�Ѿ��������ط������˻�����Ӳ����ʼ����
 */
void zsUart_Init(){
	if(zsCurrentUartDevice==0) return;
	for(int i=0;i<zsCurrentUartDevice;i++){
		HAL_UARTEx_ReceiveToIdle_DMA(uart_device[i].uart,uart_device[i].ringBuff->tempBuff,RING_BUFF_TEMP_SIZE);	
	}
}


/**
 * @brief ����UART����״̬������
 *
 * ���ݻ��λ�������״̬��ִ����Ӧ�Ĵ�������
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
 * @brief ͨ�� UART ��������
 * 
 * �ú���ʹ�� DMA ��ʽ�����ݷ��͵�ָ���� UART �豸��������ȴ�ֱ�����ݴ�����ɡ�
 * 
 * @param id UART �豸������������Ӧ������Ч��Χ�ڡ�
 * @param data ָ����������ݵ�ָ�룬���ݵ�����Ϊ uint8_t ���顣
 * 
 * @note �ú������� `uart_device` ��һ������ UART �豸���õ�ȫ�����顣
 *       ȷ���ڵ��ô˺���ǰ����ȷ��ʼ�� UART �豸��
 */
void zsUart_Send(uint8_t id,uint8_t* data){
	size_t len = strlen((char*)data);
	HAL_UART_Transmit_DMA(uart_device[id].uart,data,len);
	while (__HAL_UART_GET_FLAG(uart_device[id].uart, UART_FLAG_TC) == RESET);
	// ���������ɱ�־
	__HAL_UART_CLEAR_FLAG(uart_device[id].uart, UART_FLAG_TC);	
}

/**
 * @brief ͨ�� UART ���͸�ʽ������
 * 
 * �ú���ʹ�� DMA ��ʽ����ʽ�������ݷ��͵�ָ���� UART �豸��������ȴ�ֱ�����ݴ�����ɡ�
 * 
 * @param id UART �豸������������Ӧ������Ч��Χ�ڡ�
 * @param data ��ʽ���ַ����ĸ�ʽ�������� printf �ĸ�ʽ���ַ�����
 * @param ... �䳤���������ݸ�ʽ���ַ����ṩ��Ӧ�����ݡ�
 * 
 * @note �ú���ʹ�� `vsnprintf` ����ʽ���ַ�����ȷ����ʽ������ַ����ܹ���Ӧ��������С��
 *       ������������㣬��������ջ����������ء�
 *       ȷ���ڵ��ô˺���ǰ����ȷ��ʼ�� UART �豸��
 */
void zsUart_SendFormat(uint8_t id,uint8_t* data,...){
  va_list args;
  int length;
	uint8_t buffer[UART_PRINT_BUFFER_SIZE];
  
  if (buffer == NULL || data == NULL) return;
  
  // ��ʼ�� va_list
  va_start(args, data);
  
  // ʹ�� vsnprintf ����ʽ���ַ���
  length = vsnprintf((char *)buffer, UART_PRINT_BUFFER_SIZE, (char*)data, args);
  
  // ���� va_list
  va_end(args);
  
  // ȷ���ַ����� null ��β
  if (length > 0 && length < UART_PRINT_BUFFER_SIZE) {
    buffer[length] = '\0';
  } else {
    // ���������̫С���޷����������ַ���
    memset(buffer, 0, UART_PRINT_BUFFER_SIZE);
    length = -1;
  }
	
	HAL_UART_Transmit_DMA(uart_device[id].uart,buffer,length+1);
	while (__HAL_UART_GET_FLAG(uart_device[id].uart, UART_FLAG_TC) == RESET);
	// ���������ɱ�־
	__HAL_UART_CLEAR_FLAG(uart_device[id].uart, UART_FLAG_TC);	
	return;
}

void zsUart_SendFrame(uint8_t id,uint8_t code,uint8_t* data){
	zsFrame* frame=(zsFrame*)malloc(sizeof(zsFrame));
	uint16_t len=0;
	
	zsFrame_Builder(frame,code,data);
	uint8_t* output = zsFrame_ParserToPlainData(frame,&len);
	
	// Ϊÿ���ֽڵ�ʮ�����Ʊ�ʾ�����㹻�Ŀռ�
	// ÿ���ֽ���Ҫ2���ַ� + 1���ո񣬼������Ŀ��ַ�
	uint8_t* hexString = (uint8_t*)malloc((len * 3 + 1) * sizeof(uint8_t));
	if (hexString == NULL) {
			free(frame);
			free(output);
			return;
	}

	// ���ֽ�ת��Ϊʮ�������ַ���
	size_t hexPos = 0;
	for (size_t i = 0; i < len; ++i) {
		hexPos += snprintf((char*)&hexString[hexPos], 3, "%02X", output[i]);
	}
	hexString[hexPos] = '\0'; // ȷ���ַ����Կ��ַ���β
	
	HAL_UART_Transmit_DMA(uart_device[id].uart,hexString,len*3+1);
	while (__HAL_UART_GET_FLAG(uart_device[id].uart, UART_FLAG_TC) == RESET);
	// ���������ɱ�־
	__HAL_UART_CLEAR_FLAG(uart_device[id].uart, UART_FLAG_TC);	
	
	free(frame);
	free(output);
	free(hexString);
	return;
}


// ============================ UART �ص���ISR���� =================================

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
 * @brief UART������ɻص�����
 * 
 * ��UART����DMA�������ʱ���˻ص����������á������𽫽��յ�������
 * д�뻷�λ��������������λ�������״̬��
 * 
 * @param huart: ָ��UART_HandleTypeDef�ṹ���ָ�룬����UART������Ϣ
 * @param Size: ���յ������ݴ�С
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
		
			// ���������飬����֡У����߶���֡
			zs_uart_ringbuff_data_process(i);
		}
		
		memset(uart_device[i].ringBuff->tempBuff,0,RING_BUFF_TEMP_SIZE);
		HAL_UARTEx_ReceiveToIdle_DMA(uart_device[i].uart,uart_device[i].ringBuff->tempBuff,RING_BUFF_TEMP_SIZE);
	}
	
	
}

/**
 * @brief UART����ص�����
 * 
 * ��UART��������ʱ���˻ص����������á����������û��λ�������
 * �����ʱ������������������DMA���ա�
 * 
 * @param huart: ָ��UART_HandleTypeDef�ṹ���ָ�룬����UART������Ϣ
 */
void HAL_UART_ErrorCallback(UART_HandleTypeDef * huart)
{
	for(int i=0;i<zsCurrentUartDevice;i++){
		zsUartDevice* device = &uart_device[i]; 
		if(huart==device->uart){
			memset(device->ringBuff->tempBuff, 0, sizeof(device->ringBuff->tempBuff));
			HAL_UARTEx_ReceiveToIdle_DMA(&huart1, device->ringBuff->tempBuff, RING_BUFF_TEMP_SIZE);
			__HAL_DMA_DISABLE_IT(&hdma_usart1_rx, DMA_IT_HT);		   // �ֶ��ر�DMA_IT_HT�ж�
			printf("���ڷ��������ˣ����� \r\n");
			break;
		}
	}
}



// ============================ RingBuff ���λ��������� =================================

/**
 * @brief �ӻ��λ�������ȡָ�����ȵ����ݲ�ͨ��UART����
 *
 * �ú����ӻ��λ������ж�ȡָ�����ȵ����ݣ���ͨ��UARTʹ��DMA��ʽ���͡�
 * ������ɺ�����ȴ�ֱ��������ɱ�־�����ã�Ȼ������ñ�־��
 * ����ͷ����ڴ洢���ݵ���ʱ�ڴ档
 *
 * @param len Ҫ��ȡ�����͵����ݳ���
 */
void zsUart_RingBuffPrintLen(uint16_t len){
//	u8* temp=(u8*)malloc(len);
//	zsRingBuff_readLen(&ringBuff,temp,len);
//	HAL_UART_Transmit_DMA(&UART_CURRENT,temp,len);
//	while (__HAL_UART_GET_FLAG(&UART_CURRENT, UART_FLAG_TC) == RESET);
//	// ���������ɱ�־
//	__HAL_UART_CLEAR_FLAG(&UART_CURRENT, UART_FLAG_TC);	
//	free(temp);
}

/**
 * @brief �ӻ��λ�������ȡ�������ݲ�ͨ��UART����
 *
 * �ú�����ȡ���λ�������������д������ݳ��ȣ�������Ӧ��С���ڴ棬
 * �ӻ��λ������ж�ȡ���ݣ���ͨ��UARTʹ��DMA��ʽ������Щ���ݡ�
 * ������ɺ�����ȴ�ֱ��������ɱ�־�����ã�Ȼ������ñ�־��
 * ����ͷ����ڴ洢���ݵ���ʱ�ڴ档
 */
void zsUart_RingBuffPrintAll(void){
//	uint16_t len = zsRingBuff_getWriteLen(&ringBuff);
//	u8* temp=(u8*)malloc(len);
//	zsRingBuff_readLen(&ringBuff,temp,len);
//	HAL_UART_Transmit_DMA(&UART_CURRENT,temp,len);
//	while (__HAL_UART_GET_FLAG(&UART_CURRENT, UART_FLAG_TC) == RESET);
//	// ���������ɱ�־
//	__HAL_UART_CLEAR_FLAG(&UART_CURRENT, UART_FLAG_TC);	
//	free(temp);
}



// ============================ Console �����п��� =================================

// ����һ�������ַ�����������������ʾ��
static uint8_t* zs_console_name = (uint8_t*)zs_cfgCONSOLE_DEFAULE_NAME;

// ����һ����ά���飬���ڴ洢�ָ��������в���
static uint8_t zs_console_argv_arr[zs_cfgCONSOLE_MAX_ARGV][zs_cfgCONSOLE_MAX_ARGV_LEN] = {0};

// ����һ�����������ڽ����յ����ַ����ָ�ɶ������
static uint16_t zs_console_string_split(uint8_t *string, uint16_t len, uint8_t *argv, uint16_t size) {	
    uint16_t argc = 0;  // ����������
    uint8_t *ch = malloc(len + 1);  // ��̬�����ڴ棬���ڴ洢���Ƶ��ַ�����+1 ���� '\0'��
    
    if (ch == NULL) {
        // �ڴ����ʧ�ܣ�������󣨿����Ǵ�ӡ������Ϣ����������
        return 0;
    }

    memcpy(ch, string, len);  // �����ַ�����������ڴ�
    ch[len] = '\0';  // ����ַ���������

    uint8_t *str = (uint8_t*) strtok((char *) ch, " ");  // ʹ�� strtok �ָ��ַ���
    
    while (str != NULL) {
        // ȷ���������ᳬ������߽�
        if (argc < zs_cfgCONSOLE_MAX_ARGV) {
            strncpy((char *)argv + (argc * size), (char *)str, size - 1);  // ���Ʒָ��Ĳ����� argv ����
            ((char *)argv)[(argc * size) + size - 1] = '\0';  // ȷ��ÿ�������� '\0' ��β
            argc++;  // ���Ӳ�������
        } else {
            break;  // ��������������������˳�ѭ��
        }
        str = (uint8_t*)strtok(NULL, " ");  // ��ȡ��һ������
    }

    free(ch);  // �ͷŶ�̬������ڴ�
    return argc;  // ���ز���������
}

void zs_console_cmd_layer1(uint8_t id){
	if (strcmp((char *) zs_console_argv_arr[0], "zs") == 0) {
		zsUart_Send(id,"zShell �����ֲ� (1/1) \r\n");
		zsUart_Send(id,"zs info - �鿴shell��ϸ��Ϣ \r\n");
		zsUart_Send(id,"zs version - �鿴shell�汾��Ϣ \r\n");
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



// ============================ Frame �Զ���֡���� =================================

void zsConsole_FrameProcess(uint8_t id,uint8_t* data,uint16_t len){
	
}

