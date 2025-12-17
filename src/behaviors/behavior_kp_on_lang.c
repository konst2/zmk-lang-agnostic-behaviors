/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_kp_on_lang

#include <stdlib.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <drivers/behavior.h>
#include <zephyr/logging/log.h>

#include <zmk/keymap.h>
#include <zmk/behavior.h>
#include <zmk/behavior_queue.h>
#include <zmk/language.h>
#include <zmk/events/keycode_state_changed.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

struct behavior_kp_on_lang_config {
    struct zmk_behavior_binding behavior_ru;  // клавиша для переключения на русский
    struct zmk_behavior_binding behavior_en;  // клавиша для переключения на английский
    uint8_t layer_en; // слой EN
    uint8_t layer_ru; // слой RU
    uint8_t language; // целевой язык для нажатия (слой_en - английский, слой_ru - русский)
};

struct behavior_kp_on_lang_data {
    const struct device *dev;
    struct k_work_delayable delayed_keypress;
    uint32_t keycode;
    bool press_pending; // флаг ожидания нажатия -- чтобы отпускание клавиши сработало только после отложенного нажатия
    bool release_pending; // флаг того что реальное отпускание уже произошло во время ожидания отложенного нажатия -- чтобы отработать отпускание сразу после отложенногонажатия
};

// функция переключения языка ОС
static void switch_os_language(
    uint8_t language_to_switch,
    const struct behavior_kp_on_lang_config *config,
    struct zmk_behavior_binding_event event
) {
    if (language_to_switch != get_os_language()) {

        // ### Нажимаем клавишу переключения в зависимости от выбранного языка
        if (language_to_switch == config->layer_en) {
            // Английский язык
            // Помещаем в очередь нажатие И отпускание 
            zmk_behavior_queue_add(&event, config->behavior_en, true, 0);
            zmk_behavior_queue_add(&event, config->behavior_en, false, 0);
            LOG_DBG("Switched to English language, layer %d", config->layer_en);
        } 
        else {
            // Русский язык
            // Помещаем в очередь нажатие И отпускание 
            zmk_behavior_queue_add(&event, config->behavior_ru, true, 0);
            zmk_behavior_queue_add(&event, config->behavior_ru, false, 0);
            LOG_DBG("Switched to Russian language, layer %d", config->layer_ru);
        }
        set_os_language(language_to_switch);
    }
}

// функция для отложенного нажатия клавиши
static void delayed_keypress_handler(struct k_work *work) {
    struct k_work_delayable *dwork = k_work_delayable_from_work(work);
    struct behavior_kp_on_lang_data *data = CONTAINER_OF(dwork, struct behavior_kp_on_lang_data, delayed_keypress);
    const struct device *dev = data->dev;
    const struct behavior_kp_on_lang_config *config = dev->config;

    raise_zmk_keycode_state_changed_from_encoded(data->keycode, true, k_uptime_get());
    data->press_pending = false;

    // если во время ожидания произошло реальное отпускание, отрабатываем его
    if (!data->release_pending) {
        return;
    }
    data->release_pending = false;

    // отпускаем клавишу
    raise_zmk_keycode_state_changed_from_encoded(data->keycode, false, k_uptime_get());
    // переключаем язык клавиатуры назад, если нужно
    struct zmk_behavior_binding_event dummy_event = {
        .position = 0,
        .timestamp = k_uptime_get(),
    };
    switch_os_language(get_kb_language(), config, dummy_event);
}

static int behavior_kp_on_lang_init(const struct device *dev) { 
    struct behavior_kp_on_lang_data *data = dev->data;
    
    data->dev = dev;
    data->press_pending = false;
    data->release_pending = false;
    k_work_init_delayable(&data->delayed_keypress, delayed_keypress_handler);
    return 0;
};


static int kp_on_lang_keymap_binding_pressed(struct zmk_behavior_binding *binding,
                                             struct zmk_behavior_binding_event event) {
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    const struct behavior_kp_on_lang_config *config = dev->config;
    struct behavior_kp_on_lang_data *data = dev->data;

    // 1. переключаем язык клавиатуры, если он не совпадает с целевым
    if (get_os_language() != config->language) {

        // Нажимаем клавишу переключения в зависимости от выбранного языка
        switch_os_language(config->language, config, event);

        // 2. Возвращаем нажатие самой клавиши с задержкой, чтобы сработало переключения языков
        data->keycode = binding->param1;
        k_work_cancel_delayable(&data->delayed_keypress); // отменяем предыдущее нажатие если пользователь быстро снова нажал клавишу
        data->press_pending = true;
        k_work_reschedule_for_queue(&k_sys_work_q, &data->delayed_keypress, K_MSEC(KP_ON_LANG_DELAY_MS));
        return ZMK_BEHAVIOR_OPAQUE;
    }

    // 2. Возвращаем нажатие самой клавиши сразу
    return raise_zmk_keycode_state_changed_from_encoded(binding->param1, true, event.timestamp);
}


static int kp_on_lang_keymap_binding_released(struct zmk_behavior_binding *binding,
                                              struct zmk_behavior_binding_event event) {
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    const struct behavior_kp_on_lang_config *config = dev->config;
    struct behavior_kp_on_lang_data *data = dev->data;

    // 0. проверяем, не произошло ли реальное отпускание во время ожидания отложенного нажатия
    if (data->press_pending) {
        data->release_pending = true;
        return ZMK_BEHAVIOR_OPAQUE;
    }

    // 1. переключаем язык клавиатуры назад, если нужно
    switch_os_language(get_kb_language(), config, event);

    // 2. Возвращаем отпускание самой клавиши
    return raise_zmk_keycode_state_changed_from_encoded(binding->param1, false, event.timestamp);
}


static const struct behavior_driver_api behavior_kp_on_lang_driver_api = {
    .binding_pressed = kp_on_lang_keymap_binding_pressed,
    .binding_released = kp_on_lang_keymap_binding_released};

#define KP_ON_LANG_INST(n)                                                                         \
    static struct behavior_kp_on_lang_data behavior_kp_on_lang_data_##n = {};                      \
    static struct behavior_kp_on_lang_config behavior_kp_on_lang_config_##n = {                    \
        .behavior_en = ZMK_KEYMAP_EXTRACT_BINDING(0, DT_DRV_INST(n)),                              \
        .behavior_ru = ZMK_KEYMAP_EXTRACT_BINDING(1, DT_DRV_INST(n)),                              \
        .layer_en = DT_INST_PROP(n, en_layer),                                                     \
        .layer_ru = DT_INST_PROP(n, ru_layer),                                                     \
        .language = DT_INST_PROP(n, language),                                                     \
    };                                                                                             \
    BEHAVIOR_DT_INST_DEFINE(n, behavior_kp_on_lang_init, NULL, &behavior_kp_on_lang_data_##n,      \
                            &behavior_kp_on_lang_config_##n, APPLICATION,                          \
                            CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &behavior_kp_on_lang_driver_api);

DT_INST_FOREACH_STATUS_OKAY(KP_ON_LANG_INST)
