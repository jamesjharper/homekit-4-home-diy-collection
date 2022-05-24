
#pragma once
#include "HomeSpan.h" 
#include "extras/PwmPin.h"               

struct LedPwmController {      

    LedPin *pwm_pin; 

    // The R value in the graph equation
    float R;
    float actual_luminesce;

    int32_t brightness_step;
    int32_t target_brightness;
    int32_t current_brightness;
    boolean power_status;

    unsigned long last_tick_micros;
    unsigned long tick_duration_micros;

    LedPwmController(int ledPin, int intervals) {

        target_brightness = 0;
        current_brightness = 0;
        actual_luminesce = 0.0;
        power_status = false;
        last_tick_micros = 0;
        tick_duration_micros = 2000;

        // Use an arbitrarily large int as a set increment to avoid additional floating point math 
        brightness_step = 100000 / intervals;

        // https://diarmuid.ie/blog/pwm-exponential-led-fading-on-arduino-or-other-platforms
        // Calculate the R variable (only needs to be done once at setup)
        R = (intervals * log10(2))/(log10(intervals)) * brightness_step;
        pwm_pin = new LedPin(ledPin, 0.0 /* inital value*/, 10000 /* Khz*/);    
    } 

    int32_t get_target_brightness () { 
        if(power_status == false) {
          return 0;
        } else {
          return target_brightness;
        }
    }


    void toggle_power_status () { 
        set_power_status(!power_status);
    }

    void set_power_status (boolean new_power_status) { 

        power_status = new_power_status;
        LOG1("pin[");
        LOG1(pwm_pin->getPin());
        LOG1("].power_status = ");
        if (power_status) {
            LOG1("ON\n");
        } else {
            LOG1("OFF\n");
        }
    }

    void set_target_brightness (int32_t percentage_brightness) { 
        target_brightness = percentage_brightness * brightness_step;
        LOG1("pin[");
        LOG1(pwm_pin->getPin());
        LOG1("].target_brightness = ");
        LOG1(percentage_brightness);        
        LOG1("%\n");
    }

    void set_actual_brightness (int32_t percentage_brightness ) { 
        if (current_brightness < brightness_step) {
            pwm_pin->set(0); 
        } else {    
            float target_luminesce  = pow (2, current_brightness / R) + 1;
            
            // Anti flicker
            // prevent very small increments at lower luminosities as this will introduce 
            // a flicker when duty cycle is adjusted 
            if (abs(target_luminesce - actual_luminesce) > 0.001) {
                actual_luminesce = target_luminesce;
                pwm_pin->set(actual_luminesce); 
            }
        }
    }

    void step_next() { 

        unsigned long current_micros = micros();
        if(current_micros - last_tick_micros <= tick_duration_micros){
            return;
        }
        last_tick_micros = current_micros;


        int target_brightness = get_target_brightness();
        int delta = target_brightness - current_brightness;
        if(delta == 0) {
            return;
        }

        int increment = 0;
        if (target_brightness < current_brightness) {
          // ease off
          increment = delta / 100;
        } else {
          // ease on
          increment = delta / 2000;
        }

        if(increment == 0) {
            current_brightness = target_brightness;
        } else {
            current_brightness += increment;
        }

        set_actual_brightness(current_brightness);
    } 
};
