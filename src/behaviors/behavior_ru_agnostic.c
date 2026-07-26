/*
Данный behavior смотрит на символ который требуется нажать.
Потом, если у нас сейчас в системе русская раскладка,
ищем как нажать этот символ на ней и нажимаем именно его.
Пример - "." на английской и русской раскладках требуют нажатия разных
клавиш. Мы же теперь можем в зависимости от языка системы нажать точку
на текущей раскладке -- то есть этот символ останется на нашей раскладке
на том же месте, и при этом не будет требовать переключения языка раскладки.
*/

//#include <stdlib.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
//#include <drivers/behavior.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>  // ???

#include <zmk/event_manager.h>
#include <zmk/keymap.h>
#include <zmk/behavior.h>
#include <zmk/endpoints.h>
#include <zmk/hid.h>
#include <zmk/language.h>


#define DT_DRV_COMPAT zmk_behavior_ru_agnostic
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

struct behavior_ru_agnostic_config {
    uint8_t layer_en; // слой EN
    uint8_t layer_ru; // слой RU
};


// Глобальный массив для отслеживания нажатий по позициям клавиш
struct pressed_state {
    uint32_t actual_param; // Запакованный параметр (моды + keycode)
    bool     is_active;
};
static struct pressed_state pressed_states[ZMK_KEYMAP_LEN] = {0};


// ------------ Мапинг клавиш
// Таблица маппинга (только базовые клавиши)
// Формат: { Английская_базовая, Русская_базовая }
static const uint32_t mappings[][2] = {
    { DOT,   SLASH }, // . -> . (на русской раскладке)
    // { COMMA, B },  // Раскомментируй и добавь остальные
};
#define MAPPINGS_COUNT ARRAY_SIZE(mappings)

// Ищет замену для базовой клавиши в таблице. 
// Если замена не найдена, возвращает исходную клавишу.
static uint32_t find_mapped_key(uint32_t en_key) {
    for (int i = 0; i < MAPPINGS_COUNT; i++) {
        if (mappings[i][0] == en_key) {
            return mappings[i][1];
        }
    }
    return en_key; 
}
// ./ ------------ Мапинг клавиш


static int behavior_ru_agnostic_init(const struct device *dev) { return 0; };


static int ru_agnostic_pressed(struct zmk_behavior_binding *binding,
                                       struct zmk_behavior_binding_event event) {

    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    const struct behavior_ru_agnostic_config *config = dev->config;

    // 1. Берем код клавиши.
    uint32_t base_key = binding->param1;

    // 2. Проверяем раскладку и ищем замену, если нужно
    uint32_t key_to_send = base_key;
    if (get_kb_language() == config->layer_ru) { 
        key_to_send = find_mapped_key(base_key);
    }

    // 3. Сохраняем итоговый код клавиши для этой позиции, чтобы корректно отпустить именно его
    if (event.position < ZMK_KEYMAP_LEN) {
        pressed_states[event.position].actual_param = key_to_send;
        pressed_states[event.position].is_active = true;
    }

    // 4. Генерируем событие нажатия.
    return raise_zmk_keycode_state_changed_from_encoded(key_to_send, true, event.timestamp);

}

static int ru_agnostic_released(struct zmk_behavior_binding *binding,
                                        struct zmk_behavior_binding_event event) {
    // 1. Проверка на валидность позиции и активность
    if (event.position >= ZMK_KEYMAP_LEN || !pressed_states[event.position].is_active) {
        return -EINVAL;
    }

    // 2. Достаем сохраненный код клавиши, который мы реально нажали
    uint32_t key_to_release = pressed_states[event.position].actual_param;
    pressed_states[event.position].is_active = false; // Сбрасываем флаг

    // Генерируем событие отпускания
    return raise_zmk_keycode_state_changed_from_encoded(key_to_release, false, event.timestamp);
}

static const struct behavior_driver_api behavior_ru_agnostic_driver_api = {
    .binding_pressed = ru_agnostic_pressed,
    .binding_released = ru_agnostic_released};

#define RU_AGNOSTIC_INST(n)                                                                        \
    static struct behavior_ru_agnostic_config behavior_ru_agnostic_config_##n = {                  \
        .layer_en = DT_INST_PROP_OR(n, en_layer, 0),                                               \
        .layer_ru = DT_INST_PROP_OR(n, ru_layer, 0),                                               \
    };                                                                                             \
    BEHAVIOR_DT_INST_DEFINE(n, behavior_ru_agnostic_init, NULL, NULL,                              \
                            &behavior_ru_agnostic_config_##n, APPLICATION,                         \
                            CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &behavior_ru_agnostic_driver_api);

DT_INST_FOREACH_STATUS_OKAY(RU_AGNOSTIC_INST)
