#pragma once
#include "LedPwmController.h"
#include "HomeSpan.h" 

// Forward Declrations 
struct DimSmartLed;
struct DeviceBrightness;
struct DevicePowerStatus;

enum ButtonState {
    RELEASED = 0,
    PRESSED_DOWN = 1,
    SINGLE_RELEASED = 2,
    DOUBLE_RELEASED = 3,
    LONG_SUSTAINED_DOWN = 4
  };

struct DeviceBrightness : Service::LightBulb {      

    DimSmartLed *parent;   
    SpanCharacteristic *state;
    DeviceBrightness(DimSmartLed *pParent, int increment_notches);
    boolean update();  

    int32_t get_new_value() { 
        return this->state->getNewVal();
    }

    int32_t get_value() { 
        return this->state->getVal();
    }

    void set_value(int32_t val) { 
        this->state->setVal(val);
    }
};

struct DevicePowerStatus : Service::LightBulb {      

    DimSmartLed *parent;   
    SpanCharacteristic *state;
    ButtonState button_state;
    int button_pin;
    unsigned long last_long_press_tick_ms;
    unsigned long long_press_tick_duration_ms;

    DevicePowerStatus(
        DimSmartLed *pParent, 
        int button_pin,
        uint16_t single_press_ms,
        uint16_t double_press_ms,
        uint16_t long_press_ms,
        unsigned long long_press_repeat_ms
    );
    boolean update();
    void button(int pin, int press_type);
    void loop();

    void check_button_status();

    boolean get_new_value() { 
        return this->state->getNewVal() == 1;;
    }

    boolean get_value() { 
        return this->state->getVal() == 1;;
    }

    void set_value(boolean val) { 
        if(val) {
            this->state->setVal(1);
        } else {
            this->state->setVal(0);
        }      
    }
};


class DimSmartLed {
private:

    LedPwmController *output_led;   
    DeviceBrightness *brightness; 
    DevicePowerStatus *power; 

    // Double press functionality 
    int32_t double_press_brightness;
    int32_t single_press_brightness;
    boolean double_press_mode_enabled;
    
    // Long press functionality
    int32_t long_press_increment;
    int32_t long_press_min;
    int32_t long_press_max;

public:
    DimSmartLed(int ledPin, int buttonPin);
    void toggle_power_status();
    void toggle_power_on();
    void toggle_power_off();
    void toggle_double_press_brightness();
    void cycle_brightness_next_step();

    boolean on_power_status_changed();
    boolean on_brightness_changed();
    void on_button_press(ButtonState button_state);
    void on_next_loop_cycle();

};


  