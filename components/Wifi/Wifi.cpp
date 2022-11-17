/**
 * @file Wifi.cpp
 * @author huanghan0912 (huanghan0912@foxmail.com)
 * @brief 
 * @version 0.1
 * @date 2022-11-16
 * 
 * 
 */

#include "Wifi.h"



#define ESP_MAXIMUM_RETRY  1000


#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static const char *TAG = "wifi";

static int s_retry_num = 0;



/**
 * @brief 初始化wifi,输入ssid和password
 * 
 * @param ssid wifi网络广播
 * @param password wifi密码
 */
void Wifi::init(char* ssid,char* password){
    //给wifi_config中数据赋值
    strcpy((char *)wifi_config.sta.ssid,ssid);
    strcpy((char *)wifi_config.sta.password, password);
        
}

/**
 * @brief 
 * 
 * @param arg 表示传递给handler函数的参数
 * @param event_base 事件基
 * @param event_id 表示事件ID
 * @param event_data 表示传递给这个事件的数据
 */
void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < ESP_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}


/**
 * @brief 开始wifi的STA模式
 * 
 */
void Wifi::STA_begin(){


    //创建默认事件组
    s_wifi_event_group = xEventGroupCreate();
    //初始化潜在的TCP/IP 栈
    ESP_ERROR_CHECK(esp_netif_init());
    //创建默认循环组
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();
    //初始化wifi驱动
    //必须先调用此 API，然后才能调用所有其他 WiFi API
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,//事件基，代表事件的大类
                                                        ESP_EVENT_ANY_ID,//事件ID,从ap中获取ip
                                                        &event_handler,
                                                        NULL,//表示需要传递给handler函数的参数
                                                        &instance_any_id));//类型为：esp_event_handler_instance_t指针
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);


    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
                  wifi_config.sta.ssid,wifi_config.sta.password);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
                 wifi_config.sta.ssid,wifi_config.sta.password );
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }

}


/**
 * @brief 开启AP模式
 * 
 */
void Wifi::AP_begin(){
    // //初始化潜在的TCP/IP 栈
    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));

    wifi_config_t wifi_config = {
    .ap = {
        .ssid = "your_ssid",
        .ssid_len = strlen("your_ssid"),
        .channel = EXAMPLE_ESP_WIFI_CHANNEL,
        .password = "password",
        .max_connection = EXAMPLE_MAX_STA_CONN,
        .authmode = WIFI_AUTH_WPA_WPA2_PSK
        },
    };
}