#pragma once
#if ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#include <driver/ledc.h>

// This code borrows inspiration from 
// https://github.com/HomeSpan/HomeSpan/blob/master/src/extras/PwmPin.h

class ChannelTimerConfig {

    static ledc_timer_config_t *timer_list[LEDC_TIMER_MAX][LEDC_SPEED_MODE_MAX];

 public:
    ledc_timer_config_t *timer_config = NULL;
    ledc_timer_t id = (ledc_timer_t)0;

    ChannelTimerConfig(uint16_t freq) {
        for(int nMode=0;nMode<LEDC_SPEED_MODE_MAX;nMode++){
            for(int nTimer=0;nTimer<LEDC_TIMER_MAX;nTimer++){

                // 1: If slot is already taken, check to see if it
                // is what we are looking for, otherwise we move on to
                // the next aviable slot
                if (timer_list[nTimer][nMode]) {

                    if (timer_list[nTimer][nMode]->freq_hz == freq) {
                        // reuse this timer
                        this->timer_config = timer_list[nTimer][nMode];
                        this->id = (ledc_timer_t)nTimer;
                        return;

                    } else {
                        // Slot is already taken
                        continue;
                    }
              
                }
                   
                // 2: Create new timer
                timer_list[nTimer][nMode] = new ledc_timer_config_t;
                timer_list[nTimer][nMode]->speed_mode=(ledc_mode_t)nMode;
                timer_list[nTimer][nMode]->timer_num=(ledc_timer_t)nTimer;
                timer_list[nTimer][nMode]->freq_hz=freq;

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 0, 0)
                timer_list[nTimer][nMode]->clk_cfg=LEDC_USE_APB_CLK;
#endif

                // find the maximum possible resolution
                int res=LEDC_TIMER_BIT_MAX-1;                               
                while(getApbFrequency() / (freq*pow(2,res)) < 1) {
                    res--;
                }
                    
                timer_list[nTimer][nMode]->duty_resolution=(ledc_timer_bit_t)res;

                // attempt to configure timer

                Serial.printf("Creating timer: Id=%d, Frequency=%d Hz, Resolution=%d bits, Speed Mode=%d \n", nTimer, freq, res, nMode);

                if(ledc_timer_config(timer_list[nTimer][nMode]) != 0){
                    Serial.printf("\n*** ERROR:  Frequency=%d Hz is out of allowed range ---",freq);
                    delete timer_list[nTimer][nMode];
                    timer_list[nTimer][nMode] = NULL;
                    return;              
                }
                
                // use this timer
                this->timer_config = timer_list[nTimer][nMode];
                this->id = (ledc_timer_t)nTimer;
                return;    
            }
        }
    }
};

class PwmChannel {

    static ledc_channel_config_t *channel_list[LEDC_CHANNEL_MAX][LEDC_SPEED_MODE_MAX];

    ledc_channel_config_t *channel=NULL;
    uint32_t duty_resolution;

    
 public:
    PwmChannel(ChannelTimerConfig* timer) {
        
        for(int nMode = 0; nMode < LEDC_SPEED_MODE_MAX; nMode++){
            for(int nChannel = 0; nChannel < LEDC_CHANNEL_MAX; nChannel++ ){

                // 1: If slot is already taken, check to see if it
                // is what we are looking for, otherwise we move on to
                // the next aviable slot
                if (channel_list[nChannel][nMode]) {
                    // Slot is already taken
                    continue;
                }


                // create new channel 
                channel_list[nChannel][nMode] = new ledc_channel_config_t;         
                channel_list[nChannel][nMode]->speed_mode = (ledc_mode_t)timer->timer_config->speed_mode;
                channel_list[nChannel][nMode]->channel = (ledc_channel_t)nChannel;
                channel_list[nChannel][nMode]->timer_sel = timer->id;
                channel_list[nChannel][nMode]->intr_type = LEDC_INTR_DISABLE;
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 4, 0)
                channel_list[nChannel][nMode]->flags.output_invert = 0;
#endif
                channel_list[nChannel][nMode]->hpoint = 0;
                // This is only set when calling assign_pin(...)
                // channel_list[nChannel][nMode]->gpio_num=pin;
                this->channel = channel_list[nChannel][nMode];
                this->duty_resolution = (uint32_t)(pow(2, (int)timer->timer_config->duty_resolution) - 1);

                Serial.printf("Creating channel: Id=%d, Timer id=%d, Max Value=%d\n", nChannel, timer->id, this->duty_resolution);
                return;    
            }
        }
    }

    void assign_pin(uint8_t gpio_pin) {
        ledc_channel_config_t channel_config =
            {
                .gpio_num   = gpio_pin, 
                .speed_mode = this->channel->speed_mode,
                .channel    = this->channel->channel,
                .intr_type  = this->channel->intr_type,
                .timer_sel  = this->channel->timer_sel,
                .duty       = this->channel->duty,
                .hpoint     = this->channel->hpoint,
                .flags {
                    .output_invert = 0
                }
            };

        ledc_channel_config(&channel_config);
    }

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 4, 0)
    void assign_pin_inverted(uint8_t gpio_pin) {
        ledc_channel_config_t channel_config =
            {
                .gpio_num   = gpio_pin, 
                .speed_mode = this->channel->speed_mode,
                .channel    = this->channel->channel,
                .intr_type  = this->channel->intr_type,
                .timer_sel  = this->channel->timer_sel,
                .duty       = this->channel->duty,
                .hpoint     = this->channel->hpoint,
                .flags {
                    .output_invert = 1
                }
            };
        ledc_channel_config(&channel_config);

        
    }
#endif

    void set_duty(float duty) {
        if(duty > 1)
            duty = 1;

        if(duty < 0)
            duty = 0;

        uint32_t duty_value = duty * this->duty_resolution ;
        ledc_set_duty(
            this->channel->speed_mode, 
            this->channel->channel, 
            duty_value
        );
    }

    void apply_duty(){
        ledc_update_duty(this->channel->speed_mode, this->channel->channel);
    }  
};
