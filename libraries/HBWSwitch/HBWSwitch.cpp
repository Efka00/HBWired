
#include "HBWSwitch.h"

// Switches
HBWSwitch::HBWSwitch(uint8_t _pin, hbw_config_switch* _config) {
	pin = _pin;
    config = _config;
    nextFeedbackDelay = 0;
    lastFeedbackTime = 0;
};


// channel specific settings or defaults
void HBWSwitch::afterReadConfig() {
    digitalWrite(pin, config->n_inverted ? LOW : HIGH);		// 0=inverted, 1=not inverted (device reset will set to 1!)
    pinMode(pin,OUTPUT);
}


void HBWSwitch::set(HBWDevice* device, uint8_t length, uint8_t const * const data) {
	if (config->output_unlocked) {	//0=LOCKED, 1=UNLOCKED
		if(*data > 200) {   // toggle
			digitalWrite(pin, digitalRead(pin) ? LOW : HIGH);
		}else{   // on or off
			if (*data)
				digitalWrite(pin, LOW ^ config->n_inverted);
			else
				digitalWrite(pin, HIGH ^ config->n_inverted);
		}
	}
	// Logging
	if(!nextFeedbackDelay && config->logging) {
		lastFeedbackTime = millis();
		nextFeedbackDelay = device->getLoggingTime() * 100;
	}
};


uint8_t HBWSwitch::get(uint8_t* data) {
	if (digitalRead(pin) ^ config->n_inverted)
		(*data) = 0;
	else
		(*data) = 200;
	return 1;
};


void HBWSwitch::loop(HBWDevice* device, uint8_t channel) {
	// feedback trigger set?
    if(!nextFeedbackDelay) return;
    unsigned long now = millis();
    if(now - lastFeedbackTime < nextFeedbackDelay) return;
    lastFeedbackTime = now;  // at least last time of trying
    // sendInfoMessage returns 0 on success, 1 if bus busy, 2 if failed
	// we know that the level has only 1 byte here
	uint8_t level;
    get(&level);	
    uint8_t errcode = device->sendInfoMessage(channel, 1, &level);   
    if(errcode == 1) {  // bus busy
    // try again later, but insert a small delay
    	nextFeedbackDelay = 250;
    }else{
    	nextFeedbackDelay = 0;
    }
}

