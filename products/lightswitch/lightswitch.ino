
#include "HomeSpan.h" 
#include "DimSmartLed.h"    


void setup() {

  Serial.begin(115200);
  
  homeSpan.begin(Category::Lighting, "Jadescape Lighting");

  new SpanAccessory();                                  

    new Service::AccessoryInformation();                    // HAP requires every Accessory to implement an AccessoryInformation Service, which has 6 required Characteristics
      new Characteristic::Name("Smart Lighting");                   
      new Characteristic::Manufacturer("James Harper");             
      new Characteristic::SerialNumber("0000001");              
      new Characteristic::Model("LED PWM");                    
      new Characteristic::FirmwareRevision("1.0");   
      new Characteristic::Identify();                           
  
    new Service::HAPProtocolInformation();                  // Create the HAP Protcol Information Service  
      new Characteristic::Version("1.1.0");                     
  
    new DimSmartLed(2,5);      
}

//////////////////////////////////////



//////////////////////////////////////

void loop(){
  
  homeSpan.poll();
  
} // end of loop()
