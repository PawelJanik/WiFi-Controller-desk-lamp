#if ARDUINO >= 100
  #include "Arduino.h"
#else
  #include "WProgram.h"
  #include "pins_arduino.h"
  #include "WConstants.h"
#endif

#include "led.h"

int LEDclass::speed;

LEDclass::LEDclass(int Pin, int minValue, int maxValue)
{
	pin = Pin;

	pinMode(pin, OUTPUT);

	onOff = false;
	value = 0;
	targetValue = 0;
	lastTargetValue = 700;

	speed = 10;

	minimumValue = minValue;
	maximumValue = maxValue;
}

void LEDclass::turnOn()
{
	setValue(lastTargetValue);
}

void LEDclass::turnOff()
{
	setValue(0);
}

void LEDclass::setValue(int Value)
{
	targetValue = Value;

	if(targetValue >= 1)
	{
		onOff = true;
		lastTargetValue = targetValue;
	}
	else
	{
		onOff = false;
	}
}

void LEDclass::set()
{
	if((targetValue > value) && (value < 1000) && onOff)
		value = value + speed;
	else if((targetValue < value) && (value > 0))
		value = value - speed;

	int v = map(value, 0, 1000, minimumValue, maximumValue);
	analogWrite(pin, pow(2.0,v/100.0));
}

void LEDclass::setNow()
{
	value = targetValue;

	analogWrite(pin, pow(2.0,value/100.0));
}
