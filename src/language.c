#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <drivers/behavior.h>
#include <zephyr/logging/log.h>

#include <zmk/keymap.h>
#include <zmk/behavior.h>
#include <zmk/behavior_queue.h>
#include <zmk/language.h>
#include <zmk/events/keycode_state_changed.h>
#include <stdint.h>


// текущий язык клавиатуры -- 0 английский, 1 -- русский
uint8_t kb_language_state = 0;
uint8_t get_kb_language() { 
    return kb_language_state; 
}
void set_kb_language(uint8_t lang) {
        kb_language_state = lang;
}


// текущий язык компьютера -- 0 английский, 1 -- русский
uint8_t os_language_state = 0;
uint8_t get_os_language() { 
    return os_language_state; 
}
void set_os_language(uint8_t lang) {
        os_language_state = lang;
}


// язык клавиатуры который был актуален до нажатия первого модификатора
uint8_t kb_language_before_modifiers = 0;
uint8_t get_kb_language_before_modifiers() { 
    return kb_language_before_modifiers; 
}
void set_kb_language_before_modifiers(uint8_t lang) {
        kb_language_before_modifiers = lang;
}


// счётчик нажатых на текущий момент модификаторов CTRL, ALT, CMD
// используется чтобы модификаторы работали только на анлийской раскладке
uint8_t modifiers_counter = 0;
// получить значение счётчика нажатых модификаторов
uint8_t get_modifiers_counter() { 
    return modifiers_counter; 
}
// добавить 1 к счётчику нажатых модификаторов
void increment_modifiers_counter() {
    modifiers_counter++;
}
// уменьшить счётчик нажатых модификаторов на 1
void decrement_modifiers_counter() {
    if (modifiers_counter > 0) {
        modifiers_counter--;
    }
}
// обнулить счётчик нажатых модификаторов
void reset_modifiers_counter() {
    modifiers_counter = 0;
}

// активный язык (слой) до нажатия модификаторов
uint8_t modifiers_before_language = 0;
// сохранить активный язык (слой) при нажатии первого модификатора
void save_language_before_modifiers(uint8_t language) {
    modifiers_before_language = language;
}
// получить сохранённый язык (слой) до нажатия модификаторов
uint8_t get_language_before_modifiers() {
    return modifiers_before_language;
}


// функция переключения языка ОС
void switch_os_language(
    uint8_t language_to_switch,
    uint8_t layer_en,
    const struct zmk_behavior_binding behavior_ru,
    const struct zmk_behavior_binding behavior_en,
    struct zmk_behavior_binding_event event
) {
    if (language_to_switch != get_os_language()) {

        // ### Нажимаем клавишу переключения в зависимости от выбранного языка
        if (language_to_switch == layer_en) {
            // Английский язык
            // Помещаем в очередь нажатие И отпускание 
            zmk_behavior_queue_add(&event, behavior_en, true, 0);
            zmk_behavior_queue_add(&event, behavior_en, false, 0);
        } 
        else {
            // Русский язык
            // Помещаем в очередь нажатие И отпускание 
            zmk_behavior_queue_add(&event, behavior_ru, true, 0);
            zmk_behavior_queue_add(&event, behavior_ru, false, 0);
        }
        set_os_language(language_to_switch);
    }
}
