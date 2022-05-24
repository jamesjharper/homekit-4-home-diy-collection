#pragma once
#include <stdint.h>
#include <memory>

#if ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#include "driver/ledc.h"
#include "channels.h"

#include "HomeSpan.h" 

struct HBridgeCalibration {
    float intensity_threshold;
    float left_ratio_threshold;
    float right_ratio_threshold;
};

struct HBridgeFrequency {
    uint16_t intensity_hz;
    uint16_t ratio_hz;
};

struct HBridgeGpioPins {
    int16_t left_pin;
    int16_t right_pin;
    int16_t intensity_pin;
};

enum HBridgeChannel : uint8_t {
    A = 1 << 0,
    B = 1 << 1,
       
    AB = 3,
};

inline HBridgeChannel operator|(HBridgeChannel a, HBridgeChannel b)
{
    return static_cast<HBridgeChannel>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

inline HBridgeChannel operator&(HBridgeChannel a, HBridgeChannel b)
{
    return static_cast<HBridgeChannel>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}

enum ControlMode : uint8_t {
    UNCALIBRATED = 0,
    NO_CALIBRATION_REQUIRED = 0,
    CALIBRATED = 1 << 0,

    // AB + Ratio mode is used to calibrate the upper and lower
    // limits of the H bridge device 
    LEVEL_AB_AND_RATIO = 1 << 1,


    LEVEL_A_AND_LEVEL_B = 1 << 2,
};


inline ControlMode operator|(ControlMode a, ControlMode b)
{
    return static_cast<ControlMode>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

inline ControlMode operator&(ControlMode a, ControlMode b)
{
    return static_cast<ControlMode>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}

struct LedPwmHBridge {      

    std::unique_ptr<PwmChannel> intensity_channel;
    std::unique_ptr<PwmChannel> ratio_channel;

    ControlMode mode;
    HBridgeCalibration cal;

    float a_level, b_level;

    boolean pending_changes = false;

    LedPwmHBridge(
        HBridgeFrequency freq,
        HBridgeCalibration cal,
        HBridgeGpioPins gpio_pins, 
        ControlMode mode = ControlMode::LEVEL_A_AND_LEVEL_B & ControlMode::CALIBRATED
    ) {
        auto intensity_clock = ChannelTimerConfig(freq.intensity_hz);
        auto ratio_clock = ChannelTimerConfig(freq.ratio_hz);

        this->intensity_channel = std::unique_ptr<PwmChannel>(new PwmChannel(&intensity_clock));
        this->ratio_channel = std::unique_ptr<PwmChannel>(new PwmChannel(&ratio_clock));
        this->mode = mode;
        this->cal = cal;
        this->a_level = 0.0f;
        this->b_level = 0.0f;
        this->pending_changes = false;

        if (gpio_pins.intensity_pin > 0)
            this->intensity_channel->assign_pin((uint8_t)gpio_pins.intensity_pin);

        if (gpio_pins.left_pin > 0)
            this->ratio_channel->assign_pin((uint8_t)gpio_pins.left_pin);

        if (gpio_pins.right_pin > 0)
            this->ratio_channel->assign_pin_inverted((uint8_t)gpio_pins.right_pin);
    }

    void set_value(HBridgeChannel channel, float level) {
        if(level > 2)
            level = 2;

        if(level < 0)
            level = 0;   

        if (channel == HBridgeChannel::A) {      
            this->a_level = level;
        }  
        else if (channel == HBridgeChannel::B) {
            this->b_level = level;
        }
        this->pending_changes = true; 
    }

    /*float get_value(HBridgeChannel channel) {
        if (channel == HBridgeChannel::A)
            return this->a_level;
        if (channel == HBridgeChannel::B)
            return this->b_level;
        if (channel == HBridgeChannel::AB)
            return (this->a_level + this->b_level) / 2;
        return 0.0;
    }*/

    void apply_duty_changes() {
        if (this->pending_changes) {  
            this->update_duty(this->a_level, this->b_level);
            this->pending_changes = false;
        } 
    }

    float calculate_ratio(float left, float right) {
        if (this->mode & ControlMode::LEVEL_AB_AND_RATIO) {
            return left;
        } else {
            if (left == 0.0 && right == 0.0) 
                return 0.0; // prevent unnecessary switching from h bridge 
            
            return left / (left + right);
        }
    }

    float calculate_intensity(float ratio, float left, float right) {
        if (this->mode & ControlMode::LEVEL_AB_AND_RATIO) {
            return right;
        } else {
            if (left == 0.0 && right == 0.0) 
                return 0.0; // prevent unnecessary switching from h bridge 

            if (left > right) {
                 return left / ratio;
            } else {
                auto inv_ratio = 1 - ratio;
                return right / inv_ratio;
            }
        }
    }

    float calibrated_intensity(float intensity) {
        if (this->mode & ControlMode::CALIBRATED) {
            if (intensity == 0.0f) {
                return intensity;
            } else {
                return this->scale_to_threshold(intensity, this->cal.intensity_threshold, 1.0);
            }
        } else {
            return intensity;
        }      
    }

    float calibrated_ratio(float ratio) {
        if (this->mode & ControlMode::CALIBRATED) {
            return this->scale_to_threshold(ratio, this->cal.left_ratio_threshold,  this->cal.right_ratio_threshold);
        } else {
            return ratio;
        }
    }

    float scale_to_threshold(float val, float floor, float ceiling) {
        auto scaled = (val * (ceiling - floor)) + floor;
        return max(min(scaled, ceiling), floor);
    }


    void update_duty(float left, float right){
        auto ratio = this->calculate_ratio(left, right);
        auto ratio_cal = this->calibrated_ratio(ratio);

        auto intensity = this->calculate_intensity(ratio, left, right);
        auto intensity_cal = this->calibrated_intensity(intensity);
        
        this->intensity_channel->set_duty(intensity_cal);
        this->ratio_channel->set_duty(ratio_cal);     
        this->intensity_channel->apply_duty();
        this->ratio_channel->apply_duty();
    }  
};