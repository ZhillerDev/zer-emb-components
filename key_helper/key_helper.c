#include "key_helper.h"


/**
 * BTN_IS_PRESSED
 * 
 * 宏定义，用于检查指定按钮是否被按下。
 * 
 * 返回值：
 * 1 - 表示按钮被按下
 * 0 - 表示按钮没有被按下
*/
#define BTN_IS_PRESSED(i) (g_btn_status_reg & (1 << i))

enum KEY_STATUS
{
    KEY_STATUS_DEFAULT = 0,
    KEY_STATUS_DOWN    = 1,
		KEY_STATUS_MULTIPLE_CLICK = 2
};

typedef uint32_t btn_type_t;

static key_t *btn_head = NULL;

btn_type_t g_logic_level = (btn_type_t)0;
btn_type_t g_btn_status_reg = (btn_type_t)0;
		
static uint8_t button_cnt = 0;

/**
 * @brief Register a user button
 * 
 * @param button: button structure instance
 * @return Number of keys that have been registered, or -1 when error
*/
int32_t key_register(key_t *button)
{
    key_t *curr = btn_head;
    
    if (!button || (button_cnt > sizeof(btn_type_t) * 8))
    {
        return -1;
    }

    while (curr)
    {
        if(curr == button)
        {
            return -1;  /* already exist. */
        }
        curr = curr->next;
    }

    /**
     * First registered button is at the end of the 'linked list'.
     * btn_head points to the head of the 'linked list'.
    */
    button->next = btn_head;
    button->status = KEY_STATUS_DEFAULT;
    button->event = KEY_EVT_NONE;
    button->scan_cnt = 0;
    button->click_cnt = 0;
    button->max_multiple_clicks_interval = KEY_CONFIG_MAX_MULTIPLE_CLICKS_INTERVAL;
    btn_head = button;

    /**
     * First registered button, the logic level of the button pressed is 
     * at the low bit of g_logic_level.
    */
    g_logic_level |= (button->pressed_logic_level << button_cnt);
    button_cnt ++;

    return button_cnt;
}

static void key_read(void)
{
    uint8_t i;
    key_t* target;

    /* The button that was registered first, the button value is in the low position of raw_data */
    btn_type_t raw_data = 0;

    for(target = btn_head, i = button_cnt - 1;
        (target != NULL) && (target->usr_button_read != NULL);
        target = target->next, i--)
    {
        raw_data = raw_data | ((target->usr_button_read)(target) << i);
    }

    g_btn_status_reg = (~raw_data) ^ g_logic_level;
}

static uint8_t key_process(void)
{
    uint8_t i;
    uint8_t active_btn_cnt = 0;
    key_t* target;
    
    for (target = btn_head, i = button_cnt - 1; target != NULL; target = target->next, i--)
    {
        if (target->status > KEY_STATUS_DEFAULT)
        {
            target->scan_cnt ++;
            if (target->scan_cnt >= ((1 << (sizeof(target->scan_cnt) * 8)) - 1))
            {
                target->scan_cnt = target->long_hold_start_tick;
            }
        }

        switch (target->status)
        {
        case KEY_STATUS_DEFAULT: /* stage: default(button up) */
            if (BTN_IS_PRESSED(i)) /* is pressed */
            {
                target->scan_cnt = 0;
                target->click_cnt = 0;

                EVENT_SET_AND_EXEC_CB(target, KEY_STATUS_DOWN);

                /* swtich to button down stage */
                target->status = KEY_STATUS_DOWN;
            }
            else
            {
                target->event = KEY_EVT_NONE;
            }
            break;

        case KEY_STATUS_DOWN: /* stage: button down */
            if (BTN_IS_PRESSED(i)) /* is pressed */
            {
                if (target->click_cnt > 0) /* multiple click */
                {
                    if (target->scan_cnt > target->max_multiple_clicks_interval)
                    {
                        EVENT_SET_AND_EXEC_CB(target, 
                            target->click_cnt < KEY_EVT_REPEAT_CLICK ? 
                                target->click_cnt :
                                KEY_EVT_REPEAT_CLICK);

                        /* swtich to button down stage */
                        target->status = KEY_STATUS_DOWN;
                        target->scan_cnt = 0;
                        target->click_cnt = 0;
                    }
                }
                else if (target->scan_cnt >= target->long_hold_start_tick)
                {
                    if (target->event != KEY_EVT_LONG_HOLD)
                    {
                        EVENT_SET_AND_EXEC_CB(target, KEY_EVT_LONG_HOLD);
                    }
                }
                else if (target->scan_cnt >= target->long_press_start_tick)
                {
                    if (target->event != KEY_EVT_LONG_START)
                    {
                        EVENT_SET_AND_EXEC_CB(target, KEY_EVT_LONG_START);
                    }
                }
                else if (target->scan_cnt >= target->short_press_start_tick)
                {
                    if (target->event != KEY_EVT_SHORT_START)
                    {
                        EVENT_SET_AND_EXEC_CB(target, KEY_EVT_SHORT_START);
                    }
                }
            }
            else /* button up */
            {
                if (target->scan_cnt >= target->long_hold_start_tick)
                {
                    EVENT_SET_AND_EXEC_CB(target, KEY_EVT_LONG_HOLD_UP);
                    target->status = KEY_STATUS_DEFAULT;
                }
                else if (target->scan_cnt >= target->long_press_start_tick)
                {
                    EVENT_SET_AND_EXEC_CB(target, KEY_EVT_LONG_UP);
                    target->status = KEY_STATUS_DEFAULT;
                }
                else if (target->scan_cnt >= target->short_press_start_tick)
                {
                    EVENT_SET_AND_EXEC_CB(target, KEY_EVT_SHORT_UP);
                    target->status = KEY_STATUS_DEFAULT;
                }
                else
                {
                    /* swtich to multiple click stage */
                    target->status = KEY_STATUS_MULTIPLE_CLICK;
                    target->click_cnt ++;
                }
            }
            break;

        case KEY_STATUS_MULTIPLE_CLICK: /* stage: multiple click */
            if (BTN_IS_PRESSED(i)) /* is pressed */
            {
                /* swtich to button down stage */
                target->status = KEY_STATUS_DOWN;
            }
            else
            {
                if (target->scan_cnt > target->max_multiple_clicks_interval)
                {
                    EVENT_SET_AND_EXEC_CB(target, 
                        target->click_cnt < KEY_EVT_REPEAT_CLICK ? 
                            target->click_cnt :
                            KEY_EVT_REPEAT_CLICK);

                    /* swtich to default stage */
                    target->status = KEY_STATUS_DEFAULT;
                }
            }
            break;
        }
        
        if (target->status > KEY_STATUS_DEFAULT)
        {
            active_btn_cnt ++;
        }
    }
    
    return active_btn_cnt;
}

key_evt_t key_evt_read(key_t* button)
{
    return (key_evt_t)(button->event);
}

uint8_t key_scan(void)
{
    key_read();
    return key_process();
}

