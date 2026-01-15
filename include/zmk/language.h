/*
 * Copyright (c) 2022 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdint.h>

// Задержка на переключение языка системы в мс
#define KP_ON_LANG_DELAY_MS 50

// текущий язык клавиатуры -- 0 английский, 1 -- русский
uint8_t get_kb_language();
void set_kb_language(uint8_t lang);

// текущий язык компьютера -- 0 английский, 1 -- русский
uint8_t get_os_language();
void set_os_language(uint8_t lang);

// счётчик нажатых на текущий момент модификаторов CTRL, ALT, CMD
uint8_t get_modifiers_counter();
void increment_modifiers_counter();
void decrement_modifiers_counter();
void reset_modifiers_counter();

// сохранить активный язык (слой) при нажатии первого модификатора
void save_language_before_modifiers(uint8_t language);
// получить сохранённый язык (слой) до нажатия модификаторов
uint8_t get_language_before_modifiers();

// функция переключения языка ОС
void switch_os_language(
    uint8_t language_to_switch,
    uint8_t layer_en,
    const struct zmk_behavior_binding behavior_ru,
    const struct zmk_behavior_binding behavior_en,
    struct zmk_behavior_binding_event event
);
