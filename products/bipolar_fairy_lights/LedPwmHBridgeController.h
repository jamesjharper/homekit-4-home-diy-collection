
#pragma once
#include "HomeSpan.h" 
#include "hbridge.h"

#define FLOAT_ESPSILON 0.0001


class ChannelProperties {  
    private:

    float brightness;
    boolean power_status;
    ChannelProperties* parent;

    public:

    ChannelProperties(float brightness, boolean power_status, ChannelProperties* parent = NULL) {
        this->brightness = brightness;
        this->power_status = power_status;
        this->parent = parent;
    }

    boolean get_power_status() {
        if (this->parent && !this->parent->get_power_status()) {
            return false;
        }
        return this->power_status;
    }

    float get_target_brightness() {
        if (!this->get_power_status()) {
            return 0.0; // channel is off
        }

        if (this->parent) {
            return this->parent->get_target_brightness() * this->brightness;
        }

        return this->brightness;
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
        this->brightness = percentage_brightness / 100.0;

        if(this->brightness <= FLOAT_ESPSILON) {
            this->brightness = 0.0;
        }

        LOG1("channel[");
      //  LOG1(this->channel.to_str());
        LOG1("].target_brightness = ");
        LOG1(this->brightness);
        LOG1("\n");
    }
};

struct LedPwmHBridgeController {      

    LedPwmHBridge *h_bridge; 
    int32_t brightness_step;
    unsigned long last_tick_micros;
    unsigned long tick_duration_micros;

    std::unique_ptr<ChannelProperties> ab_channel;
    std::unique_ptr<ChannelProperties> a_channel;
    std::unique_ptr<ChannelProperties> b_channel;

    LedPwmHBridgeController(
        uint8_t a_pin, 
        uint8_t b_pin,
        uint8_t pwm_pin,
        uint16_t _freq,
        uint16_t resolution,
        uint8_t ratio_channel,
        uint8_t pwm_channel
    ) {
        this->last_tick_micros = 0;
        this->tick_duration_micros = 2000;


        HBridgeFrequency freq = {
            .intensity_hz = 10000,
            .ratio_hz = 2500
        };

        HBridgeCalibration cal = {
            .intensity_threshold = 0.45f,
            .left_ratio_threshold = 0.08f,
            .right_ratio_threshold = 0.9f
        };

        HBridgeGpioPins ab_gpio = {
            .left_pin = a_pin, 
            .right_pin = b_pin,
            .intensity_pin = pwm_pin
        };

        this->h_bridge = new LedPwmHBridge(
            freq,
            cal,
            ab_gpio,
            ControlMode::LEVEL_A_AND_LEVEL_B & ControlMode::CALIBRATED
        );

        this->ab_channel = std::unique_ptr<ChannelProperties>(new ChannelProperties(1.0, true));
        this->a_channel = std::unique_ptr<ChannelProperties>(new ChannelProperties(1.0, true, &*this->ab_channel));
        this->b_channel = std::unique_ptr<ChannelProperties>(new ChannelProperties(1.0, true, &*this->ab_channel));
    } 

    ChannelProperties* channel(HBridgeChannel channel) { 
        if (channel == HBridgeChannel::A) {
            return &*this->a_channel;
        } else if (channel == HBridgeChannel::B) {
            return &*this->b_channel;
        } 
        return &*this->ab_channel;
    }

    float get_actual_brightness(HBridgeChannel channel) { 
        return this->h_bridge->get_value(channel);   
    }

    void set_actual_brightness(HBridgeChannel channel, float brightness) { 
        this->h_bridge->set_value(channel, brightness);     
    }

    boolean step_next_brightness() { 
        return 
            this->step_next_brightness(HBridgeChannel::A) || 
            this->step_next_brightness(HBridgeChannel::B);
    }

    boolean step_next_brightness(HBridgeChannel channel) { 

        float target_brightness = this->channel(channel)->get_target_brightness();
        float actual_brightness = this->get_actual_brightness(channel);  
       
        // Values should be identical, if we have reached our target brightness
        if(target_brightness == actual_brightness) { 
            return false;
        }

        float delta = target_brightness - actual_brightness;
        float increment = 0;
        if (target_brightness < actual_brightness) {
          // ease off
          increment = delta / 100;
        } else {
          // ease on
          increment = delta / 100;
        }

        if(abs(increment) <= FLOAT_ESPSILON) {
            // if we are close enoughs to the target value, just snap to the target value
            this->set_actual_brightness(channel, target_brightness);

            LOG1("channel[");
            //LOG1(channel.to_str());
            LOG1("].actual_brightness = ");
            LOG1(target_brightness);
            LOG1("%\n");
        } else {
            this->set_actual_brightness(channel, actual_brightness + increment);
        }

        return true;
    } 


    void step_next() { 

        unsigned long current_micros = micros();
        if(current_micros - this->last_tick_micros <= this->tick_duration_micros) {
            return;
        }
        this->last_tick_micros = current_micros;

        if (this->step_next_brightness()) {
            this->h_bridge->apply_duty_changes();     
        }
    } 
};
