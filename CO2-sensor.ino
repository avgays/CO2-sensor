#define MY_SENSORS

#define MY_DEBUG
#define MY_RADIO_NRF24

#include <SPI.h>

#ifdef MY_SENSORS
#include <MySensors.h>
#endif

#include <Wire.h> 

 
/************************Hardware Related Macros************************************/
#define         MG_PIN                       A0     //define which analog input channel you are going to use
//#define         BOOL_PIN                     (4)
//#define         DC_GAIN                      (8.5)   //define the DC gain of amplifier
#define         DC_GAIN                      (1.0)   //define the DC gain of amplifier

#define         T_COMP A1 
#define         CO_VCC 7 
 
/***********************Software Related Macros************************************/
#define         READ_SAMPLE_INTERVAL         (50)    //define how many samples you are going to take in normal operation
#define         READ_SAMPLE_TIMES            (5)     //define the time interval(in milisecond) between each samples in 
                                                     //normal operation
 
/**********************Application Related Macros**********************************/
//These two values differ from sensor to sensor. user should derermine this value.
#define         ZERO_POINT_VOLTAGE           (1.73) //define the output of the sensor in volts when the concentration of CO2 is 400PPM
#define         REACTION_VOLTGAE             (0.91) //define the voltage drop of the sensor when move the sensor from air into 1000ppm CO2

 float v400ppm = 1.73;   //MUST BE SET ACCORDING TO CALIBRATION
 float v10000ppm = 0.91; //MUST BE SET ACCORDING TO CALIBRATION````````````````````````
 float deltavs = v400ppm - v10000ppm;
 float A = deltavs/(log10(400) - log10(10000));
 float B = log10(400);

 
/*****************************Globals***********************************************/
float           CO2Curve[3]  =  {2.602,ZERO_POINT_VOLTAGE,(REACTION_VOLTGAE/(2.602-3))};   
                                                     //two points are taken from the curve. 
                                                     //with these two points, a line is formed which is
                                                     //"approximately equivalent" to the original curve.
                                                     //data format:{ x, y, slope}; point1: (lg400, 0.324), point2: (lg4000, 0.280) 
                                                     //slope = ( reaction voltage ) / (log400 –log1000) 
 

#define CHILD_ID 1   // Id of the sensor child
#define CHILD_ID_MQ 2   // Id of the sensor child
#define CHILD_ID_MQ2 4   // Id of the sensor child
#define CHILD_ID_TEMP 3
#ifdef MY_SENSORS
MyMessage msg(CHILD_ID, V_VOLTAGE);
MyMessage msg2(CHILD_ID_MQ, V_LEVEL);
MyMessage msg3(CHILD_ID_TEMP, V_VOLTAGE);
MyMessage msg4(CHILD_ID_MQ2, V_LEVEL);
#endif

void presentation() {
#ifdef MY_SENSORS
	sendSketchInfo("CO2", "1.5");
	present(CHILD_ID, S_MULTIMETER);
	present(CHILD_ID_MQ, S_AIR_QUALITY);
	present(CHILD_ID_MQ2, S_AIR_QUALITY);
	present(CHILD_ID_TEMP, S_MULTIMETER);
#endif
#ifdef MY_DEBUG
	Serial.print("MG-811 Demostration\n");
#endif

}

void setup()
{
	analogReference(EXTERNAL);
	pinMode(CO_VCC, OUTPUT);
	digitalWrite(CO_VCC, HIGH);

#ifdef MY_DEBUG
	Serial.begin(115200);
	Serial.println("Power sensor On");
#endif
//    pinMode(BOOL_PIN, INPUT);                        //set pin to input
//    digitalWrite(BOOL_PIN, HIGH);                    //turn on pullup resistors


               
}
 
void loop()
{
    int percentage;
	int percentage2;
    float volts;
    float myTemp;

	digitalWrite(CO_VCC, HIGH);
	delay(120000);

    myTemp=analogRead(T_COMP) * 2.5 / 1024;

#ifdef MY_DEBUG
//    Serial.print( "Tcomp:" );
//    Serial.println(myTemp); 
#endif
  
    volts = MGRead(MG_PIN);
	percentage = MGGetPercentage(volts, CO2Curve);
	percentage2 = MGGetPercentage2(volts);
#ifdef MY_DEBUG
	Serial.print( "SEN0159:" );
    Serial.print(volts); 
    Serial.print( "V           " );
    Serial.print("CO2:");
    if (percentage == -1) {
        Serial.print( "<400" );
    } else {
        Serial.print(percentage);
    }
    Serial.print( "ppm" );
	Serial.print("       ");
	Serial.print(percentage2);
	Serial.print("ppm");
    Serial.print( "       Time point:" );
    Serial.print(millis());
    Serial.print("\n");
#endif
#ifdef MY_SENSORS
     send(msg.set(volts,2));
     send(msg3.set(myTemp,2));
     send(msg2.set(percentage));
	 send(msg4.set(percentage2));
#endif
     
//    if (digitalRead(BOOL_PIN) ){
//        Serial.print( "=====BOOL is HIGH======" );
//    } else {
//        Serial.print( "=====BOOL is LOW======" );
//    }
       
    Serial.print("\n");

	//digitalWrite(CO_VCC, LOW);
    delay(120000);
}
 
 
 
/*****************************  MGRead *********************************************
Input:   mg_pin - analog channel
Output:  output of SEN-000007
Remarks: This function reads the output of SEN-000007
************************************************************************************/
float MGRead(int mg_pin)
{
    int i;
    float v=0;
 
    for (i=0;i<READ_SAMPLE_TIMES;i++) {
        v += analogRead(mg_pin);
        delay(READ_SAMPLE_INTERVAL);
    }
   // v = (v/READ_SAMPLE_TIMES) *5/1024 ;
	v = (v / READ_SAMPLE_TIMES) * 2.5 / 1024;
    return v;  
}
 
/*****************************  MQGetPercentage **********************************
Input:   volts   - SEN-000007 output measured in volts
         pcurve  - pointer to the curve of the target gas
Output:  ppm of the target gas
Remarks: By using the slope and a point of the line. The x(logarithmic value of ppm) 
         of the line could be derived if y(MG-811 output) is provided. As it is a 
         logarithmic coordinate, power of 10 is used to convert the result to non-logarithmic 
         value.
************************************************************************************/
int  MGGetPercentage(float volts, float *pcurve)
{
  
   if ((volts/DC_GAIN )>=ZERO_POINT_VOLTAGE) {
      return -1;
   } else { 
      return pow(10, ((volts/DC_GAIN)-pcurve[1])/pcurve[2]+pcurve[0]);
   }
//  float power = ((volts - v400ppm)/A) + B;
//  float co2ppm = pow(10,power);
//  return co2ppm;
  
   
}

int  MGGetPercentage2(float voltage)
{
	float power = ((voltage - v400ppm) / A) + B;
	float co2ppm = pow(10, power);
	return (int)co2ppm;
}