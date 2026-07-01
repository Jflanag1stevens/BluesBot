#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_event.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_nimble_hci.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include "sdkconfig.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "string.h"
#include "HarmonicaPacket.h"

static const int RX_BUF_SIZE = 1024;

#define TDX_PIN (33)
#define RDX_PIN (32)
#define BLOW_MOSFET_GPIO 19
#define DRAW_MOSFET_GPIO 18
#define WAHWAH_GPIO 21
#define LED_GPIO 2
#define ARTICULATION_DELAY 70
#define INSTRUCTION_QUEUE_SIZE 256
char *TAG = "BLE-Server";
uint8_t ble_addr_type;
void ble_app_advertise(void);

const char* draw_open = "#11D0100\r";
const char* draw_close = "#11D200\r"; 

const char* blow_open = "#12D0350\r";
const char* blow_close = "#12D0350\r";



const char* shutterOpen[] = {"#10D430\r","#9D-70\r","#8D270\r","#7D480\r","#6D400\r","#5D-580\r","#4D-330\r","#3D-370\r","#2D-210\r","#1D-270\r"};
const char* shutterClose[] = {"#10D330\r","#9D-170\r","#8D160\r","#7D340\r","#6D290\r","#5D-470\r","#4D-250\r","#3D-290\r","#2D-130\r","#1D-190\r"};
QueueHandle_t instructionQueue;


void uart_init(void){
    const uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT
    };
    uart_driver_install(UART_NUM_1, RX_BUF_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(UART_NUM_1, &uart_config);
    uart_set_pin(UART_NUM_1, TDX_PIN, RDX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

   
}

int sendData(const char* logName, const char* data)
{   
    const int len = strlen(data);
    const int txBytes = uart_write_bytes(UART_NUM_1, data, len);
    ESP_LOGI(logName, "Wrote %d bytes", txBytes);
    return txBytes;
}

            /////// OPEN BIT = 1 //// CLOSE BIT == 0 //HOLE 1 IS MSB IS BIT 21
void harmonica_control(void *arg){
    static const char *HARMONICA_CONTROL_TASK_TAG = "HARMONICA_CONTROL";
    uint32_t packetEncoded;
    uint32_t nextPacketEncoded;
    harmonica_packet_t* packet = malloc(sizeof(harmonica_packet_t));
    harmonica_packet_t* nextPacket = malloc(sizeof(harmonica_packet_t));

    const char* speeds[] = {"#12SR100\r","#11SR100\r","#10SR100\r","#9SR100\r","#8SR100\r","#7SR100\r","#6SR100\r","#5SR100\r","#4SR100\r","#3SR100\r","#2SR100\r","#1SR100\r"};
    for(int i = 0; i < 12; i++){
            sendData(HARMONICA_CONTROL_TASK_TAG, speeds[i]);
    }

    for(;;){
        
        if((xQueueReceive(instructionQueue, &packetEncoded,0) == pdTRUE)){
            harmonica_packet_decoder(packetEncoded, packet);
            if(packet->articulate == 1){
            gpio_set_level(BLOW_MOSFET_GPIO, 0);
            gpio_set_level(DRAW_MOSFET_GPIO,0);
            }


            if(packet->shutter == 0){
                gpio_set_level(BLOW_MOSFET_GPIO, 0);
                gpio_set_level(DRAW_MOSFET_GPIO,0);
                sendData(HARMONICA_CONTROL_TASK_TAG, blow_open);
                sendData(HARMONICA_CONTROL_TASK_TAG, draw_open);

            }
            else{
            for(int i = 0; i < 10; i++){
                uint16_t mask = 1;
                if((packet->shutter & (mask << i))){
                    sendData(HARMONICA_CONTROL_TASK_TAG,shutterOpen[i]);
                    
                }
                else{
                    sendData(HARMONICA_CONTROL_TASK_TAG,shutterClose[i]);
                    
                }
            }
            

            if(xQueuePeek(instructionQueue, &nextPacketEncoded,0) == pdTRUE){
                harmonica_packet_decoder(nextPacketEncoded, nextPacket);
            }
            else{
                nextPacketEncoded = 0;
            }
            if(packet->articulate == 1){
            vTaskDelay(ARTICULATION_DELAY/portTICK_PERIOD_MS);
            packet->time = packet->time  - ARTICULATION_DELAY;
            }
            if(packet->direction > 0){
                gpio_set_level(BLOW_MOSFET_GPIO, 0);
                gpio_set_level(DRAW_MOSFET_GPIO,1);
                sendData(HARMONICA_CONTROL_TASK_TAG, blow_open);
                sendData(HARMONICA_CONTROL_TASK_TAG, draw_close);
               
            }
            else{
                gpio_set_level(BLOW_MOSFET_GPIO, 1);
                gpio_set_level(DRAW_MOSFET_GPIO, 0);
                sendData(HARMONICA_CONTROL_TASK_TAG, blow_close);
                sendData(HARMONICA_CONTROL_TASK_TAG, draw_open);
                
            }
            }    
            ESP_LOGI(TAG,"delay time %d", packet->time);
            vTaskDelay((packet->time)/portTICK_PERIOD_MS);
        }

        else{
            vTaskDelay(100/portTICK_PERIOD_MS); //if no packet in queue wait 100ms before checking again
            //ESP_LOGI(TAG,"no instruction in queue");
        }
    }
}

// Write data to ESP32 defined as server
static int device_write(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    uint32_t data;
    memcpy(&data, ctxt->om->om_data ,sizeof(uint32_t));
    xQueueSend(instructionQueue, &data,0);

    return 0;
}

// Read data from ESP32 defined as server
static int device_read(uint16_t con_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    os_mbuf_append(ctxt->om, "Data from the server", strlen("Data from the server"));
    return 0;
}


static const struct ble_gatt_svc_def gatt_svcs[] = {
    {.type = BLE_GATT_SVC_TYPE_PRIMARY,
     .uuid = BLE_UUID16_DECLARE(0x180),                 // Define UUID for device type
     .characteristics = (struct ble_gatt_chr_def[]){
         {.uuid = BLE_UUID16_DECLARE(0xFEF4),           // Define UUID for reading
          .flags = BLE_GATT_CHR_F_READ,
          .access_cb = device_read},
         {.uuid = BLE_UUID16_DECLARE(0xDEAD),           // Define UUID for writing
          .flags = BLE_GATT_CHR_F_WRITE,
          .access_cb = device_write},
         {0}}},
    {0}};

// BLE event handling
static int ble_gap_event(struct ble_gap_event *event, void *arg)
{
    switch (event->type)
    {
    // Advertise if connected
    case BLE_GAP_EVENT_CONNECT:
        ESP_LOGI("GAP", "BLE GAP EVENT CONNECT %s", event->connect.status == 0 ? "OK!" : "FAILED!");
        if (event->connect.status != 0)
        {
            ble_app_advertise();
        }
        else{
            gpio_set_level(LED_GPIO, 1);
        }
        break;
    // Advertise again after completion of the event
    case BLE_GAP_EVENT_DISCONNECT:
        ESP_LOGI("GAP", "BLE GAP EVENT DISCONNECTED");
        gpio_set_level(LED_GPIO, 0);
        ble_app_advertise();
        break;
    case BLE_GAP_EVENT_ADV_COMPLETE:
        ESP_LOGI("GAP", "BLE GAP EVENT");
        ble_app_advertise();
        break;
    default:
        break;
    }
    return 0;
}

// Define the BLE connection
void ble_app_advertise(void)
{

    struct ble_hs_adv_fields fields;
    const char *device_name;
    memset(&fields, 0, sizeof(fields));
    device_name = ble_svc_gap_device_name(); 
    fields.name = (uint8_t *)device_name;
    fields.name_len = strlen(device_name);
    fields.name_is_complete = 1;
    ble_gap_adv_set_fields(&fields);

    // GAP - device connectivity definition
    struct ble_gap_adv_params adv_params;
    memset(&adv_params, 0, sizeof(adv_params));
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND; 
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;
    ble_gap_adv_start(ble_addr_type, NULL, BLE_HS_FOREVER, &adv_params, ble_gap_event, NULL);
}


void ble_app_on_sync(void)
{
    ble_hs_id_infer_auto(0, &ble_addr_type); 
    ble_app_advertise();                     
}

// The infinite task
void host_task(void *param)
{
    nimble_port_run(); // This function will return only when nimble_port_stop() is executed
}
void gpio_initialization(){
    gpio_reset_pin(LED_GPIO);
    gpio_reset_pin(BLOW_MOSFET_GPIO);
    gpio_reset_pin(DRAW_MOSFET_GPIO);
    gpio_set_direction(LED_GPIO,GPIO_MODE_OUTPUT);
    gpio_set_direction(BLOW_MOSFET_GPIO,GPIO_MODE_OUTPUT);
    gpio_set_direction(DRAW_MOSFET_GPIO,GPIO_MODE_OUTPUT);
    gpio_set_direction(WAHWAH_GPIO,GPIO_MODE_INPUT);
    gpio_set_pull_mode(WAHWAH_GPIO,GPIO_PULLDOWN_ONLY);
    gpio_input_enable(WAHWAH_GPIO);
}

void wahwah_task(void *arg){
    uint8_t input = 0;
    static const char *WAHWAH_TASK_TAG = "WAHWAH";
    const char* rotate = "#13MD3600\r";
    for(;;){
        input = gpio_get_level(WAHWAH_GPIO);
        if(input == 1){
        sendData(WAHWAH_TASK_TAG, rotate);
        }
        vTaskDelay(600/portTICK_PERIOD_MS);
    }
}

void app_main()
{
    nvs_flash_init();                         
    gpio_initialization();
    uart_init();
    instructionQueue = xQueueCreate(INSTRUCTION_QUEUE_SIZE,sizeof(uint32_t));
    // esp_nimble_hci_and_controller_init();      
    nimble_port_init();                        
    ble_svc_gap_device_name_set("BLE-Server"); 
    ble_svc_gap_init();                        
    ble_svc_gatt_init();                       
    ble_gatts_count_cfg(gatt_svcs);            
    ble_gatts_add_svcs(gatt_svcs);             
    ble_hs_cfg.sync_cb = ble_app_on_sync;      
    nimble_port_freertos_init(host_task);      
    xTaskCreate(harmonica_control,"harmonica_control_loop",4096,NULL,configMAX_PRIORITIES - 3, NULL);
    xTaskCreate(wahwah_task,"wahwah_control",2048,NULL,1, NULL);
}