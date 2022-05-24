#include "fairy_lights_service.h"
#include "hbridge.h"


// DimSmartLed
// Public: 

FairyLightsService::FairyLightsService(        
        uint8_t button_pin, 
        HBridgeFrequency freq,
        HBridgeCalibration cal,
        HBridgeGpioPins ab_gpio_pins, 
        ControlMode mode
    ) {

    //this->output_devices.push_back(new LedPwmHBridgeController(freq, cal, ab_gpio_pins, mode));

    this->output_devices.push_back(
        new PwmDeviceController<LedPwmHBridge>( 
            new LedPwmHBridge(
                freq,
                cal,
                ab_gpio_pins,
                mode
            )
        )
    );


    auto brightness_a = new FairyLightChannelBrightness("A", this, {HBridgeChannelId::ID_A1()});
    auto brightness_b = new FairyLightChannelBrightness("B", this, {HBridgeChannelId::ID_B1()});
    auto power = new FairyLightPowerStatus("Power", this, { HBridgeChannelId::ID_AB1() }, button_pin);

    power->addLink(brightness_a);
    power->addLink(brightness_b);
} 


// DimSmartFairyLights
// Private: 
 
boolean FairyLightsService::on_power_status_changed(std::vector<HBridgeChannelId> &controlled_channels, boolean val) {    

    for (auto target : controlled_channels) {

        try {
             LOG1("on_power_status_changed");
            this->output_devices.at(target.device_id)->channel(target.channel)->set_power_status(val);
        } catch (std::out_of_range const& exc) {
            LOG1("Error:");
            LOG1(exc.what());
            LOG1("\n");
        }
    }
    return true;            
}

boolean FairyLightsService::on_brightness_changed(std::vector<HBridgeChannelId> &controlled_channels, int32_t val) {
    for (auto target : controlled_channels) {
         try {
            LOG1("on_brightness_changed ");
            LOG1(target.device_id);
            LOG1("\n");
            this->output_devices.at(target.device_id)->channel(target.channel)->set_target_brightness(val);
        } catch (std::out_of_range const& exc) {
            LOG1("Error:");
            LOG1(exc.what());
            LOG1("\n");
        }

         
    }
    
    return true;            
}


void FairyLightsService::on_next_loop_cycle() { 
    for (auto target_device : this->output_devices) {
        target_device->step_next();
    }
}


// FairyLightRatio
FairyLightChannelBrightness::FairyLightChannelBrightness(
    const char* name, 
    FairyLightsService *parent, 
    std::vector<HBridgeChannelId> controlled_channels
) : 
        parent(parent),
        controlled_channels(controlled_channels), 
        Service::LightBulb() 
    {

    new Characteristic::Name(
        /* Default Device name showing in "home app" */ name
    );
    new Characteristic::On(
        /* inital value */ 0, 
        /* persist value */ true
    );
    this->state = new Characteristic::Brightness(
        /* inital value */ 100, 
        /* persist value */ true
    );  
    this->state->setRange(
        /* min */ 0, 
        /* max */ 100, 
        /* step size */ 1
    );
    this->parent->on_brightness_changed(this->controlled_channels, this->get_value());
} 

boolean FairyLightChannelBrightness::update() { 
    return this->parent->on_brightness_changed(this->controlled_channels, this->get_new_value());
}

// FairyLightPowerStatus

FairyLightPowerStatus::FairyLightPowerStatus(
    const char* name, 
    FairyLightsService *parent, 
    std::vector<HBridgeChannelId> controlled_channels,
    int button_pin
    ) : 
        parent(parent),
        controlled_channels(controlled_channels), 
        button_pin(button_pin),
        Service::LightBulb() 
    {
 
    new Characteristic::Name(name);
    this->state = new Characteristic::On(0,true);

    new SpanButton(this->button_pin); 
    this->parent->on_power_status_changed(this->controlled_channels, this->get_value());   
} 

boolean FairyLightPowerStatus::update() {    
   return this->parent->on_power_status_changed(this->controlled_channels, this->get_new_value());          
}

void FairyLightPowerStatus::button(int pin, int press_type) {
    LOG1("pin[");
    LOG1(pin);
    LOG1("].button_press_status = ");

    if (press_type == SpanButton::SINGLE) {
        LOG1("SINGLE_RELEASED\n"); 
        //this->parent->on_button_press(ButtonState::SINGLE_RELEASED);

    } else if (press_type == SpanButton::DOUBLE) {
        LOG1("DOUBLE_RELEASED\n"); 
       // this->parent->on_button_press(ButtonState::DOUBLE_RELEASED);

    } else if (press_type == SpanButton::LONG) {
        LOG1("LONG_DOWN\n"); 
    }
    
} 

void FairyLightPowerStatus::loop() {    
    this->parent->on_next_loop_cycle();
}
