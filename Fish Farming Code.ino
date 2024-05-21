#define BLYNK_TEMPLATE_ID "TMPL6MA8pWjc_"
#define BLYNK_TEMPLATE_NAME "Mini Project"
#define BLYNK_AUTH_TOKEN "ghyHjoFRleRt9VPa6KX6ojFHFusNPdMD"

char auth[] = "ghyHjoFRleRt9VPa6KX6ojFHFusNPdMD";   //Authentication code sent by Blynk
char ssid[] = "Lucky OnePlus 7T";                        //WiFi SSID
char pass[] = "abcdefgh";                        //WiFi Password

//DS18B20 sensor address 0x28EB2381E3563CCC
#include <OneWire.h>
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <DallasTemperature.h>
#include <DHT.h>
#include <Servo.h>

Servo myServo;  // Create a servo object
unsigned long previousTime = 0;  // Variable to store the previous time
int movementCount = 0;  // Counter for the number of movements

#define ONE_WIRE_BUS 14
#define DHTPIN 12     // what pin we're connected to  
#define DHTTYPE DHT22   // DHT 22  (AM2302)
#define TdsSensorPin A0

#define VREF 3.3      // analog reference voltage(Volt) of the ADC
#define SCOUNT  30           // sum of sample point

DHT dht(DHTPIN, DHTTYPE); //// Initialize DHT sensor for normal 16MHz Arduino
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
DeviceAddress insideThermometer;

int chk;
float tempC, tempF;
float hum;  //Stores humidity value
float temp; //Stores temperature value

int analogBuffer[SCOUNT];    // store the analog value in the array, read from ADC
int analogBufferTemp[SCOUNT];
int analogBufferIndex = 0,copyIndex = 0;
float averageVoltage = 0,tdsValue = 0,salinity=0,temperature = 25;

void setup(void)
{
  Serial.begin(9600);
  myServo.attach(13);  // Attach the servo to pin 9

  Serial.println("Dallas Temperature IC Control Library Demo");
  Serial.print("Locating devices...");
  sensors.begin();
  Serial.print("Found ");
  Serial.print(sensors.getDeviceCount(), DEC);
  Serial.println(" devices.");

  Serial.print("Parasite power is: "); 
  if (sensors.isParasitePowerMode()) Serial.println("ON");
  else Serial.println("OFF");
  if (!sensors.getAddress(insideThermometer, 0)) Serial.println("Unable to find address for Device 0"); 
  
  Serial.print("Device 0 Address: ");
  printAddress(insideThermometer);
  Serial.println();
  sensors.setResolution(insideThermometer, 11);
 
  Serial.print("Device 0 Resolution: ");
  Serial.print(sensors.getResolution(insideThermometer), DEC); 
  Serial.println();

  WiFi.begin(ssid, pass);
  while(WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.println("Connecting to WiFi....");
  }
  Blynk.begin(auth, ssid, pass);
  
  pinMode(TdsSensorPin,INPUT);
  dht.begin();
}

void printAddress(DeviceAddress deviceAddress)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}
void printTemperature(DeviceAddress deviceAddress)
{
  tempC = sensors.getTempC(deviceAddress);
  if(tempC == DEVICE_DISCONNECTED_C) 
  {
    Serial.println("Error: Could not read temperature data");
    return;
  }
  Serial.print("Water Temp C: ");
  Serial.println(tempC);
  Serial.print("Water Temp F: ");
  tempF = DallasTemperature::toFahrenheit(tempC);
  Serial.println(tempF); // Converts tempC to Fahrenheit
  delay(1000);
}
int getMedianNum(int bArray[], int iFilterLen) 
{
      int bTab[iFilterLen];
      for (byte i = 0; i<iFilterLen; i++)
      bTab[i] = bArray[i];
      int i, j, bTemp;
      for (j = 0; j < iFilterLen - 1; j++) 
      {
      for (i = 0; i < iFilterLen - j - 1; i++) 
          {
        if (bTab[i] > bTab[i + 1]) 
            {
        bTemp = bTab[i];
            bTab[i] = bTab[i + 1];
        bTab[i + 1] = bTemp;
         }
      }
      }
      if ((iFilterLen & 1) > 0)
    bTemp = bTab[(iFilterLen - 1) / 2];
      else
    bTemp = (bTab[iFilterLen / 2] + bTab[iFilterLen / 2 - 1]) / 2;
      return bTemp;
}

void loop(void)
{
   Serial.print("Requesting temperatures...");
   sensors.requestTemperatures(); // Send the command to get temperatures
   Serial.println("DONE");
   printTemperature(insideThermometer);
   delay(1000);

   hum = dht.readHumidity();
   temp= dht.readTemperature();
   Serial.print("Air Humidity: ");
   Serial.println(hum);
   Serial.print("Air Temp C: ");
   Serial.println(temp);
   delay(1000);

   static unsigned long analogSampleTimepoint = millis();
   if(millis()-analogSampleTimepoint > 40U)     //every 40 milliseconds,read the analog value from the ADC
   {
     analogSampleTimepoint = millis();
     analogBuffer[analogBufferIndex] = analogRead(TdsSensorPin);    //read the analog value and store into the buffer
     analogBufferIndex++;
     if(analogBufferIndex == SCOUNT) 
         analogBufferIndex = 0;
   }   
   static unsigned long printTimepoint = millis();
   if(millis()-printTimepoint > 800U)
   {
      printTimepoint = millis();
      for(copyIndex=0;copyIndex<SCOUNT;copyIndex++)
        analogBufferTemp[copyIndex]= analogBuffer[copyIndex];
      averageVoltage = getMedianNum(analogBufferTemp,SCOUNT) * (float)VREF / 1024.0; // read the analog value more stable by the median filtering algorithm, and convert to voltage value
      float compensationCoefficient=1.0+0.02*(temperature-25.0);    //temperature compensation formula: fFinalResult(25^C) = fFinalResult(current)/(1.0+0.02*(fTP-25.0));
      float compensationVolatge=averageVoltage/compensationCoefficient;  //temperature compensation
      tdsValue=(133.42*compensationVolatge*compensationVolatge*compensationVolatge - 255.86*compensationVolatge*compensationVolatge + 857.39*compensationVolatge)*0.5; //convert voltage value to tds value
      //Serial.print("voltage:");
      //Serial.print(averageVoltage,2);
      //Serial.print("V   ");
   }
   Serial.print("Water TDS Value:");
   Serial.print(tdsValue,0);
   Serial.println("ppm");
   salinity=tdsValue;  //assuming that the water is clean and pure
   Serial.print("Clean Water Salinity:");
   Serial.print(salinity,0);
   Serial.println("ppt");

   unsigned long currentTime = millis();  // Get the current time
  // Check if 30 seconds have passed since the last movement
  if (currentTime - previousTime >= 3000) {
    // Reset the previous time
    previousTime = currentTime;
    // Move the servo 3 times
    for (int i = 0; i < 3; i++) {
      myServo.write(0);  // Move the servo to position 0 (full speed clockwise)
      delay(1000);       // Wait for 1 second
      myServo.write(180); // Move the servo to position 180 (full speed counterclockwise)
      delay(1000);       // Wait for 1 second
    }
    movementCount++; // Increment the movement count
    // Check if 3 movements have been completed
    if (movementCount >= 3) {
      // Reset the movement count
      movementCount = 0;
      // Add extra delay if needed before starting the next cycle
      delay(1000); // Adjust this delay as needed
    }
  }

   Blynk.virtualWrite(V0, tempC);
   Blynk.virtualWrite(V1, temp);
   Blynk.virtualWrite(V2, hum);
   Blynk.virtualWrite(V3, tdsValue);
   Blynk.virtualWrite(V4, tempF);
   delay(1000);
}