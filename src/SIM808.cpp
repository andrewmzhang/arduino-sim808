#include "SIM808.h"

TOKEN(RDY);

SIM808::SIM808(uint8_t resetPin, uint8_t pwrKeyPin, uint8_t statusPin)
{
	_resetPin = resetPin;
	_pwrKeyPin = pwrKeyPin;
	_statusPin = statusPin;

	pinMode(_resetPin, OUTPUT);
	if(_pwrKeyPin != SIM808_UNAVAILABLE_PIN) pinMode(_pwrKeyPin, OUTPUT);
	if (_statusPin != SIM808_UNAVAILABLE_PIN) pinMode(_statusPin, INPUT);
	
	if(_pwrKeyPin != SIM808_UNAVAILABLE_PIN) digitalWrite(_pwrKeyPin, HIGH);
	digitalWrite(_resetPin, HIGH);
}

SIM808::~SIM808() { }

#pragma region Public functions

void SIM808::init()
{
	SIM808_PRINT_SIMPLE_P("Init...");

	reset();
	waitForReady();
	delay(1500);

	setEcho(SIM808Echo::Off);
}

void SIM808::reset()
{
	digitalWrite(_resetPin, HIGH);
	delay(10);
	digitalWrite(_resetPin, LOW);
	delay(200);

	digitalWrite(_resetPin, HIGH);
}

void SIM808::waitForReady()
{
	do
	{
		SIM808_PRINT_SIMPLE_P("Waiting for echo...");
		sendAT(S_F(""));
	// Despite official documentation, we can get an "AT" back without a "RDY" first.
	} while (waitResponse(TO_F(TOKEN_AT)) != 0);

	// Its unclear to me why this RDY needs to exist or when its sent. Despite official documentation, on the
	// DFRobot_SIM808, I don't consistent get this when the Serial pins for the module are default pins (0,1).
	// However if the Serial pins are changed to (2, 3), note that the board must be detached and power/serial supplied
	// via jumper lines, I do receive a RDY. I'll need to do more testing. 
	// Anyways, if you stack the module and use (0,1), default pins, as Serial RDY is not always received and you may hang
	// on this line. 
	//while (waitResponse(TO_F(TOKEN_RDY)) != 0);
}

bool SIM808::setEcho(SIM808Echo mode)
{
	sendAT(S_F("E"), (uint8_t)mode);

	return waitResponse() == 0;
}

size_t SIM808::sendCommand(const char *cmd, char *response, size_t responseSize)
{
	flushInput();
	sendAT(cmd);
	
	uint16_t timeout = SIMCOMAT_DEFAULT_TIMEOUT;
	readNext(response, responseSize, &timeout);
}

#pragma endregion


