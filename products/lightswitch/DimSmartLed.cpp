#include "DimSmartLed.h"


// DimSmartLed
// Public: 

DimSmartLed::DimSmartLed(int ledPin, int buttonPin) {

    int increment_notches = 100;

    this->output_led = new LedPwmController(ledPin, increment_notches);  
    this->brightness = new DeviceBrightness(this, increment_notches);
    this->power = new DevicePowerStatus(
        this, 
        /* pin */ buttonPin,
        /* single press ms */ 5, 
        /* double press ms */ 1000,
        /* long press ms */ 1500, 
        /* long repeat ms */ 200
    );
    

    // Assign power as the main device 
    this->power->addLink(this->brightness);

    this->double_press_brightness = 60;/* perecent */
    this->double_press_mode_enabled = false;
    this->single_press_brightness = this->brightness->get_value();


    this->long_press_increment = 5;/* perecent */
    this->long_press_min = 5; /* perecent */
    this->long_press_max = 100; /* perecent */

    this->output_led->set_power_status(this->power->get_value()); 
    this->output_led->set_target_brightness(this->brightness->get_value()); 
} 

void DimSmartLed::toggle_power_status() { 
    bool toggled_status = !this->power->get_value();
    this->power->state->setVal(toggled_status);
    this->output_led->set_power_status(toggled_status);
}

void DimSmartLed::toggle_power_on() { 
    if (!this->power->get_value()) {
        this->power->state->setVal(true);
        this->output_led->set_power_status(true);
    }
}

void DimSmartLed::toggle_power_off() { 
    if (this->power->get_value()) {
        this->power->state->setVal(false);
        this->output_led->set_power_status(false);
    }
}

void DimSmartLed::toggle_double_press_brightness() { 
    if(this->double_press_mode_enabled) {
        this->double_press_mode_enabled = false;
        this->output_led->set_target_brightness(this->single_press_brightness);
        this->brightness->set_value(this->single_press_brightness);
    } else {
        this->double_press_mode_enabled = true;
        this->output_led->set_target_brightness(this->double_press_brightness);
        this->brightness->set_value(this->double_press_brightness);
    }
    // if the status is not already on, make sure its on
    this->toggle_power_on();
}


void DimSmartLed::cycle_brightness_next_step() { 

    int32_t next_val = this->brightness->get_value() + this->long_press_increment;

    if (next_val < this->long_press_min) {
        this->long_press_increment *= -1;
        next_val = this->long_press_min;
    } else if (next_val > this->long_press_max) {
        this->long_press_increment *= -1;
        next_val = this->long_press_max;
    }

    this->double_press_mode_enabled = false;
    this->output_led->set_target_brightness(next_val);
    this->brightness->set_value(next_val);

    // if the status is not already on, make sure its on
    this->toggle_power_on();
}

// DimSmartLed
// Private: 

boolean DimSmartLed::on_power_status_changed() {    
    this->output_led->set_power_status(this->power->get_new_value());
    return true;            
}

boolean DimSmartLed::on_brightness_changed() {
    // Any change to brightness disables double press mode 
    this->double_press_mode_enabled = false;
    this->single_press_brightness = this->brightness->get_new_value();
    this->output_led->set_target_brightness(this->single_press_brightness);
    return true;            
}

void DimSmartLed::on_button_press(ButtonState button_state) {

    // If power is currently off, then the instant the light is pressed,
    // we turn it on. However as we wil receive a SINGLE_RELEASED event at some point,
    // we just set the LED status, the value will be persisted by SINGLE_RELEASED
    if (button_state == ButtonState::PRESSED_DOWN && !this->power->get_value()) {
        this->output_led->set_power_status(true);
        return;
    }

    // If short press is released, then power off 
    if(button_state == ButtonState::SINGLE_RELEASED) {
        this->toggle_power_status();
        return;
    } 

    // if double press is released, then toggle brightness
    if (button_state == ButtonState::DOUBLE_RELEASED) {
        this->toggle_double_press_brightness();
    }

    // if double press is released, then toggle brightness
    if (button_state == ButtonState::LONG_SUSTAINED_DOWN) {
        this->cycle_brightness_next_step();
    }
} 

void DimSmartLed::on_next_loop_cycle() { 
    this->output_led->step_next();
}


// DeviceBrightness

DeviceBrightness::DeviceBrightness(DimSmartLed *pParent, int increment_notches) : Service::LightBulb() {
    this->parent = pParent;   
    new Characteristic::Name("Brightness");
    new Characteristic::On(0,true);
    this->state = new Characteristic::Brightness(50, true);  
    this->state->setRange(0,increment_notches,1);
} 

boolean DeviceBrightness::update() { 
    return this->parent->on_brightness_changed();       
}

// DevicePowerStatus


DevicePowerStatus::DevicePowerStatus(
    DimSmartLed *pParent, 
    int button_pin,
    uint16_t single_press_ms,
    uint16_t double_press_ms,
    uint16_t long_press_ms,
    unsigned long long_press_repeat_ms
) : Service::LightBulb() {
    this->parent = pParent;   
    new Characteristic::Name("Power");
    this->state = new Characteristic::On(0,true);
    this->button_pin = button_pin;
    this->button_state = ButtonState::RELEASED;

    this->last_long_press_tick_ms = 0;
    this->long_press_tick_duration_ms = long_press_repeat_ms;
    new SpanButton(this->button_pin, 
        /* long press ms */ long_press_ms, 
        /* single press ms */ single_press_ms, 
        /* double press ms */ double_press_ms
    ); 
} 

boolean DevicePowerStatus::update() {    
    return this->parent->on_power_status_changed();          
}

void DevicePowerStatus::button(int pin, int press_type) {
    LOG1("pin[");
    LOG1(pin);
    LOG1("].button_press_status = ");

    if (press_type == SpanButton::SINGLE) {
        LOG1("SINGLE_RELEASED\n"); 
        this->parent->on_button_press(ButtonState::SINGLE_RELEASED);
        this->button_state = ButtonState::RELEASED;

    } else if (press_type == SpanButton::DOUBLE) {
        LOG1("DOUBLE_RELEASED\n"); 
        this->parent->on_button_press(ButtonState::DOUBLE_RELEASED);
        this->button_state = ButtonState::RELEASED;

    } else if (press_type == SpanButton::LONG) {
        LOG1("LONG_DOWN\n"); 
        this->button_state = ButtonState::LONG_SUSTAINED_DOWN;
    }
    
} 

void DevicePowerStatus::loop() {    
    this->check_button_status();
    this->parent->on_next_loop_cycle();
}

void DevicePowerStatus::check_button_status() {

    // send PRESSED_DOWN event if the button has just been pressed
    if(this->button_state == ButtonState::RELEASED) {
        if (digitalRead(this->button_pin) == LOW) {
            LOG1("pin[");
            LOG1(this->button_pin);
            LOG1("].button_press_status = ");
            LOG1("PRESSED_DOWN\n");
            this->button_state = ButtonState::PRESSED_DOWN;
            this->parent->on_button_press(ButtonState::PRESSED_DOWN);
        } 

        return;
    }
    
    // send repeating LONG_DOWN for each nth ms which the button press is sustained 
    if(this->button_state == ButtonState::LONG_SUSTAINED_DOWN) {
        unsigned long current_millis = millis();
        if(current_millis - this->last_long_press_tick_ms <= this->long_press_tick_duration_ms){
            return;
        }
        this->last_long_press_tick_ms = current_millis;

        LOG1("pin[");
        LOG1(this->button_pin);
        LOG1("].button_press_status = ");

        
        if (digitalRead(this->button_pin) == LOW) {
            LOG1("LONG_SUSTAINED_DOWN\n");
            this->parent->on_button_press(ButtonState::LONG_SUSTAINED_DOWN);
        } else {
            // Clear state if released
            LOG1("LONG_RELEASE\n");
            this->button_state = ButtonState::RELEASED;
            this->last_long_press_tick_ms = 0;
        }
    }

}