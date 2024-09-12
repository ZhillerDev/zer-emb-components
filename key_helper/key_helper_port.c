#include "key_helper.h"

#include "gpio.h"
#include "pins.h"

#include "stm32f4xx_hal.h"
#include "stm32f4xx.h"


/// 用户注册的按键 ///

#define KEY0_READ        HAL_GPIO_ReadPin(GPIOE,GPIO_PIN_4) 
#define KEY1_READ        HAL_GPIO_ReadPin(GPIOE,GPIO_PIN_3)  
#define KEY2_READ        HAL_GPIO_ReadPin(GPIOE,GPIO_PIN_2) 

typedef enum {
	USER_BUTTON_0 = 0,
	USER_BUTTON_1,
	USER_BUTTON_2,
	USER_BUTTON_MAX
} user_key_t;

static key_t user_keys[USER_BUTTON_MAX];


/// 按键读取与回调函数 ///

static uint8_t common_key_read(void *arg){
	uint8_t value = 0;

	key_t *btn = (key_t *)arg;

	switch (btn->id)
	{
	case USER_BUTTON_0:
			value = KEY0_READ;
			break;
	case USER_BUTTON_1:
			value = KEY1_READ;
			break;
	case USER_BUTTON_2:
			value = KEY2_READ;
			break;
	default:
			break;
	}

	return value;
}

static void common_key_evt_cb(void *arg){
//	key_t *btn = (key_t *)arg;
	
	key_evt_t k1 = key_evt_read(&user_keys[USER_BUTTON_0]);
	key_evt_t k2 = key_evt_read(&user_keys[USER_BUTTON_1]);
	key_evt_t k3 = key_evt_read(&user_keys[USER_BUTTON_2]);

	if(k1==KEY_EVT_SHORT_START){
		LED0=!LED0;
	}
//	if(k2==KEY_EVT_LONG_HOLD){
//		LED0=!LED0;
//	}
}

/// 按键扫描与初始化 ///

void user_key_scan(void){
	key_scan();
	HAL_Delay(20); // 固定1ms扫描一次按键
}

void user_key_init(void){
	int i;
	memset(&user_keys[0],0x0,sizeof(key_t)*USER_BUTTON_MAX);
	for (i = 0; i < USER_BUTTON_MAX; i++)
	{
		user_keys[i].id = i;
		user_keys[i].usr_button_read = common_key_read;
		user_keys[i].cb = common_key_evt_cb;
		user_keys[i].pressed_logic_level = 0;
		user_keys[i].short_press_start_tick = KEY_CONFIG_MS_TO_COUNT(200);
		user_keys[i].long_press_start_tick = KEY_CONFIG_MS_TO_COUNT(1000);
		user_keys[i].long_hold_start_tick = KEY_CONFIG_MS_TO_COUNT(1500);

//		if (i == USER_BUTTON_2)
//		{
//				user_keys[USER_BUTTON_2].pressed_logic_level = 1;
//		}

		key_register(&user_keys[i]);
	}
}



