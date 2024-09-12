# 按键控制

一般的，按键检测通常会有以下三种方式实现：

1. 轮询：占用 CPU，效率低，需要延时消抖
2. 中断：CPU 资源占用少，但是不同主控之间的硬件移植复杂，且多个按键需要多个 NVIC 中断，但是效率较高，同样也需要消抖
3. 状态机：CPU 资源占用少，支持多状态甚至组合状态操作，同时不需要额外设置消抖环节（因为状态机的轮转实际上就可以充当消抖环节了）

## 状态机按键检测

> 参考文献：
>
> [STM32 实现按键有限状态机（超详细，易移植）\_单片机\_PlayCodes-开放原子开发者工作坊 (csdn.net)](https://openatomworkshop.csdn.net/664ee3efb12a9d168eb70cd2.html?dp_token=eyJ0eXAiOiJKV1QiLCJhbGciOiJIUzI1NiJ9.eyJpZCI6NjkyMzMzLCJleHAiOjE3MjY3MjM1NTEsImlhdCI6MTcyNjExODc1MSwidXNlcm5hbWUiOiJkZWxldGVfeW91In0.VGM6EPGF3emXjGasx15HYGLZ2_oO23mlY7UbSOj051I&spm=1001.2101.3001.6661.1&utm_medium=distribute.pc_relevant_t0.none-task-blog-2%7Edefault%7Ebaidujs_utm_term%7Eactivity-1-119787933-blog-132571731.235%5Ev43%5Epc_blog_bottom_relevance_base3&depth_1-utm_source=distribute.pc_relevant_t0.none-task-blog-2%7Edefault%7Ebaidujs_utm_term%7Eactivity-1-119787933-blog-132571731.235%5Ev43%5Epc_blog_bottom_relevance_base3&utm_relevant_index=1)
>
> [【源码详解~按键状态机~简洁易懂】1.单个按键实现短按长按的功能（基于 STM32）\_stm32 按键长按短按双击-CSDN 博客](https://blog.csdn.net/qq_44078824/article/details/123753825)
>
> [FlexibleButton: FlexibleButton 是一个基于标准 C 语言的小巧灵活的按键处理库，支持单击、连击、短按、长按、自动消抖，可以自由设置组合按键，可用于中断和低功耗场景 (gitee.com)](https://gitee.com/RT-Thread-Mirror/FlexibleButton)

个人移植成品代码库：[zer_emb_components/key_helper at main · ZhillerDev/zer_emb_components (github.com)](https://github.com/ZhillerDev/zer_emb_components/tree/main/key_helper)

这里基于 flexible button 项目做了一个简单的移植，并重构了一些冗余的命名方法，调整了大体的代码结构，这里将详细讲述如何实现以下功能：

1. 单击、连击、短按、长按、消抖诸多功能
2. 状态机管理按键功能切换
3. 统一监听器 listener 回调函数管理，避免构建多个回调函数

这里我们划分为三个文件，各个文件的作用是：

1. key_helper.h 按键库头文件：一些按键配置与函数变量定义
2. key_helper.c 按键库 c 文件
3. key_helper_port.c 按键库可移植文件，用户需要实现此文件内的函数，来对按键检测进行控制欲移植

### 按键核心库

第一步：配置头文件

<details class="lake-collapse"><summary id="ua152be03"><span class="ne-text">key_helper.h</span></summary><h3 id="if05P"><span class="ne-text">主配置项</span></h3><p id="u518c3cba" class="ne-p"><span class="ne-text">为避免再创建一个配置头文件的麻烦，这里直接把配置项写在可核心库头文件里面；</span></p><p id="u2cdaf296" class="ne-p"><code class="ne-code"><span class="ne-text">KEY_CONFIG_MS_TO_COUNT(ms) (ms/20)</span></code><span class="ne-text"> 指的是将输入的毫秒值转换为计数值，可能你不太理解，我这边给你说一下他的工作原理吧：</span></p><ol class="ne-ol"><li id="u56145f6b" data-lake-index-type="0"><span class="ne-text">因为该按键检测框架使用的是查询模式，也就是说需要在主循环内每20ms扫描一下按键，判断是否有新的按键触发了，并且执行相对应的事件</span></li><li id="u86986392" data-lake-index-type="0"><span class="ne-text">那么每次调用一次按键扫描，都会令按键结构体key_t内的scan_cnt++，所以说我们需要使用这个宏，把ms转换为计数值，这样才可以让代码正常运行</span></li></ol><pre data-language="cpp" id="duk3P" class="ne-codeblock language-cpp"><code>#include &quot;stdint.h&quot;
#include &quot;stdlib.h&quot;
#include &quot;stdio.h&quot;
#include &quot;string.h&quot;

/// 按键配置 ///

#define KEY_CONFIG_MS_TO_COUNT(ms) (ms/20) // 将毫秒值转换为计算的数值

#define KEY_CONFIG_MAX_BTNS 32 // 最大可追踪的按钮数量不可以超过 32 个

#define KEY_CONFIG_MAX_MULTIPLE_CLICKS_INTERVAL (KEY_CONFIG_MS_TO_COUNT(200))// 多次点击的时间间隔，ms 单位</code></pre><p id="ubcf5fdb9" class="ne-p"><br></p><h3 id="IKWPy"><span class="ne-text">按键事件枚举</span></h3><p id="ua8884eef" class="ne-p"><span class="ne-text">众所周知，一个完整的状态机会包括：状态、事件、条件，三大部分；</span></p><p id="u006d6c07" class="ne-p"><code class="ne-code"><span class="ne-text">key_evt_t</span></code><span class="ne-text"> 枚举类定义了所有可以被触发的按键事件，那么一般情况开发中只会使用到以下几种状态：</span></p><ol class="ne-ol"><li id="u681e9147" data-lake-index-type="0"><span class="ne-text">KEY_EVT_DOUBLE_CLICK：双击事件</span></li><li id="uab295e00" data-lake-index-type="0"><span class="ne-text">KEY_EVT_SHORT_START：单击事件（这里实际上是检测按键开始按下后的一个短延迟时间，一般直接使用此状态来表示单击事件）</span></li><li id="u5d2c8412" data-lake-index-type="0"><span class="ne-text">KEY_EVT_LONG_START：长按事件</span></li><li id="u983ee809" data-lake-index-type="0"><span class="ne-text">KEY_EVT_LONG_HOLD：持续按住不撒手事件</span></li></ol><div class="ne-quote"><p id="uaf35d088" class="ne-p"><span class="ne-text">一般不会使用诸如 KEY_EVT_LONG_UP 这样的按键弹起检测事件，因为我们总是会需要用户点击后立刻触发反应，而不是用户撒手后才触发回调函数</span></p></div><pre data-language="cpp" id="CXe1W" class="ne-codeblock language-cpp"><code>// 这个宏定义是为了确保在整个项目中 NULL 具有一致的定义。
// 如果 NULL 还没有被定义，那么这里将其定义为 0。
#ifndef NULL
#define NULL 0
#endif

// 定义了一个函数指针类型 key*resp_cb，该类型的函数接受一个 void 指针作为参数，并且没有返回值。
// 这个类型通常用于回调函数，允许用户定义一个函数来响应特定的事件。
typedef void (\_key_resp_cb)(void*);

// 定义了一个枚举类型 key*evt_t，用于表示不同的按键事件。
// 每个枚举值代表一个特定的按键动作或状态，这些状态可以被用来触发相应的事件处理函数。
typedef enum {
KEY_EVT_DOWN = 0, // 按键被按下的事件
KEY_EVT_CLICK, // 按键单击事件，通常在按键被快速按下然后释放时触发
KEY_EVT_DOUBLE_CLICK, // 按键双击事件，通常在用户快速连续点击两次时触发
KEY_EVT_REPEAT_CLICK, // 按键重复点击事件，当用户持续快速点击同一个按键时触发
KEY_EVT_SHORT_START, // 短按开始事件，当按键按下达到预设的短按时长阈值时触发
KEY_EVT_SHORT_UP, // 短按释放事件，当短按结束后按键被释放时触发
KEY_EVT_LONG_START, // 长按开始事件，当按键按下达到预设的长按时长阈值时触发
KEY_EVT_LONG_UP, // 长按释放事件，当长按结束后按键被释放时触发
KEY_EVT_LONG_HOLD, // 长按保持事件，当按键持续被按下超过长按开始事件后一段时间时触发
KEY_EVT_LONG_HOLD_UP, // 长按保持释放事件，当长按按键被释放时触发
KEY_EVT_MAX, // 按键事件的最大值，用于迭代或数组大小定义
KEY_EVT_NONE, // 无按键事件，用于初始化或表示按键没有事件发生
} key_evt_t;</code></pre><p id="u8f082785" class="ne-p"><br></p><h3 id="ncSdB"><span class="ne-text">按键结构体</span></h3><div class="ne-quote"><p id="u3e9f08bc" class="ne-p"><span class="ne-text">很关键，所有的按键都从这里被初始化</span></p><p id="ua68761df" class="ne-p"><span class="ne-text">只要你细心看懂下面的所有解释，你基本就能理解 70%的按键库内容了</span></p></div><p id="u2ddde373" class="ne-p"><span class="ne-text">下面将针对该按键结构体的定义分析几个重难关键点：</span></p><ol class="ne-ol"><li id="u8337f6b3" data-lake-index-type="0"><code class="ne-code"><span class="ne-text">struct \_key_t* next;</span></code><span class="ne-text"> 将按键定义为一个链表，通过遍历该按键链表，实现至高 32 个按键的状态检测；</span></li><li id="u8f6460ae" data-lake-index-type="0"><code class="ne-code"><span class="ne-text">uint8_t (*usr_button_read)(void *)</span></code><span class="ne-text"> 获取按键电平状态的函数，因为每个 MCU 的架构不一样，提供的代码库函数也不一样，但是我们需要在该函数内统一返回指定 GPIO 口的电平状态（比如 STM32 HAL 库就使用 HAL*GPIO_ReadPin 读取引脚电平状态）</span></li><li id="uf8dd8e47" data-lake-index-type="0"><code class="ne-code"><span class="ne-text">key_resp_cb cb</span></code><span class="ne-text"> 当按键事件变化的时候就会调用这个函数</span></li><li id="u87eb9e91" data-lake-index-type="0"><code class="ne-code"><span class="ne-text">scan_cnt</span></code><span class="ne-text"> 扫描计数器，每次调用一次按键扫描函数时，就会令其自增 1，表示一次扫描</span></li><li id="u0063aa49" data-lake-index-type="0"><code class="ne-code"><span class="ne-text">click_cnt</span></code><span class="ne-text"> 点按计数器，默认为 0，如果变成了 1 及以上，就表示这是连点！！！</span></li><li id="u31e1b4f7" data-lake-index-type="0"><code class="ne-code"><span class="ne-text">max_multiple_clicks_interval</span></code><span class="ne-text"> 连点的触发间隔时间</span></li><li id="ud9a274bf" data-lake-index-type="0"><code class="ne-code"><span class="ne-text">pressed_logic_level</span></code><span class="ne-text"> 该按键按下状态的电平是什么？（假设你的单片机按键是默认上拉的，低电平触发，那么这里就写 0，因为按下状态的电平是低电平！！！）</span></li></ol><pre data-language="cpp" id="j40LC" class="ne-codeblock language-cpp"><code>// 定义了一个结构体类型\_key_t，用于表示按键的状态和行为。
typedef struct \_key_t
{
// 指向下一个按键结构体的指针，用于构建按键链表。
struct \_key_t* next;

    // 用户定义的函数指针，用于读取按键的状态（按下或未按下）。
    uint8_t  (*usr_button_read)(void *);

    // 按键事件回调函数，当按键状态发生变化时调用。
    key_resp_cb  cb;

    // 扫描计数器，用于去抖动和检测按键事件。
    uint16_t scan_cnt;

    // 点击计数器，用于检测多次点击事件。
    uint16_t click_cnt;

    // 多次点击事件的最大间隔时间（毫秒）。
    uint16_t max_multiple_clicks_interval;

    // 去抖动计数器，用于消除按键的抖动。
    uint16_t debounce_tick;

    // 短按开始的阈值（毫秒），按键按下达到这个时间后视为短按开始。
    uint16_t short_press_start_tick;

    // 长按开始的阈值（毫秒），按键按下达到这个时间后视为长按开始。
    uint16_t long_press_start_tick;

    // 长按保持的阈值（毫秒），按键持续按下达到这个时间后视为长按保持。
    uint16_t long_hold_start_tick;

    // 按键的唯一标识符。
    uint8_t id;

    // 表示按键按下时的逻辑电平，0或1。
    uint8_t pressed_logic_level : 1;

    // 当前的按键事件类型，如单击、双击等。
    uint8_t event               : 4;

    // 当前的按键状态，如默认、按下、长按等。
    uint8_t status              : 3;

} key_t;</code></pre><p id="u68724e22" class="ne-p"><br></p><p id="u3d82f0c4" class="ne-p"><span class="ne-text">这里还定义了一个叫做 </span><code class="ne-code"><span class="ne-text">EVENT_SET_AND_EXEC_CB</span></code><span class="ne-text"> 的宏函数，他接受一个 btn 指针和一个 evt 事件；它对应的作用是：</span></p><ol class="ne-ol"><li id="uce5e7752" data-lake-index-type="0"><span class="ne-text">他被调用的地方是：按键一轮扫描后，针对扫描结果进行处理</span></li><li id="ue5c24d0f" data-lake-index-type="0"><span class="ne-text">当调用此函数时，表示该按键对象的事件类型已经被更新，此时立即触发按键事件更新回调函数 cb，来处理事件变更的相关业务代码</span></li></ol><pre data-language="cpp" id="WPOTn" class="ne-codeblock language-cpp"><code>#define EVENT_SET_AND_EXEC_CB(btn, evt) \
 do \
 { \
 btn-&gt;event = evt; \
 if(btn-&gt;cb) \
 btn-&gt;cb((key_t\*)btn); \
 } while(0)</code></pre><p id="u35f93baa" class="ne-p"><br></p><h3 id="mPN7U"><span class="ne-text">函数声明</span></h3><p id="u5143d00a" class="ne-p"><span class="ne-text">声明之后要调用的函数，并且声明两个 main.c 内调用的函数</span></p><pre data-language="cpp" id="CXYOf" class="ne-codeblock language-cpp"><code>#ifdef \_\_cplusplus
extern &quot;C&quot; {
#endif

// 按键注册于事件挂载相关
int32*t key_register(key_t \_button);
key_evt_t key_evt_read(key_t* button);
uint8_t key_scan(void);

// main.c 内调用的初始化与 loop 循环函数
void user_key_init(void);
void user_key_scan(void);

#ifdef \_\_cplusplus
}
#endif </code></pre></details>
第二步：配置按键核心文件

<details class="lake-collapse"><summary id="uf86f2cad"><span class="ne-text">key_helper.c</span></summary><h3 id="XSwFf"><span class="ne-text">预定义</span></h3><p id="u12ed833e" class="ne-p"><span class="ne-text">这里定义了后续会使用到的所有变量啥的；</span></p><p id="u402fd92f" class="ne-p"><span class="ne-text">我们可以注意到</span><code class="ne-code"><span class="ne-text">g_logic_level</span></code><span class="ne-text">是存储按键逻辑电平状态的，并且该变量类型为uint32_t，也就是说他只有32位，只能最高记录32个按钮的状态（肯定够了，上帝都用不了这么多按钮）</span></p><p id="ua936128e" class="ne-p"><span class="ne-text">同理，</span><code class="ne-code"><span class="ne-text">g_btn_status_reg</span></code><span class="ne-text">存储按钮的状态，同样也是32bit的</span></p><pre data-language="cpp" id="x8cxw" class="ne-codeblock language-cpp"><code>/**
 * BTN_IS_PRESSED
 * 
 * 宏定义，用于检查指定按钮是否被按下。
 * 通过位运算检查全局状态寄存器中相应位的状态。
 * 
 * 参数：
 * i - 按钮索引
 * 
 * 返回值：
 * 1 - 表示按钮被按下
 * 0 - 表示按钮没有被按下
*/
#define BTN_IS_PRESSED(i) (g_btn_status_reg &amp; (1 &lt;&lt; i))

/\*\*

- 枚举类型 KEY_STATUS，用于表示按键的不同状态。
  \*/
  enum KEY_STATUS
  {
  KEY_STATUS_DEFAULT = 0, // 默认状态，按键未被按下
  KEY_STATUS_DOWN = 1, // 按键被按下的状态
  KEY_STATUS_MULTIPLE_CLICK = 2 // 多次点击状态，用于处理连续点击事件
  };

/\*\*

- 定义按键类型为无符号 32 位整数。
- 用于存储按键的状态和逻辑电平。
  \*/
  typedef uint32_t btn_type_t;

/\*\*

- 定义一个指向\_key_t 结构体的静态指针，用于指向按键链表的头部。
- 这个链表用于管理所有的按键。
  */
  static key_t *btn_head = NULL;

/\*\*

- 定义一个 btn_type_t 类型的全局变量，用于存储按键的逻辑电平。
- 每个按键的逻辑电平存储在一位中，这样可以支持多个按键。
  \*/
  btn_type_t g_logic_level = (btn_type_t)0;

/\*\*

- 定义一个 btn_type_t 类型的全局变量，用于存储按键的当前状态。
- 每个按键的状态存储在一位中，这样可以支持多个按键。
  \*/
  btn_type_t g_btn_status_reg = (btn_type_t)0;

/\*\*

- 定义一个静态的无符号 8 位整数，用于记录注册的按键数量。
- 这个变量用于跟踪系统中按键的数量，以确保不超过最大按键数量。
  \*/
  static uint8_t button_cnt = 0;</code></pre><p id="u920dced6" class="ne-p"><br></p><h3 id="KXHKd"><span class="ne-text">按键注册</span></h3><p id="ua38ba754" class="ne-p"><span class="ne-text">注册一个按键主要分为以下几个步骤：</span></p><ol class="ne-ol"><li id="ufe02125d" data-lake-index-type="0"><span class="ne-text">判断注册的按钮数量是否超出了最大值</span></li><li id="u17ec5eb4" data-lake-index-type="0"><span class="ne-text">将按钮注册到链表头部</span></li><li id="u0166a229" data-lake-index-type="0"><span class="ne-text">初始化按钮的状态</span></li><li id="u1e6f1aa3" data-lake-index-type="0"><span class="ne-text">更新 g_logic_level 变量内指定位的按键电平</span></li><li id="ufcf798aa" data-lake-index-type="0"><span class="ne-text">自增 button_cnt，表示注册了新的按钮</span></li></ol><pre data-language="cpp" id="VOi4x" class="ne-codeblock language-cpp"><code>/\*\*
- @brief 注册一个用户按键
-
- 该函数用于将用户定义的按键添加到按键管理系统中。
- 它将按键实例添加到一个链表中，并设置按键的初始状态。
-
- @param button: 指向按键结构体实例的指针
- @return 返回已注册的按键数量，或者在出错时返回 -1
  */
  int32_t key_register(key_t *button)
  {
  key_t \*curr = btn_head;

      // 检查传入的按钮指针是否为空，或者是否超出了最大按键数量
      if (!button || (button_cnt &gt;= sizeof(btn_type_t) * 8))
      {
          return -1;
      }

      // 遍历链表，检查是否已经注册了相同的按钮实例
      while (curr)
      {
          if (curr == button)
          {
              return -1;  // 如果按钮已经存在，则返回错误
          }
          curr = curr-&gt;next;
      }

      // 将新按钮添加到链表的头部
      button-&gt;next = btn_head;
      // 设置按钮的初始状态
      button-&gt;status = KEY_STATUS_DEFAULT;
      button-&gt;event = KEY_EVT_NONE;
      button-&gt;scan_cnt = 0;
      button-&gt;click_cnt = 0;
      // 设置最大多次点击间隔
      button-&gt;max_multiple_clicks_interval = KEY_CONFIG_MAX_MULTIPLE_CLICKS_INTERVAL;
      btn_head = button;

      // 更新逻辑电平寄存器，将新注册的按钮的逻辑电平设置到适当的位
      g_logic_level |= (button-&gt;pressed_logic_level &lt;&lt; button_cnt);
      // 增加已注册按键的数量
      button_cnt ++;

      // 返回当前注册的按键数量
      return button_cnt;

  }</code></pre><p id="u30137c5b" class="ne-p"><br></p><h3 id="pO0fz"><span class="ne-text">按键读取</span></h3><p id="u6e2d691c" class="ne-p"><span class="ne-text">这一块非常类似于 Linux 的 select 操作；</span></p><ol class="ne-ol"><li id="u5cec58c0" data-lake-index-type="0"><span class="ne-text">首先创建 32bit 的变量 raw_data，均初始化为 0</span></li><li id="uf3b97f60" data-lake-index-type="0"><span class="ne-text">从链表头部开始遍历所有按键，调用按键对象的 usr_button_read 函数来获取当前按键的电平状态，并吧电平状态写入 raw_data 对应位上</span></li><li id="u716aa630" data-lake-index-type="0"><span class="ne-text">最后对 raw_data 取反后和 g_logic_level 异或，该过程的目的是让状态不一致的标志位发生改变，状态一致的标志位就直接保持不变就好</span></li></ol><pre data-language="cpp" id="lRExG" class="ne-codeblock language-cpp"><code>/\*\*

- @brief 读取所有注册按键的状态
-
- 该函数遍历所有已注册的按键，调用每个按键的用户定义读取函数，
- 并将读取到的状态存储在全局状态寄存器中。
  _/
  static void key_read(void)
  {
  uint8_t i;
  key_t_ target;

      // 初始化一个变量来存储所有按键的原始数据，开始时所有位都为0。
      btn_type_t raw_data = 0;

      // 从链表的头部开始遍历所有注册的按键。
      for (target = btn_head, i = button_cnt - 1;
          (target != NULL) &amp;&amp; (target-&gt;usr_button_read != NULL);
          target = target-&gt;next, i--)
      {
          // 调用每个按键的读取函数，并将结果左移相应的位数后，
          // 通过按位或操作合并到raw_data中。
          // 这样保证了先注册的按键其值位于raw_data的低位。
          raw_data = raw_data | ((target-&gt;usr_button_read)(target) &lt;&lt; i);
      }

      // 更新全局按钮状态寄存器g_btn_status_reg。
      // 取反raw_data是因为我们通常认为0表示按下，1表示未按。
      // 然后与g_logic_level进行异或操作，得到最终的按键状态。
      g_btn_status_reg = (~raw_data) ^ g_logic_level;

  }</code></pre><p id="u0c0b1711" class="ne-p"><br></p><h3 id="ERLRl"><span class="ne-text">按键扫描处理</span></h3><p id="ua1430604" class="ne-p"><span class="ne-text">首先看这两个基本的代码，分别是读取当前按键的事件，另一个是按键扫描函数；</span></p><p id="u3b04a068" class="ne-p"><span class="ne-text">key_scan 按键扫描函数又包含了两个子函数</span></p><ol class="ne-ol"><li id="ud299ff7c" data-lake-index-type="0"><span class="ne-text">key_read() 即读取按键状态，并把按键状态记录到变量 g_btn_status_reg 对应位上</span></li><li id="ubeb897a7" data-lake-index-type="0"><span class="ne-text">key_process() 按键处理函数，根据按键当前数值判断他应该跳转到那种事件上，并执行对应的事件回调函数</span></li></ol><pre data-language="cpp" id="AX3eq" class="ne-codeblock language-cpp"><code>key_evt_t key_evt_read(key_t\* button)
  {
  return (key_evt_t)(button-&gt;event);
  }

uint8_t key_scan(void)
{
key_read();
return key_process();
}</code></pre><p id="u870b8652" class="ne-p"><span class="ne-text">按键扫描处理函数，这里用到了全篇最关键的状态机</span></p><p id="u7cdbb922" class="ne-p"><span class="ne-text">使用状态机的流程如下</span></p><ol class="ne-ol"><li id="u555688ef" data-lake-index-type="0"><span class="ne-text">判断当前按钮状态，它是按下 DOWN 还是空闲 DEFAULT，还是处于连点状态 RELEASE（KEY_STATUS_MULTIPLE_CLICK）</span></li><li id="u47203fa1" data-lake-index-type="0"><span class="ne-text">根据当前条件判断按钮触发时长，从而切换到对应的按钮事件</span></li><li id="u4c53c64e" data-lake-index-type="0"><span class="ne-text">更新对应按钮的按钮事件，然后立即调用事件监听回调函数处理掉该事件</span></li></ol><pre data-language="cpp" id="zMM99" class="ne-codeblock language-cpp"><code>/\*\*

- @brief 处理所有注册按键的状态变化
-
- 该函数遍历所有已注册的按键，根据每个按键的当前状态和扫描计数器，
- 来更新按键的状态，并在适当的时候触发事件。
-
- @return 返回当前活跃（状态非默认）的按键数量
  _/
  static uint8_t key_process(void)
  {
  uint8_t i;
  uint8_t active_btn_cnt = 0; // 用于计数当前活跃的按键数量
  key_t_ target;

      // 遍历所有注册的按键
      for (target = btn_head, i = button_cnt - 1; target != NULL; target = target-&gt;next, i--)
      {
          // 如果按键状态不是默认状态，则增加扫描计数器
          if (target-&gt;status &gt; KEY_STATUS_DEFAULT)
          {
              target-&gt;scan_cnt++;
              // 检查扫描计数器是否溢出，如果是，则重置为长按开始的计数值
              if (target-&gt;scan_cnt &gt;= ((1 &lt;&lt; (sizeof(target-&gt;scan_cnt) * 8)) - 1))
              {
                  target-&gt;scan_cnt = target-&gt;long_hold_start_tick;
              }
          }

          // 根据按键的当前状态执行不同的操作
          switch (target-&gt;status)
          {
          case KEY_STATUS_DEFAULT: // 默认状态，按键未被按下
              // 如果检测到按键被按下，则重置扫描和点击计数器，设置状态为按下
              if (BTN_IS_PRESSED(i))
              {
                  target-&gt;scan_cnt = 0;
                  target-&gt;click_cnt = 0;
                  EVENT_SET_AND_EXEC_CB(target, KEY_STATUS_DOWN);
                  target-&gt;status = KEY_STATUS_DOWN;
              }
              else
              {
                  target-&gt;event = KEY_EVT_NONE;
              }
              break;

          case KEY_STATUS_DOWN: // 按键被按下状态
              // 如果按键仍然被按下
              if (BTN_IS_PRESSED(i))
              {
                  // 处理多次点击事件
                  if (target-&gt;click_cnt &gt; 0)
                  {
                      if (target-&gt;scan_cnt &gt; target-&gt;max_multiple_clicks_interval)
                      {
                          EVENT_SET_AND_EXEC_CB(target, target-&gt;click_cnt &lt; KEY_EVT_REPEAT_CLICK ? target-&gt;click_cnt : KEY_EVT_REPEAT_CLICK);
                          target-&gt;status = KEY_STATUS_DOWN;
                          target-&gt;scan_cnt = 0;
                          target-&gt;click_cnt = 0;
                      }
                  }
                  // 处理长按事件
                  else if (target-&gt;scan_cnt &gt;= target-&gt;long_hold_start_tick)
                  {
                      if (target-&gt;event != KEY_EVT_LONG_HOLD)
                      {
                          EVENT_SET_AND_EXEC_CB(target, KEY_EVT_LONG_HOLD);
                      }
                  }
                  // 处理长按开始事件
                  else if (target-&gt;scan_cnt &gt;= target-&gt;long_press_start_tick)
                  {
                      if (target-&gt;event != KEY_EVT_LONG_START)
                      {
                          EVENT_SET_AND_EXEC_CB(target, KEY_EVT_LONG_START);
                      }
                  }
                  // 处理短按开始事件
                  else if (target-&gt;scan_cnt &gt;= target-&gt;short_press_start_tick)
                  {
                      if (target-&gt;event != KEY_EVT_SHORT_START)
                      {
                          EVENT_SET_AND_EXEC_CB(target, KEY_EVT_SHORT_START);
                      }
                  }
              }
              // 如果按键被释放
              else
              {
                  // 根据扫描计数器的值触发不同的释放事件，并重置状态
                  if (target-&gt;scan_cnt &gt;= target-&gt;long_hold_start_tick)
                  {
                      EVENT_SET_AND_EXEC_CB(target, KEY_EVT_LONG_HOLD_UP);
                      target-&gt;status = KEY_STATUS_DEFAULT;
                  }
                  else if (target-&gt;scan_cnt &gt;= target-&gt;long_press_start_tick)
                  {
                      EVENT_SET_AND_EXEC_CB(target, KEY_EVT_LONG_UP);
                      target-&gt;status = KEY_STATUS_DEFAULT;
                  }
                  else if (target-&gt;scan_cnt &gt;= target-&gt;short_press_start_tick)
                  {
                      EVENT_SET_AND_EXEC_CB(target, KEY_EVT_SHORT_UP);
                      target-&gt;status = KEY_STATUS_DEFAULT;
                  }
                  else
                  {
                      target-&gt;status = KEY_STATUS_MULTIPLE_CLICK;
                      target-&gt;click_cnt++;
                  }
              }
              break;

          case KEY_STATUS_MULTIPLE_CLICK: // 多次点击状态
              // 如果按键仍然被按下，切换回按下状态
              if (BTN_IS_PRESSED(i))
              {
                  target-&gt;status = KEY_STATUS_DOWN;
              }
              else
              {
                  // 如果超过了最大多次点击间隔，触发点击事件，并重置状态
                  if (target-&gt;scan_cnt &gt; target-&gt;max_multiple_clicks_interval)
                  {
                      EVENT_SET_AND_EXEC_CB(target, target-&gt;click_cnt &lt; KEY_EVT_REPEAT_CLICK ? target-&gt;click_cnt : KEY_EVT_REPEAT_CLICK);
                      target-&gt;status = KEY_STATUS_DEFAULT;
                  }
              }
              break;
          }

          // 如果按键状态不是默认状态，增加活跃按键计数
          if (target-&gt;status &gt; KEY_STATUS_DEFAULT)
          {
              active_btn_cnt++;
          }
      }

      // 返回当前活跃的按键数量
      return active_btn_cnt;

  }</code></pre></details>

### 按键移植库

首先注册我们之后需要用到的三个按钮

1. 使用 HAL_GPIO_ReadPin 获取引脚电平状态；
2. user_key_t 枚举类记录了对应的 3 个按钮，枚举类末尾的 USER_BUTTON_MAX 表示当前枚举类内有多少个对象，这个也是我们

```cpp
/**
 * @brief 用户注册的按键宏定义
 *
 * 这些宏定义用于简化GPIO读取操作，使得代码更加清晰。
 * 每个宏都对应一个特定的GPIO引脚，用于读取按键的状态。
 */
#define KEY0_READ        HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_4) ///< 读取按键0的状态
#define KEY1_READ        HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_3) ///< 读取按键1的状态
#define KEY2_READ        HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_2) ///< 读取按键2的状态

/**
 * @brief 用户按键枚举类型
 *
 * 定义了一个枚举类型user_key_t，用于表示不同的用户按键。
 * 这使得代码更加可读，并且可以方便地通过枚举值来引用特定的按键。
 */
typedef enum {
	USER_BUTTON_0 = 0, ///< 枚举值，表示用户按键0
	USER_BUTTON_1,     ///< 枚举值，表示用户按键1
	USER_BUTTON_2,     ///< 枚举值，表示用户按键2
	USER_BUTTON_MAX    ///< 枚举值，表示用户按键的最大数量
} user_key_t;

/**
 * @brief 用户按键数组
 *
 * 定义了一个静态数组user_keys，用于存储所有用户按键的状态。
 * 数组的大小由USER_BUTTON_MAX决定，确保每个按键都有一个对应的状态结构体。
 */
static key_t user_keys[USER_BUTTON_MAX];
```

第二步，看看按钮的读取与回调函数如何，我们需要在这里写入我们的按钮读取函数，包括事件中断回调的处理逻辑；

```cpp
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
```

第三步：完成我们的按键初始化与按键扫描任务

```cpp
/**
 * @brief 扫描用户按键状态
 *
 * 该函数负责调用key_scan()来检测所有注册的按键状态。
 * 然后通过HAL_Delay()函数实现20毫秒的延迟，以便为下一次扫描提供稳定的时基。
 */
void user_key_scan(void){
    key_scan();  // 调用key_scan()函数扫描所有按键
    HAL_Delay(20); // 延迟20毫秒，相当于1ms扫描一次按键
}

/**
 * @brief 初始化用户按键
 *
 * 该函数用于初始化所有用户可操作的按键。
 * 它将每个按键的属性设置为默认值，并将它们注册到按键处理系统中。
 */
void user_key_init(void){
    int i;
    // 使用memset()函数将按键数组清零，初始化所有按键状态为0
    memset(&user_keys[0], 0x0, sizeof(key_t) * USER_BUTTON_MAX);
    // 遍历所有用户按键
    for (i = 0; i < USER_BUTTON_MAX; i++)
    {
        // 设置每个按键的ID，与枚举值对应
        user_keys[i].id = i;
        // 设置每个按键的用户读取函数
        user_keys[i].usr_button_read = common_key_read;
        // 设置每个按键的事件回调函数
        user_keys[i].cb = common_key_evt_cb;
        // 设置按键按下时的逻辑电平，默认为0（低电平触发）
        user_keys[i].pressed_logic_level = 0;
        // 设置短按开始的扫描计数阈值，使用宏定义转换毫秒为计数
        user_keys[i].short_press_start_tick = KEY_CONFIG_MS_TO_COUNT(200);
        // 设置长按开始的扫描计数阈值
        user_keys[i].long_press_start_tick = KEY_CONFIG_MS_TO_COUNT(1000);
        // 设置长按保持的扫描计数阈值
        user_keys[i].long_hold_start_tick = KEY_CONFIG_MS_TO_COUNT(1500);

        // 将每个按键注册到按键处理系统中
        key_register(&user_keys[i]);
    }
}
```

最终在我们的 main.c 里面进行一个调用就好啦

（在此之前请确保你已经使能了 PE2/PE3/PE4 三个 GPIO 口，并设置为输入，默认上拉模式）

```cpp
int main(void)
{
    user_key_init();

  while (1)
  {
	user_key_scan();
  }
}

```
