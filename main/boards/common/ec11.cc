// EC11Encoder.cc 实现文件
#include "ec11.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#define TAG "EC11Encoder"

EC11Encoder::EC11Encoder(const Config& config) {
    // 1. 创建PCNT单元
    pcnt_unit_config_t unitCfg = {
        .low_limit = config.lowLimit,
        .high_limit = config.highLimit
    };
    ESP_ERROR_CHECK(pcnt_new_unit(&unitCfg, &unit_));

    // 2. 配置硬件滤波器
    pcnt_glitch_filter_config_t filterCfg = {
        .max_glitch_ns = config.glitchFilterNs
    };
    ESP_ERROR_CHECK(pcnt_unit_set_glitch_filter(unit_, &filterCfg));

    // 3. 初始化正交解码通道
    auto createChannel = [&](gpio_num_t edgePin, gpio_num_t levelPin,
        pcnt_channel_edge_action_t rise, pcnt_channel_edge_action_t fall) {
        pcnt_chan_config_t chanCfg = {
            .edge_gpio_num = edgePin,
            .level_gpio_num = levelPin
        };
        pcnt_channel_handle_t channel;
        ESP_ERROR_CHECK(pcnt_new_channel(unit_, &chanCfg, &channel));
        ESP_ERROR_CHECK(pcnt_channel_set_edge_action(channel, rise, fall));
        ESP_ERROR_CHECK(pcnt_channel_set_level_action(channel,
            PCNT_CHANNEL_LEVEL_ACTION_KEEP,
            PCNT_CHANNEL_LEVEL_ACTION_INVERSE));
        return channel;
    };

    // 通道A配置（下降沿递减，上升沿递增）
    chanA_ = createChannel(config.gpioA, config.gpioB,
        PCNT_CHANNEL_EDGE_ACTION_DECREASE,
        PCNT_CHANNEL_EDGE_ACTION_INCREASE);

    // 通道B配置（上升沿递增，下降沿递减）
    chanB_ = createChannel(config.gpioB, config.gpioA,
        PCNT_CHANNEL_EDGE_ACTION_INCREASE,
        PCNT_CHANNEL_EDGE_ACTION_DECREASE);

    // 4. 创建事件队列
    eventQueue_ = xQueueCreate(10, sizeof(int));
    
    // 5. 注册事件回调
    pcnt_event_callbacks_t cbs = {
        .on_reach = &EC11Encoder::pcntCallback
    };
    ESP_ERROR_CHECK(pcnt_unit_register_event_callbacks(
        unit_, &cbs, eventQueue_));

    // 6. 启动计数器
    ESP_ERROR_CHECK(pcnt_unit_enable(unit_));
    ESP_ERROR_CHECK(pcnt_unit_clear_count(unit_));
    ESP_ERROR_CHECK(pcnt_unit_start(unit_));
}

EC11Encoder::~EC11Encoder() {
    if (unit_) {
        pcnt_unit_stop(unit_);
        pcnt_unit_disable(unit_);
        pcnt_del_unit(unit_);
    }
    vQueueDelete(eventQueue_);
}

bool EC11Encoder::pcntCallback(pcnt_unit_handle_t unit,
    const pcnt_watch_event_data_t* event, void* userCtx) {
    QueueHandle_t queue = static_cast<QueueHandle_t>(userCtx);
    const int value = event->watch_point_value;
    xQueueSendFromISR(queue, &value, nullptr);
    return false;
}

void EC11Encoder::registerEventCallback(EventCallback cb) {
    userCallback_ = cb;
}

int EC11Encoder::getCount() const {
    int count = 0;
    ESP_ERROR_CHECK(pcnt_unit_get_count(unit_, &count));
    return count;
}

void EC11Encoder::clearCount() {
    ESP_ERROR_CHECK(pcnt_unit_clear_count(unit_));
}

void EC11Encoder::pollEvents() {
    int event = 0;
    while (xQueueReceive(eventQueue_, &event, 0) == pdTRUE) {
        if (userCallback_) {
            userCallback_(event);
        }
    }
}
