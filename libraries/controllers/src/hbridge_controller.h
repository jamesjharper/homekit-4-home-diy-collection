#pragma once
#include "HomeSpan.h" 
#include "hbridge.h"


#include "channel_properties.h"


class LedPwmHBridgeController {      
    float a_actual_brightness;
    float b_actual_brightness;


    unsigned long last_tick_micros;
    unsigned long tick_duration_micros;

    std::unique_ptr<LedPwmHBridge> h_bridge;
    std::shared_ptr<ChannelProperties> ab_channel;
    std::shared_ptr<ChannelProperties> a_channel;
    std::shared_ptr<ChannelProperties> b_channel;

public:
    LedPwmHBridgeController(
        HBridgeFrequency freq,
        HBridgeCalibration cal,
        HBridgeGpioPins gpio_pins, 
        ControlMode mode 
    ) {
        this->last_tick_micros = 0;
        this->tick_duration_micros = 20000;

        LOG1(" this->a_actual_brightness = 0.0f\n");
        this->a_actual_brightness = 0.0f;
        this->b_actual_brightness = 0.0f;
        
        this->h_bridge = std::unique_ptr<LedPwmHBridge>(
            new LedPwmHBridge(
                freq,
                cal,
                gpio_pins,
                mode
            )
        );

        this->ab_channel = std::shared_ptr<ChannelProperties>(new ChannelProperties(1.0, true));
        this->a_channel = std::shared_ptr<ChannelProperties>(new ChannelProperties(1.0, true, this->ab_channel));
        this->b_channel = std::shared_ptr<ChannelProperties>(new ChannelProperties(1.0, true, this->ab_channel));
    } 

    std::shared_ptr<ChannelProperties> channel(HBridgeChannel channel) { 
        if (channel == HBridgeChannel::A) {
            return this->a_channel;
        } else if (channel == HBridgeChannel::B) {
            return this->b_channel;
        } 
        return this->ab_channel;
    }

    void step_next() { 

        unsigned long current_micros = micros();
        if(current_micros - this->last_tick_micros <= this->tick_duration_micros) {
            return;
        }

        this->last_tick_micros = current_micros;
        this->step_next_brightness(HBridgeChannel::A);
        this->step_next_brightness(HBridgeChannel::B);
        this->h_bridge->apply_duty_changes();
    } 

private:
    void set_actual_brightness(HBridgeChannel channel, float brightness) { 
        auto lum = pow (2.0f, brightness) - 1.0f;
        this->h_bridge->set_value(channel, lum);     
    }

    float get_actual_brightness(HBridgeChannel channel) { 
        if (channel == HBridgeChannel::A) {
            return this->a_actual_brightness;
        } 
        if (channel == HBridgeChannel::B) {
            return this->b_actual_brightness;
        }
        return 0.0;
    }


    void step_next_brightness(HBridgeChannel channel) { 

    
        auto channel_properties = this->channel(channel);
        float target_brightness = channel_properties->get_target_brightness();
        float actual_brightness = channel_properties->get_current_brightness();  
       
        // Values should be identical, if we have reached our target brightness
        if(target_brightness == actual_brightness) { 
            channel_properties->steady_state();
            return;
        }

        float delta = target_brightness - actual_brightness;
        float increment = 0;
        if (target_brightness < actual_brightness) {
            // ease off
            increment = delta / channel_properties->get_ease_off();
        } else {
            // ease on
            increment = delta / channel_properties->get_ease_on();
        }

        if(abs(increment) <= FLOAT_ESPSILON) {

            auto lum = pow (2.0f, target_brightness) - 1.0f;
            LOG1("channel[");
          //  LOG1(increment);
            LOG1("].actual_brightness = ");
            LOG1(target_brightness);
            LOG1("% / ");
            LOG1(lum);
            LOG1(" duty\n");

            channel_properties->set_current_brightness(target_brightness);
            // if we are close enoughs to the target value, snap exactly to the target value
            this->set_actual_brightness(channel, target_brightness);
            

        } else {
            channel_properties->set_current_brightness(target_brightness);
            this->set_actual_brightness(channel, actual_brightness + increment);
        }
    }
};
