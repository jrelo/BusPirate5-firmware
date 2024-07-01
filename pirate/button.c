#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pirate.h"
#include "system_config.h"
#include "pirate/button.h"
#include "hardware/timer.h"

#define BP_BUTTON_SHORT_PRESS_MS 550
#define BP_BUTTON_DOUBLE_TAP_MS 250
#define DEBOUNCE_STATE_MASK 0xF000

typedef enum {
    BUTTON_STATE_IDLE,
    BUTTON_STATE_PRESSED,
    BUTTON_STATE_LONG_PRESS,
    BUTTON_STATE_DOUBLE_TAP_WAIT
} button_state_t;

static button_state_t button_state = BUTTON_STATE_IDLE;
static bool button_pressed = false;
static absolute_time_t press_start_time;
static absolute_time_t last_press_time;
static enum button_codes button_code = BP_BUTT_NO_PRESS;
static uint16_t debounce_state_history = 0; 

// timer variables
static struct repeating_timer button_timer;
static bool timer_active = false;
static bool button_debounced = false;

// poll the value of button button_id
bool button_get(uint8_t button_id){
    return gpio_get(EXT1);
} 
// debounce button by requiring 12 consecutive reads of button being pressed
bool debounce_switch() {
       debounce_state_history =
        (debounce_state_history << 1) |  // make room in the history
        !button_get(0) |  // one bit, unless button is pressed
        DEBOUNCE_STATE_MASK;
    return (debounce_state_history == DEBOUNCE_STATE_MASK);
}
// timer callback function for button debouncing
bool button_timer_callback(struct repeating_timer *t) {
    if (debounce_switch()) {
        button_debounced = true;
        return false; // stop the timer
    }
    return true; // continue the timer
}// check button press type
enum button_codes button_check_press(uint8_t button_id) {
    if (button_pressed) {
        enum button_codes press_code = button_code;
        button_code = BP_BUTT_NO_PRESS;
        button_pressed = false;
        return press_code;
    }
    return BP_BUTT_NO_PRESS;
}
// example irq callback handler, copy for your own uses
void button_irq_callback(uint gpio, uint32_t events){
    if (events & GPIO_IRQ_EDGE_RISE) {
        press_start_time = get_absolute_time();
        if (!timer_active) {
            add_repeating_timer_ms(1, button_timer_callback, NULL, &button_timer);
            timer_active = true;
        }
    }

    if (events & GPIO_IRQ_EDGE_FALL) {
        absolute_time_t press_end_time = get_absolute_time();
        int64_t duration_ms = absolute_time_diff_us(press_start_time, press_end_time) / 1000;

        if (button_debounced) {
            switch (button_state) {
                case BUTTON_STATE_IDLE:
                    button_state = BUTTON_STATE_PRESSED;
                    break;
                case BUTTON_STATE_PRESSED:
                    if (duration_ms >= BP_BUTTON_SHORT_PRESS_MS) {
                        button_code= BP_BUTT_LONG_PRESS;
                        button_state = BUTTON_STATE_IDLE;
                    } else {
                        button_state = BUTTON_STATE_DOUBLE_TAP_WAIT;
                        last_press_time = press_end_time;
                    }
                    break;
                case BUTTON_STATE_DOUBLE_TAP_WAIT: {
                    int64_t double_press_duration = absolute_time_diff_us(last_press_time, press_end_time) / 1000;
                    if (double_press_duration <= BP_BUTTON_DOUBLE_TAP_MS) {
                        button_code = BP_BUTT_DOUBLE_TAP;
                        button_state = BUTTON_STATE_IDLE;
                    } else {
                        button_code = BP_BUTT_SHORT_PRESS;
                        button_state = BUTTON_STATE_IDLE;
                    }
                    break;
                }
                case BUTTON_STATE_LONG_PRESS:
                    button_code = BP_BUTT_LONG_PRESS;
                    button_state = BUTTON_STATE_IDLE;
                    break;
                default:
                    button_state = BUTTON_STATE_IDLE;
                    break;
            }
            
            button_pressed = true;
            button_debounced = false;
            timer_active = false;
            cancel_repeating_timer(&button_timer);
        }
    }
    
    gpio_acknowledge_irq(gpio, events);   
    gpio_set_irq_enabled(gpio, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
}
// enable the irq for button button_id
void button_irq_enable(uint8_t button_id, void *callback){
    button_pressed=false;
    gpio_set_irq_enabled_with_callback(EXT1, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, callback);
} 
// disable the irq for button button_id
void button_irq_disable(uint8_t button_id){
    gpio_set_irq_enabled(EXT1, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, false);
    button_pressed=false;
}
// initialize all buttons
void button_init(void){
    gpio_set_function(EXT1, GPIO_FUNC_SIO);
    gpio_set_dir(EXT1, GPIO_IN);
    gpio_pull_down(EXT1);
}
