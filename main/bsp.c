#include "bsp.h"

void bsp_config(void)
{
    gpio_config_t gpio_config_structure;
    /* 初始化gpio配置结构体*/
    gpio_config_structure.pin_bit_mask = (1ULL << GPIO_WIFI_LED);
    gpio_config_structure.mode = GPIO_MODE_OUTPUT;              /* 输出模式 */
    gpio_config_structure.pull_up_en = 0;                       /* 不上拉 */
    gpio_config_structure.pull_down_en = 0;                     /* 不下拉 */
    gpio_config_structure.intr_type = GPIO_PIN_INTR_DISABLE;    /* 禁止中断 */ 

    /* 根据设定参数初始化并使能 */  
	gpio_config(&gpio_config_structure);

    gpio_config_structure.pin_bit_mask = (1ULL << GPIO_BLE_LED);
    gpio_config(&gpio_config_structure);

    gpio_config_structure.pin_bit_mask = (1ULL << GPIO_RED_LED);
    gpio_config(&gpio_config_structure);

    gpio_config_structure.pin_bit_mask = (1ULL << GPIO_GREEN_LED);
    gpio_config(&gpio_config_structure);

    gpio_config_structure.pin_bit_mask = (1ULL << GPIO_YELLOW_LED);
    gpio_config(&gpio_config_structure);

    gpio_config_structure.pin_bit_mask = (1ULL << GPIO_BEEP);
    gpio_config(&gpio_config_structure);

    gpio_config_structure.pin_bit_mask = (1ULL << GPIO_RELAY);
    gpio_config(&gpio_config_structure);
    gpio_set_level(GPIO_YELLOW_LED,0);
    gpio_set_level(GPIO_WIFI_LED,0);
    gpio_set_level(GPIO_RED_LED,0);
    gpio_set_level(GPIO_BLE_LED,0);
    gpio_set_level(GPIO_GREEN_LED,0);
    gpio_set_level(GPIO_BEEP,0);
    gpio_set_level(GPIO_RELAY,0);
}

