#include <String.h>

// Sensor Libraries
#include <Adafruit_MPL3115A2.h>
// #include <Adafruit_GPS.h>
// #include <SoftwareSerial.h>
#include <Adafruit_CCS811.h>
#include <SPI.h>
#include <SD.h>
#include <RH_RF69.h>
#include <ServoTimer2.h>

// Radio Pins
#define RFM69_INT 3
#define RFM69_CS 4
#define RFM69_RST 2
#define LED 13

String dataPackage;
float maxHeight = 0;

// Sensor Objects
Adafruit_MPL3115A2 typePressureSensor;
// SoftwareSerial mySerial(8, 7);
// Adafruit_GPS typeGPS(&mySerial);
// uint32_t timer = millis();
Adafruit_CCS811 typeAirSensor;
RH_RF69 typeRadio(RFM69_CS, RFM69_INT);
ServoTimer2 typeMotor;

File dataFile;

void setup()
{
    Serial.begin(9600);
    while(!Serial);

    // typeGPS.begin(9600);
    // typeGPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
    // typeGPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);

    pinMode(A0, INPUT);
    pinMode(RFM69_RST, OUTPUT);
    digitalWrite(RFM69_RST, LOW);
    digitalWrite(RFM69_RST, HIGH);
    delay(10);
    digitalWrite(RFM69_RST, LOW);
    delay(10);

    pinMode(6, OUTPUT); // Buzzer
    digitalWrite(6, LOW);

    saveData("Program Started EASC");

    checkSensors();

    typeRadio.setTxPower(20, true);
    uint8_t key[] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                   0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    typeRadio.setEncryptionKey(key);

    typeMotor.attach(9);
    typeMotor.write(2000);
}

void loop()
{
    getData();
    Serial.println(dataPackage);

    saveData(dataPackage);

    sendData();

    delay(1000);
}

void checkSensors()
{
    digitalWrite(6, HIGH);
    delay(500);
    digitalWrite(6, LOW);
    delay(500);
    digitalWrite(6, HIGH);
    delay(500);
    digitalWrite(6, LOW);
    delay(500);
    digitalWrite(6, HIGH);
    delay(500);
    digitalWrite(6, LOW);

    if(!typePressureSensor.begin())
    {
        Serial.println("Error::PressureSensor");
        saveData("Error::PressureSensor");
    }
    else
    {
        typePressureSensor.setSeaPressure(1013.26);
    }

    if(!typeAirSensor.begin())
    {
        Serial.println("Error::AirSensor");
        saveData("Error::AirSensor");
    }

   if(!SD.begin(10))
   {
       Serial.println("Error::SDcard");
       saveData("Error::SDcard");
   }
   else
   {
//       SD.remove("database.txt");
   }

   if(!typeRadio.init())
   {
       Serial.println("Error::Radio");
       saveData("Error::Radio");
   }

   if(!typeRadio.setFrequency(433.0))
   {
       Serial.println("Error::RadioFrequency");
       saveData("Error::RadioFrequency");
   }

   Serial.println("Error::GPS");
   saveData("Error::GPS");
}

void getData()
{
    dataPackage = "EASC|";

    // Pressure Sensor Data
    if(typePressureSensor.begin())
    {
        dataPackage += (String)typePressureSensor.getPressure();
        dataPackage += "|";
        dataPackage += (String)typePressureSensor.getTemperature();
        dataPackage += "|";

        float dataAltitude = typePressureSensor.getAltitude();
        dataPackage += (String)dataAltitude;
        dataPackage += "|";

        maxHeight = max(maxHeight, dataAltitude);
        if(maxHeight - dataAltitude > 50)
        {
            typeMotor.write(750);
            digitalWrite(6, HIGH);
        }
        else
        {
            digitalWrite(6, LOW);
        }
    }
    else
    {
        fillEmpty(3);
    }

    //GPS Data
    
    // typeGPS.read();
    // if(typeGPS.newNMEAreceived())
    // {
    //     if(!typeGPS.parse(typeGPS.lastNMEA()))
    //         return;
    // }

    // if(millis() - timer > 200)
    // {
    //     timer = millis();
    //     typeGPS.fix;
    //     typeGPS.fixquality;
    //     if(typeGPS.fix)
    //     {
    //         Serial.print("Lat:");
    //         Serial.println(typeGPS.latitude);
    //         dataPackage += (String)typeGPS.latitude;
    //         dataPackage += "|";
    //         dataPackage += (String)typeGPS.longitude;
    //         dataPackage += "|";
    //         dataPackage += (String)typeGPS.altitude;
    //         dataPackage += "|";
    //         dataPackage += (String)typeGPS.satellites;
    //         dataPackage += "|";
    //     }
    //     else
    //     {
    //         fillEmpty(4);
    //     }
    // }
    // else
    // {
    //    fillEmpty(4);
    // }
    fillEmpty(4);

    //UV Light Data
    dataPackage += (String)analogRead(A0);
    dataPackage += "|";

    //Air Quality Data
    if(typeAirSensor.available())
    {
        if(!typeAirSensor.readData())
        {
            dataPackage += (String)typeAirSensor.geteCO2();
            dataPackage += "|";
            dataPackage += (String)typeAirSensor.getTVOC();
            dataPackage += "|";
        }
        else
        {
            fillEmpty(2);
        }
    }
    else
    {
        fillEmpty(2);
    }

    dataPackage += ".";
}

void saveData(String DataText)
{
   dataFile = SD.open("database.txt", FILE_WRITE);
   if(dataFile)
   {
       dataFile.println(DataText);
       dataFile.close();
   }
}

void fillEmpty(int times)
{
    for (int i = 0; i < times; ++i)
    {
        dataPackage += "-|";
    }
}

void sendData()
{
    char charPackage[dataPackage.length()-2];
    for (int i = 0; i < dataPackage.length(); ++i)
    {
        charPackage[i] = dataPackage[i];
    }

    typeRadio.send((uint8_t *)charPackage, strlen(charPackage));
}
