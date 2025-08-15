/**
 * This is a code to demonstrate the use of queues to interact with a shared resource between tasks.
 * The code produces an interactive console that controls the LED blink delay
 * By typing `delay <time in miliseconds>` i can set the delay variable in a thread safe manner.
 * 
 * For the console I used the shown usage basic example in the ESP IDF documentation
 * 
 */

#include <stdio.h>
#include <string.h>
#include "esp_system.h"
#include "esp_log.h"
#include "esp_console.h"
#include "esp_vfs_dev.h"
#include "esp_vfs_fat.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "cmd_system.h"
#include "cmd_wifi.h"
#include "cmd_nvs.h"


// Global Variables
static bool led_state=1;
static QueueHandle_t queue_handle;

/*
 * We warn if a secondary serial console is enabled. A secondary serial console is always output-only and
 * hence not very useful for interactive console applications. If you encounter this warning, consider disabling
 * the secondary serial console in menuconfig unless you know what you are doing.
 */
#if SOC_USB_SERIAL_JTAG_SUPPORTED
#if !CONFIG_ESP_CONSOLE_SECONDARY_NONE
#warning "A secondary serial console is not useful when using the console component. Please disable it in menuconfig."
#endif
#endif

static const char* TAG = "example";
#define PROMPT_STR CONFIG_IDF_TARGET

/* Console command history can be stored to and loaded from a file.
 * The easiest way to do this is to use FATFS filesystem on top of
 * wear_levelling library.
 */
#if CONFIG_CONSOLE_STORE_HISTORY

#define MOUNT_PATH "/data"
#define HISTORY_PATH MOUNT_PATH "/history.txt"

static void initialize_filesystem(void)
{
    static wl_handle_t wl_handle;
    const esp_vfs_fat_mount_config_t mount_config = {
            .max_files = 4,
            .format_if_mount_failed = true
    };
    esp_err_t err = esp_vfs_fat_spiflash_mount_rw_wl(MOUNT_PATH, "storage", &mount_config, &wl_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount FATFS (%s)", esp_err_to_name(err));
        return;
    }
}
#endif // CONFIG_STORE_HISTORY

static void initialize_nvs(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK( nvs_flash_erase() );
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
}

static int delay_command(int argc, char** argv)
{
    // Check if the command has an argument
    if (argc != 2) {
        printf("Usage: delay <delay in miliseconds>\n");
        return ESP_ERR_INVALID_ARG;
    }
    // Get the text from the command line argument
    const int new_delay = atoi(argv[1]);
    
    delay = new_delay;
    
    xQueueSend(queue_handle, (void*)&new_delay,portMAX_DELAY);
    
    return ESP_OK;
}


void blink_task(void* params){
    int delay = 1000;
    while(1){
        xQueueReceive(queue_handle, &delay, 0);
        gpio_set_level(GPIO_NUM_2, led_state);
        led_state = !led_state;
        vTaskDelay(pdMS_TO_TICKS(delay));
    }
}

void app_main(void)
{
    queue_handle = xQueueCreate(5, sizeof(int));

    gpio_set_direction(GPIO_NUM_2, GPIO_MODE_OUTPUT);
    xTaskCreatePinnedToCore(
        blink_task,
        "blink_task",
        1024,
        NULL,
        0,
        NULL,
        1        
    );

    /* Config and Initialization of Console Task */
    esp_console_repl_t *repl = NULL;
    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    /* Prompt to be printed before each line.
     * This can be customized, made dynamic, etc.
     */
    repl_config.prompt = PROMPT_STR ">";
    repl_config.max_cmdline_length = CONFIG_CONSOLE_MAX_COMMAND_LINE_LENGTH;

    initialize_nvs();


#if CONFIG_CONSOLE_STORE_HISTORY
    initialize_filesystem();
    repl_config.history_save_path = HISTORY_PATH;
    ESP_LOGI(TAG, "Command history enabled");
#else
    ESP_LOGI(TAG, "Command history disabled");
#endif
    const esp_console_cmd_t delay_cmd = {
        .command = "delay",
        .help = "sets the delay",
        .hint = "<enter delay int>",
        .func = &delay_command
    };
    const esp_console_cmd_t send_cmd = {
        .command = "send",
        .help = "Send text to monitor",
        .hint = "<Enter Text>",
        .func = &serial_send_command
    };

    /* Register commands */
    esp_console_register_help_command();
    register_system_common();
    esp_console_cmd_register(&delay_cmd);
    esp_console_cmd_register(&send_cmd);

#if SOC_LIGHT_SLEEP_SUPPORTED
    register_system_light_sleep();
#endif
#if SOC_DEEP_SLEEP_SUPPORTED
    register_system_deep_sleep();
#endif
#if (CONFIG_ESP_WIFI_ENABLED || CONFIG_ESP_HOST_WIFI_ENABLED)
    register_wifi();
#endif
    register_nvs();

#if defined(CONFIG_ESP_CONSOLE_UART_DEFAULT) || defined(CONFIG_ESP_CONSOLE_UART_CUSTOM)
    esp_console_dev_uart_config_t hw_config = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_console_new_repl_uart(&hw_config, &repl_config, &repl));

#elif defined(CONFIG_ESP_CONSOLE_USB_CDC)
    esp_console_dev_usb_cdc_config_t hw_config = ESP_CONSOLE_DEV_CDC_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_console_new_repl_usb_cdc(&hw_config, &repl_config, &repl));

#elif defined(CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG)
    esp_console_dev_usb_serial_jtag_config_t hw_config = ESP_CONSOLE_DEV_USB_SERIAL_JTAG_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_console_new_repl_usb_serial_jtag(&hw_config, &repl_config, &repl));

#else
#error Unsupported console type
#endif

    ESP_ERROR_CHECK(esp_console_start_repl(repl));

}
