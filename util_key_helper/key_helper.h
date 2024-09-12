#ifndef __KEY_HELPER_H__
#define __KEY_HELPER_H__


#include "stdint.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"


/// 按键配置 ///

#define KEY_CONFIG_MS_TO_COUNT(ms) (ms/20) // 将毫秒值转换为计算的数值

#define KEY_CONFIG_MAX_BTNS 32 // 最大可追踪的按钮数量不可以超过32个

#define KEY_CONFIG_MAX_MULTIPLE_CLICKS_INTERVAL (KEY_CONFIG_MS_TO_COUNT(200))// 多次点击的时间间隔，ms单位


/// 按钮参数定义 ///

#ifndef NULL
#define NULL 0
#endif

typedef void (*key_resp_cb)(void*);

typedef enum {
	KEY_EVT_DOWN = 0,
	KEY_EVT_CLICK,
	KEY_EVT_DOUBLE_CLICK,
	KEY_EVT_REPEAT_CLICK,
	KEY_EVT_SHORT_START,
	KEY_EVT_SHORT_UP,
	KEY_EVT_LONG_START,
	KEY_EVT_LONG_UP,
	KEY_EVT_LONG_HOLD,
	KEY_EVT_LONG_HOLD_UP,
	KEY_EVT_MAX,
	KEY_EVT_NONE,
} key_evt_t;

typedef struct _key_t
{
    struct _key_t* next;

    uint8_t  (*usr_button_read)(void *);
    key_resp_cb  cb;

    uint16_t scan_cnt;
    uint16_t click_cnt;
    uint16_t max_multiple_clicks_interval;

    uint16_t debounce_tick;
    uint16_t short_press_start_tick;
    uint16_t long_press_start_tick;
    uint16_t long_hold_start_tick;

    uint8_t id;
    uint8_t pressed_logic_level : 1;
    uint8_t event               : 4;
    uint8_t status              : 3;
} key_t;


#define EVENT_SET_AND_EXEC_CB(btn, evt)                                        \
    do                                                                         \
    {                                                                          \
        btn->event = evt;                                                      \
        if(btn->cb)                                                            \
            btn->cb((key_t*)btn);                                      \
    } while(0)


/// 按钮主要定义 ///
		
#ifdef __cplusplus
extern "C" {
#endif

int32_t key_register(key_t *button);
key_evt_t key_evt_read(key_t* button);
uint8_t key_scan(void);
	
void zUtil_Key_Init(void);
void zUtil_Key_Scan(void);

#ifdef __cplusplus
}
#endif  

#endif
