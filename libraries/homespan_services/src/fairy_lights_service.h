#pragma once
#include "HomeSpan.h"
#include "channel_id.h"
#include "hbridge_controller.h"
#include "pwm_device_controller.h"

#include <stdint.h>
#include <string>
#include <vector>
#include <memory>

// Forward Declrations 
struct FairyLightsService;
struct FairyLightChannelBrightness;
struct FairyLightPowerStatus;



struct FairyLightChannelBrightness : Service::LightBulb {      

    FairyLightsService *parent;   
    SpanCharacteristic *state;
    std::vector<HBridgeChannelId> controlled_channels;

    FairyLightChannelBrightness(
        const char* name, 
        FairyLightsService *pParent, 
        std::vector<HBridgeChannelId> controlled_channels
    );

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

struct FairyLightPowerStatus : Service::LightBulb {      

    FairyLightsService *parent;   
    SpanCharacteristic *state;
    std::vector<HBridgeChannelId> controlled_channels;
    int button_pin;

    FairyLightPowerStatus(
        const char* name,
        FairyLightsService *pParent,
        std::vector<HBridgeChannelId> controlled_channels,
        int button_pin
    );

    boolean update();
    void button(int pin, int press_type);
    void loop();


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


class FairyLightsService {
private:

    std::vector<PwmDeviceController<LedPwmHBridge>*> output_devices;
public:
    FairyLightsService(        
        uint8_t button_pin, 
        HBridgeFrequency freq,
        HBridgeCalibration cal,
        HBridgeGpioPins ab_gpio_pins, 
        ControlMode mode
    );

    ~FairyLightsService() {
        std::for_each( output_devices.begin(), output_devices.end(), []( PwmDeviceController<LedPwmHBridge>* p ) { delete p; } );
    }
    
    void toggle_power_status();
    void toggle_power_on();
    void toggle_power_off();
    void toggle_double_press_brightness();
    void cycle_brightness_next_step();

    boolean on_power_status_changed(std::vector<HBridgeChannelId> &controlled_channels, boolean val);
    boolean on_brightness_changed(std::vector<HBridgeChannelId> &controlled_channels, int32_t val);

    void on_next_loop_cycle();

};


  