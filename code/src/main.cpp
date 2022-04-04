#include <Arduino.h>
#include <SparkFun_Alphanumeric_Display.h>

HT16K33 display;

void setup()
{
  // put your setup code here, to run once:
  SerialUSB.begin(115200);
  Wire.begin();
  delay(2000);
}

void loop()
{

  // put your main code here, to run repeatedly:
  SerialUSB.write("George is Cool\n");
  if (display.begin() == false)
  {
    Serial.println("Device did not acknowledge!");
  }
  else
  {
    Serial.println("Display acknowledged.");
  }
  display.print("Milk");

  delay(1000);
}