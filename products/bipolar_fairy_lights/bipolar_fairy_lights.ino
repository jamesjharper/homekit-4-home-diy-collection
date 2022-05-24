
#include "HomeSpan.h" 
#include "fairy_lights_service.h"  

void setup() {

  Serial.begin(115200);
  
  homeSpan.begin(Category::Lighting, "Smart Fairy Lights");

  new SpanAccessory();                                  

    new Service::AccessoryInformation();                    // HAP requires every Accessory to implement an AccessoryInformation Service, which has 6 required Characteristics
      new Characteristic::Name("Smart Fairy Lights");                   
      new Characteristic::Manufacturer("James Harper");             
      new Characteristic::SerialNumber("0000001");              
      new Characteristic::Model("LED H Bridge PWM");                    
      new Characteristic::FirmwareRevision("1.0");   
      new Characteristic::Identify();                           
  
    new Service::HAPProtocolInformation();                  // Create the HAP Protcol Information Service  
      new Characteristic::Version("1.1.0");                     
  

    HBridgeFrequency freq = {
        .intensity_hz = 10000,
        .ratio_hz = 2500
    };

    HBridgeCalibration cal = {
        .intensity_threshold = 0.0f,
        .left_ratio_threshold = 0.00f,
        .right_ratio_threshold = 1.0f
    };

    HBridgeGpioPins ab_gpio = {
        .left_pin = 4, 
        .right_pin = 16,
        .intensity_pin = 17
    };

    new FairyLightsService(
        /* button_pin */ 18, 
        freq,
        cal,
        ab_gpio,
        ControlMode::LEVEL_A_AND_LEVEL_B & ControlMode::NO_CALIBRATION_REQUIRED
    );      
}

void loop(){
  homeSpan.poll(); 
}