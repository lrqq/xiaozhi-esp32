// EC11Encoder.h 头文件
#pragma once
#include <functional>
#include "driver/gpio.h"
#include "driver/pulse_cnt.h"
#include "freertos/FreeRTOS.h"

class EC11Encoder {
public:
    // 配置结构体(前置声明)
    struct Config;

    // 事件回调类型
    using EventCallback = std::function<void(int)>;

    // 构造与析构
    EC11Encoder(const Config& config);
    ~EC11Encoder();

    // 禁用拷贝构造和赋值
    EC11Encoder(const EC11Encoder&) = delete;
    EC11Encoder& operator=(const EC11Encoder&) = delete;

    // API方法
    void registerEventCallback(EventCallback cb);
    int getCount() const;
    void clearCount();
    void pollEvents();

private:
    // 私有成员
    pcnt_unit_handle_t unit_ = nullptr;
    pcnt_channel_handle_t chanA_ = nullptr;
    pcnt_channel_handle_t chanB_ = nullptr;
    QueueHandle_t eventQueue_ = nullptr;
    EventCallback userCallback_;

    // 静态回调方法
    static bool pcntCallback(
        pcnt_unit_handle_t unit,
        const pcnt_watch_event_data_t* event,
        void* userCtx);
};

// 配置结构体定义
struct EC11Encoder::Config {
    gpio_num_t gpioA;          // 编码器A相引脚
    gpio_num_t gpioB;          // 编码器B相引脚
    int lowLimit = -1000;      // 计数下限
    int highLimit = 1000;      // 计数上限
    uint32_t glitchFilterNs = 1000; // 硬件滤波时间(ns)
};
