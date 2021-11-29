#include "tcp_app.h"
TaskHandle_t task_client;           //创建任务句柄
TaskHandle_t task_mouter;           //创建任务句柄
TaskHandle_t task_alloc;           //创建任务句柄
TaskHandle_t task_wifi;           //创建任务句柄
extern bool gl_sta_connected;
bool first_connect = true;
bool tcp_is_connect = false;
static portMUX_TYPE my_mutex = portMUX_INITIALIZER_UNLOCKED;

static void tcp_client_task(void *pvParameters);
static bool Deal_Rx_Data(uint8_t *rx_buffer,uint8_t len);
static void control_mouter_task(void *pvParameters);
static void alloc_task(void *pvParameters);
static void wifi_task(void *pvParameters);

void app_main(void)
{
    /* 打印Hello world! */
    TCP_INFO("Hello world!\n");
     /* 硬件配置 */
    bsp_config();

     /* 蓝牙配网 */
    blufi_init();
    /* 创建开始事件 */
    xTaskCreate(alloc_task, "task", 512, NULL, 2, &task_alloc);
    //vTaskStartScheduler();  //开启任务调度器,不用了startup.c中有
}

bool Checked_Sum(Mounter_Data_t mouter)
{
    uint8_t  Check_Temp=(mouter.Function<<4)|(mouter.Size<<2)|mouter.Data;
    if(mouter.Check_Sum==Check_Temp)     //校验码    
        return true;
    else
        return false;     

}
static void alloc_task(void *pvParameters)
{
    portENTER_CRITICAL(&my_mutex); // 进入临界区
    xTaskCreate(wifi_task, "wifi_task", 2048, NULL, 9, &task_wifi);
    xTaskCreate(tcp_client_task, "tcp_client", 4096, NULL, 8, &task_client);//configMAX_PRIORITIES=32
    xTaskCreate(control_mouter_task, "control_mouter", 2048, NULL, 7, &task_mouter);
    portEXIT_CRITICAL(&my_mutex); // 退出临界区  
    vTaskDelete(task_alloc); // 删除开始任务
}

static void wifi_task(void *pvParameters)
{
    static bool wifi_pre_connected = false;
    while(1)
    {
        //TCP_INFO("the max stack of wifi_task is %d\n",uxTaskGetStackHighWaterMark(task_wifi));
        /*用 switch 就更完美了*/
        //TCP_INFO("wifi task is running!!!!!!!!!!");
        if(first_connect == true)//第一次连接
            {
                first_connect = false;
                TCP_INFO("wifi task is first running!!!!!!!!!!");
                continue;
            }
        if(gl_sta_connected != wifi_pre_connected)
        {
            wifi_pre_connected = gl_sta_connected;
            if(gl_sta_connected == false)//WiFi未连接
                {
                    //vTaskSuspendAll();
                    gpio_set_level(GPIO_YELLOW_LED,0);
                    gpio_set_level(GPIO_RED_LED,0);
                    gpio_set_level(GPIO_BLE_LED,0);
                    gpio_set_level(GPIO_GREEN_LED,0);
                    gpio_set_level(GPIO_BEEP,0);
                    gpio_set_level(GPIO_RELAY,0);
                    vTaskSuspend(task_client);
                    vTaskSuspend(task_mouter);
                    TCP_INFO("wifi is disconnect!!!task has been Suspended!!!!!!!!!!");

                }
            else//WiFi重连
                {
                    //xTaskResumeAll();
                    TCP_INFO("wifi is connect!!!task has been resumed!!!!!!!!!!");
                     vTaskResume(task_client);
                }
        }
        while(gl_sta_connected == false)//断网等待重连
            {
                vTaskDelay(200/ portTICK_PERIOD_MS);
            }
            vTaskDelay(200/ portTICK_PERIOD_MS);
    }
    vTaskDelete(task_wifi); // 删除开始任务
}

static void control_mouter_task(void *pvParameters)
{
    while(1)
    {
        //TCP_INFO("mouter task is running!!!!!!!!!!");
        //TCP_INFO("the max stack of control_mouter_task is %d\n",uxTaskGetStackHighWaterMark(task_mouter));
         if(mouter_rx.Addr == Host_Addr)
         {
            if(mouter_rx.Data == producing_Normal)
            {
                gpio_set_level(GPIO_YELLOW_LED,0);
                gpio_set_level(GPIO_RED_LED,0);
                gpio_set_level(GPIO_GREEN_LED,1);
                gpio_set_level(GPIO_BEEP,0);
                gpio_set_level(GPIO_RELAY,0);
                //TCP_INFO("nomal");
            }
            if(mouter_rx.Data == producing_Abnormal)
            {
                gpio_set_level(GPIO_RED_LED,0);
                gpio_set_level(GPIO_GREEN_LED,0);
                gpio_set_level(GPIO_RELAY,0);
                gpio_set_level(GPIO_YELLOW_LED,1);
                vTaskDelay(3500/ portTICK_PERIOD_MS);
                gpio_set_level(GPIO_BEEP,1);
                vTaskDelay(500/ portTICK_PERIOD_MS);
                gpio_set_level(GPIO_BEEP,0);
                //TCP_INFO("abnomal!!!!");
            }

            if(mouter_rx.Data == producing_Default)
            {
                gpio_set_level(GPIO_YELLOW_LED,0);
                gpio_set_level(GPIO_GREEN_LED,0);
                gpio_set_level(GPIO_RED_LED,1);
                gpio_set_level(GPIO_RELAY,1);
                vTaskDelay(500/ portTICK_PERIOD_MS);
                gpio_set_level(GPIO_BEEP,1);
                vTaskDelay(500/ portTICK_PERIOD_MS);
                gpio_set_level(GPIO_BEEP,0);
                TCP_INFO("Default!!!!!!!!!!");
            }
            //TCP_INFO("nomal!!!!!!!!!!");
         }
         else 
            {
                gpio_set_level(GPIO_YELLOW_LED,0);
                gpio_set_level(GPIO_RED_LED,0);
                gpio_set_level(GPIO_GREEN_LED,1);
                gpio_set_level(GPIO_BEEP,0);
                gpio_set_level(GPIO_RELAY,0);
            }
        vTaskDelay(200/ portTICK_PERIOD_MS);
    } 
        vTaskDelete(task_mouter);//删除任务本身
}

static bool Deal_Rx_Data(uint8_t *rx_buffer,uint8_t len)
{
    if(len < 5)
    {
        return false;
    }
        Mounter_Data_t mouter_rx_temp;
        uint8_t buffer_num = 0,M_size = 0;

         for(;buffer_num < len;buffer_num++)
         {
            if(rx_buffer[buffer_num] == Host_Addr)
            {
                uint8_t mouter_num = buffer_num;
                mouter_rx_temp.Addr = rx_buffer[mouter_num++];
                mouter_rx_temp.Function = rx_buffer[mouter_num++];       //功能，目前只有控制命令
                mouter_rx_temp.Size = rx_buffer[mouter_num++];           //数据多少
                M_size = mouter_rx_temp.Size;
                while(M_size--)
                {
                    mouter_rx_temp.Data = rx_buffer[mouter_num++];          //数据，可拓展
                }
                mouter_rx_temp.Check_Sum = rx_buffer[mouter_num];     //校验码
                if(Checked_Sum(mouter_rx_temp) == true)
                {
                    mouter_rx = mouter_rx_temp;
                    return true;
                }
            }
            else 
            {
                memset(&mouter_rx_temp,0,sizeof(Mounter_Data_t));        //更新数据
                continue;
            }
                /*数据校验*/
         }
         return false;  
}
static void tcp_client_task(void *pvParameters)
{
    char host_ip[] = HOST_IP_ADDR;
    int addr_family = 0;
    int ip_protocol = 0;
    uint8_t rx_buffer[128];                //TCP数据接受
    uint8_t tx_buffer[5];                //TCP数据发送
    int sock = -1;                      //socket套接字
    memset(&mouter_rx,0,sizeof(Mounter_Data_t));//初始化
    while (1) 
    {
       //TCP_INFO("tcp task is running!!!!!!!!!!");
        //TCP_INFO("the max stack of tcp_client_task is %d\n",uxTaskGetStackHighWaterMark(task_client));
        /*目标地址和端口*/
        struct sockaddr_in dest_addr;
        dest_addr.sin_addr.s_addr = inet_addr(host_ip);
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_port = htons(HOST_PORT);
        addr_family = AF_INET;
        ip_protocol = IPPROTO_IP;
        if(tcp_is_connect == false)
        {
            if (sock != -1) 
            {
                TCP_INFO("reinit  sock...");
                shutdown(sock, SHUT_RDWR);
                close(sock);
            }
            gpio_set_level(GPIO_YELLOW_LED,0);
            gpio_set_level(GPIO_RED_LED,0);
            gpio_set_level(GPIO_GREEN_LED,0);
            gpio_set_level(GPIO_BEEP,0);
            gpio_set_level(GPIO_RELAY,0);
            TCP_INFO("TCP server is disconnect!!!mouter_task has been Suspended!!!!!!!!!!");
            vTaskSuspend(task_mouter);
        }
        vTaskDelay(100/ portTICK_PERIOD_MS);
        sock =  socket(addr_family, SOCK_STREAM, ip_protocol);
        if (sock < 0) 
        {
            TCP_INFO("Unable to create socket: errno %d", errno);
            //vTaskSuspend(task_client);
            // restart=true;
            continue;
        }
        TCP_INFO("Socket created, connecting to %s:%d", host_ip, HOST_PORT);
        // struct sockaddr_in Loacl_addr;  //设置本地端口
        // Loacl_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        // Loacl_addr.sin_family = AF_INET;
        // Loacl_addr.sin_port = htons(4321);
        // uint8_t res = 0;
        // res = bind(sock,(struct sockaddr *)&Loacl_addr,sizeof(struct sockaddr_in6));
        // if(res != 0)
        // {
        //     printf("bind error\n");
        //     break;
        // }
        // TCP_INFO( "Successfully bind!"); 
        int connect_err = connect(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        if (connect_err < 0) {
            TCP_INFO( "Socket unable to connect: errno %d,err=%d\n", errno,connect_err);
            //vTaskSuspend(task_client);
            // restart=true;
            tcp_is_connect = false;
            continue;
        }
        TCP_INFO( "Successfully connected");
        tcp_is_connect = true;
        vTaskResume(task_mouter);
        //发送
        memcpy(tx_buffer, &send_start, sizeof(send_start));
        int send_err = send(sock,tx_buffer, sizeof(tx_buffer), 0);
        if (send_err < 0) 
             {
                TCP_INFO("Error occurred during sending: errno %d", errno);
                continue;
             }
        else
        {
            TCP_INFO("request to start from sub has been sent:%s, receiving...",tx_buffer);
        }     
                /*发送和接受数据处理*/
                while(1)
                {
                    //TCP_INFO("tcp task while is running!!!!!!!!!!");
                    vTaskDelay(200/ portTICK_PERIOD_MS);
                     //接收，WiFi连接中断套接字会变会退出，不用处理
                         int len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
                        if (len >= 0) //len等于0，TCP断开
                        {
                            rx_buffer[len] = '\0'; // Null-terminate whatever we received and treat like a string
                            TCP_INFO("Received %d bytes from %s:", len, host_ip);
                            TCP_INFO("%s", rx_buffer);
                            if(len == 0)
                            {
                                tcp_is_connect = false;
                                TCP_INFO("TCP IS %d\n", tcp_is_connect);
                                break;
                            }
                            if(Deal_Rx_Data(rx_buffer,len) == false )//数据指令错误，请求重新发送
                            {
                                memcpy(tx_buffer, &send_error, sizeof(send_error));
                                send_err = send(sock,tx_buffer, sizeof(tx_buffer), 0);
                                if (send_err < 0) 
                                {
                                    TCP_INFO("Error occurred during request to resend: errno %d ", errno);
                                    break;
                                }
                                else
                                {
                                    uint8_t i = 3;
                                    while(i--)
                                    {
                                        vTaskDelay(500/ portTICK_PERIOD_MS);
                                        gpio_set_level(GPIO_WIFI_LED,0);
                                        vTaskDelay(500/ portTICK_PERIOD_MS);
                                        gpio_set_level(GPIO_WIFI_LED,1);
                                    }
                                    TCP_INFO("request to resend from sub has been sent:::%s, hold on...",tx_buffer);
                                }  
                            }
                            else continue;                          
                        }

                    else break;
                }
    }     
            if (sock != -1) 
            {
                TCP_INFO("Shutting down socket restart...");
                shutdown(sock, SHUT_RDWR);
                close(sock);
            }
                vTaskDelete(task_client);//删除任务本身      
}


