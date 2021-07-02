#include "SIM808.h"

TOKEN_TEXT(CBC, "+CBC");
TOKEN_TEXT(CFUN, "+CFUN");

bool SIM808::powered()
{
	if(_statusPin == SIM808_UNAVAILABLE_PIN) {
		sendAT();
		return waitResponse(SIMCOMAT_DEFAULT_TIMEOUT) != -1;
	}
	
	return digitalRead(_statusPin) == HIGH;
}

bool SIM808::powerOnOff(bool power)
{
	if (_pwrKeyPin == SIM808_UNAVAILABLE_PIN) return false;

	bool currentlyPowered = powered();
	
	SIM808_PRINT_P("powerOnOff: %t -> %t \n", power, currentlyPowered);
	
	// NOTE: This is an interesting design choice... This forces the user to check before they call powerOnOff
	// if the power state is already what they want. This would mean 2 consequtive calls to powered()
	// AND the behavior is different than in the last return statment at the bottom; the user
	// cannot tell if the power failed to set and timed-out or if it was already in the correct powerstate.
	// Thus I have changed this line to return true if already in the proper powerstate
	// if (currentlyPowered == power) return false;  // ORIGINAL CODE
	if (currentlyPowered == power) return true;

	// Make sure the _pwrKeyPin is set low. Wait 250ms to ensure SIM808 registers.
	// 250ms because I assume it will work to make the chip see the LOW.
	digitalWrite(_pwrKeyPin, LOW);
	delay(250);

	// For poweron, we need to hold _pwrKeyPin HIGH for 1s
	// we hold the pin high so if the module shuts off it'll turn back on.
	// For poweroff, we need to hold _pwrKeyPin HIGH for 3s
	if (power) {
		digitalWrite(_pwrKeyPin, HIGH);
		delay(1250);
		//digitalWrite(_pwrKeyPin, LOW);
	} else {
		digitalWrite(_pwrKeyPin, HIGH);
		delay(3250);
		digitalWrite(_pwrKeyPin, LOW);
	}
	
	// Wait 2s for change. 
	int16_t timeout = 2000;  // BUG: I need to make a PR for this. Straight bug.
	do {
		delay(150);
		timeout -= 150;
		currentlyPowered = powered();
	} while(currentlyPowered != power && timeout > 0);

	return currentlyPowered == power;
}

SIM808ChargingStatus SIM808::getChargingState()
{
	uint8_t state;
	uint8_t level;
	uint16_t voltage;

	sendAT(TO_F(TOKEN_CBC));

	if (waitResponse(TO_F(TOKEN_CBC)) == 0 &&
		parseReply(',', (uint8_t)SIM808BatteryChargeField::Bcs, &state) &&
		parseReply(',', (uint8_t)SIM808BatteryChargeField::Bcl, &level) &&
		parseReply(',', (uint16_t)SIM808BatteryChargeField::Voltage, &voltage) &&
		waitResponse() == 0)
		return { (SIM808ChargingState)state, level, voltage };
			
	return { SIM808ChargingState::Error, 0, 0 };
}

SIM808PhoneFunctionality SIM808::getPhoneFunctionality()
{
	uint8_t state;

	sendAT(TO_F(TOKEN_CFUN), TO_F(TOKEN_READ));

	if (waitResponse(10000L, TO_F(TOKEN_CFUN)) == 0 &&
		parseReply(',', 0, &state) &&
		waitResponse() == 0)
		return (SIM808PhoneFunctionality)state;
	
	return SIM808PhoneFunctionality::Fail;
}

bool SIM808::setPhoneFunctionality(SIM808PhoneFunctionality fun)
{
	sendAT(TO_F(TOKEN_CFUN), TO_F(TOKEN_WRITE), (uint8_t)fun);

	return waitResponse(10000L) == 0;
}

bool SIM808::setSlowClock(SIM808SlowClock mode)
{
	sendAT(S_F("+CSCLK"), TO_F(TOKEN_WRITE), (uint8_t)mode);

	return waitResponse() == 0;
}

