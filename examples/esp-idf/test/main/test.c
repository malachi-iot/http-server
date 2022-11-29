#include <stdio.h>

#include <nvs_flash.h>

#include <threadpool.h>

#include <http/debug.h>
#include <http/pipeline.h>
#include <http/request.h>
#include <http/threadpool.h>

void wifi_init_sta(void);

static ThreadPool* thread_pool;
static HttpPipeline pipeline;
volatile sig_atomic_t signalShutdown = 0;


void app_main(void)
{
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    wifi_init_sta();

    http_pipeline_placement_new(&pipeline);
    http_pipeline_add_request(&pipeline);
    http_pipeline_add_diagnostic(&pipeline);

    thread_pool = threadpool_new(3, NULL, NULL);

    http_pipeline_startup(&pipeline);
    http_pipeline_threadpool_tcp_listener(thread_pool, &pipeline, 80);
    http_pipeline_shutdown(&pipeline);
}
