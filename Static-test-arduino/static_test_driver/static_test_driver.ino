#include <SPI.h>
//#include <SD.h>
#include <Wire.h>
//#include "RTClib.h"
#include <Adafruit_MMA8451.h>
#include <Adafruit_Sensor.h>
#include <Telemetry.h>

//File dataFile;                                      // tempFile,accFile; 
//RTC_DS1307 RTC;
Adafruit_MMA8451 mma = Adafruit_MMA8451();
const int chipSelect=10;                            // Use digital pin 10 as the slave select pin (SPI bus).
float temp,x,y,z;
bool temp_status = false;

#define temp_pin A2

// This is the information on the sensor being used. 
// See the www.vernier.com/products/sensors.

#define HIGH_RANGE

#ifdef HIGH_RANGE
float intercept = -250;
float slope = 250;
#else
float intercept = -1000;
float slope = 1000;
#endif
int TimeBetweenReadings = 500; // in ms
int ReadingNumber=0;

#define NUM_HIST_VALS 10

float prev_vals[NUM_HIST_VALS];
int val_num = 0;
bool zero_ready = false;

char data[10] = "";
char data_name[20] = "";

// Logging
//char filename[] = "DATA000.csv";

//I commented the setup and loop functions so that I could made separate ones without all the hardware implementations
//They will be uncommented and tested after making sure the state machine is working correctly

/*void setup() {
  Serial.begin(230400);
  Serial.println("Initializing...");
  //------------- set up temp sensor-----------


  //-------------set up accelerometer---------------
  if (!mma.begin()) {
    Serial.println("Acc error");
    while(1){}
  }
  
  mma.setRange(MMA8451_RANGE_2_G);  // set acc range (2 5 8)
  Serial.print("Acc range "); Serial.print(2 << mma.getRange()); Serial.println("G");
  
  Wire.begin();
}*/

void setup()
{
  Serial.begin(230400);
  Serial.println("Initializing...");
  pinMode(temp_pin, INPUT);
  temp = analogRead(temp_pin);
  if (temp == -102 && temp_status) {
    Serial.println("Temp sensor err");
    temp_status = false;
  }
  else if (temp != -102 && !temp_status) {
    Serial.println("Temp sensor connected");
    temp_status = true;
  }
}

unsigned long loops = 0;
void loop() {
  
  float count = analogRead(A0);
  float voltage = count / 1023 * 5.0;// convert from count to raw voltage
  float force_reading = intercept + voltage * slope; //converts voltage to sensor reading

  prev_vals[val_num] = force_reading;
  val_num++;
  if (val_num >= NUM_HIST_VALS) {
    val_num = 0;
    zero_ready = true;
  }


  // --------------Grab Tempdata------------------------ 

  // Temp sensors are slow, so alternate taking data from each sensor each loop
  // Why "byIndex"? You can have more than one IC on the same bus. 
  temp = (analogRead(temp_pin)-512)/5;
  /*if (temp == -102 && temp_status) {
    Serial.println("Temp sensor err");
    temp_status = false;
  }
  else if (temp != -102 && !temp_status) {
    Serial.println("Temp sensor connected");
    temp_status = true;
  }*/
  // Send the command to get temperatures for the next loop
  //sensors.requestTemperatures();
  
  // ---Get a new sensor event 
  //sensors_event_t event;
  //mma.getEvent(&event);

  //---------- Display the results (acceleration is measured in m/s^2)
  
  //x=event.acceleration.x;  y=event.acceleration.y;  z=event.acceleration.z;
  //storeData(force_reading, tempc, x, y, z);

  // Run autonoumous control
  // Get a throttle setting, throttle the engine
  
  run_control();

  BEGIN_SEND
  SEND_ITEM(force, force_reading)
  //SEND_ITEM(acceleration, x)
  //SEND_GROUP_ITEM(y)
  //SEND_GROUP_ITEM(z)
  SEND_ITEM(outlet_temperature, temp)
  END_SEND

  BEGIN_READ
  READ_FLAG(zero) {
    if (zero_ready) {
      Serial.println("Zeroing");
      float zero_val = 0;
      for (int i = 0; i < NUM_HIST_VALS; i++)
        zero_val += prev_vals[i];
      zero_val /= NUM_HIST_VALS;
      intercept -= force_reading;
    }
    else {
      Serial.println("Zero err");
    }
  }
  READ_FLAG(start) {
    start_countdown();
  }
  READ_FLAG(stop) {
    abort_autosequence();
  }
  READ_DEFAULT(data_name, data) {
    Serial.print("Invalid: ");
    Serial.print(data_name);
    Serial.print(":");
    Serial.println(data);
  }
  END_READ

//  delay(10);
}
