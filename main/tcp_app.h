#include <stdio.h>
#include <string.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "driver/gpio.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>
#include "blufi.h"
#include "bsp.h"
#include "esp_spi_flash.h"
#include "esp_err.h"
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_bt.h"

#define TCP_TAG "TCP"
#define TCP_INFO(fmt, ...)   ESP_LOGI(TCP_TAG, fmt, ##__VA_ARGS__) 
/* wifi连接函数，位于app_wifi.c,阻塞状态 */

#define HOST_PORT 8082
#define HOST_IP_ADDR "192.168.0.234"
#define Sub_Addr  0X34          //发送这个让主机开始发数据
#define Host_Addr   0X80        //主机地址标识码

#define producing_Normal 0
#define producing_Default 1
#define producing_Abnormal 2


// bool restart=true;               //  重连标志位，不用了，换个continue就能重连
//#pragma pack(push)
//#pragma pack(1)
typedef struct 
            {
                uint8_t Addr;           //主机地址标识码
                uint8_t Function;       //功能，目前只有控制命令
                uint8_t Size;           //数据多少
                uint8_t Data;          //数据，可拓展
                uint8_t  Check_Sum;     //校验码
            }Mounter_Data_t;
            Mounter_Data_t mouter_rx;           //  贴片机数据接受
Mounter_Data_t send_start = {
                                .Addr = 0X34,
                                .Function = 0X01,
                                .Size = 0X01,
                                .Data = 0X00,
                                .Check_Sum = 0X14
                                };//请求主机发送数据
Mounter_Data_t send_error = {     
                                .Addr = 0X34,
                                .Function = 0X01,
                                .Size = 0X01,
                                .Data = 0X00,
                                .Check_Sum = 0X15
                                };//主机发送数据错误命令
//#pragma pack(pop)

