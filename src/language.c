#include <zmk/language.h>

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
