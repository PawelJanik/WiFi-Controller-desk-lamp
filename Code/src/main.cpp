#include "config.h"

#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#include <Adafruit_NeoPixel.h>

#include "setup.h"
#include "led.h"

WiFiClient espClient;
PubSubClient client(espClient);

LEDclass led(4,90,1000);

const int pinWallSwitch = 5;
bool switchState;

bool lightState;

unsigned long ledTimer = 0;
unsigned long timer200ms = 0;
unsigned long timer10s = 0;
unsigned long timer1m = 0;

#define pixelCount 5

Adafruit_NeoPixel strip = Adafruit_NeoPixel(pixelCount, 14, NEO_GRB + NEO_KHZ800);
uint8_t lastBrightness;
uint32_t lastColor[pixelCount];

uint32_t bytesToInt(unsigned char* b, unsigned length)
{
	int val = 0;
	int j = 0;
	for (int i = length-1; i >= 0; --i)
	{
		val += (b[i] - '0') * pow(10,j);
		j++;
	}
	return val;
}

void reconnect()
{
	digitalWrite(1, LOW);

	//if (client.connect(controllerName, mqttLogin, mqttPasswd))
	if (client.connect(controllerName))
	{
		Serial.println("connected");

		client.subscribe("home/myRoom/light/2/0");
		client.subscribe("home/myRoom/light/2/0/level");

		client.subscribe("home/myRoom/light/2/1");
		client.subscribe("home/myRoom/light/2/1/color");
		client.subscribe("home/myRoom/light/2/1/brightness");

		client.subscribe("home/myRoom/light/0");
		client.subscribe("home/myRoom/light/0/level");

		client.subscribe("home/myRoom/light/2/0/speed");
		client.subscribe("home/myRoom/light/0/speed");

		client.subscribe("home/controllers/2/restart");

		digitalWrite(1, HIGH);
	}
}

void callback(char * topic, byte* payload, unsigned int length)
{
	if((strcmp(topic,"home/myRoom/light/2/0")==0) ||
		(strcmp(topic,"home/myRoom/light/0")==0))
	{
		if ((char)payload[0] == '1')
		{
			led.turnOn();
			lightState = true;
		}
		else
		{
			led.turnOff();
			lightState = false;
		}
	}

	if((strcmp(topic,"home/myRoom/light/2/0/level")==0) ||
		(strcmp(topic,"home/myRoom/light/0/level")==0))
	{
		int value = bytesToInt(payload,length);
		Serial.println(value);
		led.setValue(value);

		if(value > 0)
			lightState = true;
		else
			lightState = false;
	}

	if((strcmp(topic,"home/myRoom/light/2/speed")==0) ||
		(strcmp(topic,"home/myRoom/light/0/speed")==0))
	{
		LEDclass::speed = bytesToInt(payload,length);
	}

	if(strcmp(topic,"home/myRoom/light/2/1")==0)
	{
		if ((char)payload[0] == '1')
		{
			strip.setBrightness(lastBrightness);
			for(int i = 0; i < pixelCount; i++)
			{
				strip.setPixelColor(i, lastColor[i]);
			}
		}
		else
		{
			for(int i = 0; i < pixelCount; i++)
			{
				lastColor[i] = strip.getPixelColor(i);
			}
			lastBrightness = strip.getBrightness();
			strip.setBrightness(0);
		}

		strip.show();
	}

	if(strcmp(topic,"home/myRoom/light/2/1/color")==0)
	{
		uint32_t color = bytesToInt(payload,length);

		for(int i = 0; i < pixelCount; i++)
		{
			strip.setPixelColor(i, color);
		}

		strip.show();
	}

	if(strcmp(topic,"home/myRoom/light/2/1/brightness")==0)
	{
		uint8_t brightness = bytesToInt(payload,length);

		strip.setBrightness(brightness);

		strip.show();
	}

	if(strcmp(topic,"home/controllers/2/restart")==0)
	{
		if((char)payload[0] == 'r')
		{
			client.publish("home/controllers/2/condition", "reset");
			delay(500);

			digitalWrite(1, LOW);
			digitalWrite(3, LOW);
			ESP.restart();
		}
	}
}

void setup()
{
	WIFIsetup();

	Serial.println(WiFi.localIP());

	OTAsetup();

	client.setServer(mqttIP, mqttPort);
	client.setCallback(callback);
	reconnect();

	pinMode(5, INPUT_PULLUP);
	switchState = (digitalRead(5));
	lightState = false;

	strip.begin();
        strip.show();

	//LEDclass::speed = 1;
}

void loop()
{
	if (!client.connected())
	{
		reconnect();

		if(digitalRead(D5) != switchState)
		{
			switchState = digitalRead(D5);


			if(lightState == false)
			{
				led.turnOn();

				lightState = true;
			}
			else
			{
				led.turnOff();

				lightState = false;
			}

			led.setNow();
		}
	}
	else
	{
		client.loop();
	}

	ArduinoOTA.handle();

	if((millis() - ledTimer) > 10)
	{
		ledTimer = millis();

		led.set();
	}

	if((millis() - timer200ms) > 200)
	{
		timer200ms = millis();
		client.publish("home/sensors/light/0", String(analogRead(A0)).c_str());
	}

	if((millis() - timer10s) > 10000)
	{
		timer10s = millis();
		client.publish("home/controllers/2/condition", "ok");
	}

	if(digitalRead(5) != switchState)
	{
		switchState = digitalRead(5);


		if(lightState == false)
		{
			client.publish("home/sensors/switch/1", "1");
		}
		else
		{
			client.publish("home/sensors/switch/1", "0");
		}
	}
}
