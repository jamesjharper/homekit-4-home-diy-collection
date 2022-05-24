#pragma once
#include "HomeSpan.h" 

#define FLOAT_ESPSILON 0.001

class ChannelProperties {  
    private:

    float target_brightness;
    float set_brightness;
    float current_brightness;
    float ease_on;
    float ease_off;
    boolean power_status;
    std::shared_ptr<ChannelProperties> parent;

    public:

    ChannelProperties(float brightness, boolean power_status, std::shared_ptr<ChannelProperties> parent = NULL) {
        this->target_brightness = brightness;
        this->set_brightness = brightness;
        this->power_status = power_status;
        this->current_brightness = 0.0f;
        this->parent = parent;
        this->ease_on = 50.0f;
        this->ease_off = 50.0f;
    }
    
    boolean get_power_status() {
        if (this->parent && !this->parent->get_power_status()) {
            return false;
        }
        return this->power_status;
    }

    float get_ease_on() {
        return this->ease_on;
    }

    float get_ease_off() {
        return this->ease_off;
    }

    float get_target_brightness() {
        if (!this->get_power_status()) {
            return 0.0; // channel is off
        }

        if (this->parent) 
            return this->parent->get_target_brightness() * this->target_brightness;
    

        return this->target_brightness;
    }

    float get_current_brightness() {
        return this->current_brightness;
    }

    void set_power_status(boolean new_power_status) { 
        this->power_status = new_power_status;

        LOG1("channel[");
       // LOG1(this->channel.to_str());
        LOG1("].power_status = ");
        if (this->power_status) {
            LOG1("ON\n");
        } else {
            LOG1("OFF\n");
        }
    }

    void toggle_power_status() { 
        set_power_status(!this->power_status);
    }

    void set_target_brightness(int32_t percentage_brightness) { 

        LOG1("set_target_brightness\n");
        this->target_brightness = percentage_brightness / 100.0f;
        
        if(this->target_brightness <= FLOAT_ESPSILON) {
            this->target_brightness = 0.0;
        }
        this->set_brightness = this->target_brightness;

        LOG1("channel[");
      //  LOG1(this->channel.to_str());
        LOG1("].target_brightness = ");
        LOG1(this->target_brightness);
        LOG1("\n");
    }

    void set_current_brightness(float current_brightness) { 
        this->current_brightness = current_brightness;
    }

    void steady_state() { 

        float upper = this->set_brightness * 100.0f;
        float lower = (upper / 10.0f) * 7;

        this->target_brightness = random(lower, upper) / 100.0f;
        this->ease_on = random(2, 3);
        this->ease_off = random(2, 3);
    }
};
