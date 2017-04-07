#define MY_SENSORS
//#define MY_DEBUG
#define MY_RADIO_NRF24
#define MY_OTA_FIRMWARE_FEATURE

#include <SPI.h>
#include <Wire.h> 
#include <math.h>

#ifdef MY_SENSORS
#include <MySensors.h>
#endif
 
/************************Hardware Related Macros************************************/
#define         MG_PIN                       A0     //define which analog input channel you are going to use
//#define         BOOL_PIN                     (4)
//#define         DC_GAIN                      (8.5)   //define the DC gain of amplifier
#define         T_COMP A1 
#define         CO_VCC 7 

/***********************Software Related Macros************************************/
#define         READ_SAMPLE_INTERVAL         (50)    //define how many samples you are going to take in normal operation
#define         READ_SAMPLE_TIMES            (5)     //define the time interval(in milisecond) between each samples in 

 float v400ppm;   //MUST BE SET ACCORDING TO CALIBRATION
 float v40000ppm; //MUST BE SET ACCORDING TO CALIBRATION
 float deltavs;
 float A;
 float B = log10(400);
 bool sendVolts = true;

#define CHILD_ID_DIAGNOSTIC 254
#define CHILD_ID 1   // Id of the sensor child
#define CHILD_ID_MQ 2   // Id of the sensor child
#define CHILD_ID_VCC 4   // Id of the sensor on/of switch
//#define CHILD_ID_TEMP 3


#ifdef MY_SENSORS
MyMessage msg(CHILD_ID, V_VOLTAGE);
MyMessage msg2(CHILD_ID_MQ, V_LEVEL);
//MyMessage msg3(CHILD_ID_TEMP, V_VOLTAGE);
MyMessage msg4(CHILD_ID_VCC, V_STATUS);
MyMessage msgDiagnostic(CHILD_ID_DIAGNOSTIC, V_CUSTOM);
#endif

unsigned long SLEEP_TIME;

void presentation() {
#ifdef MY_SENSORS
	String myVals;
	v400ppm = (float)((int)loadState(2)*256 + loadState(1)) / 1000;
	v40000ppm = (float)((int)loadState(4) * 256 + loadState(3)) / 1000;
	sendVolts = (bool)loadState(8);
	

	deltavs = v400ppm - v40000ppm;
	A = deltavs / (log10(400) - log10(40000));
	SLEEP_TIME = (unsigned long)(loadState(7)) * 10000;
	myVals = "0:" + String(v400ppm,3) + ":" + String(v40000ppm,3) + ":0:" + SLEEP_TIME + ":" + sendVolts;
	sendSketchInfo("CO2 sensor", "1.5");
	present(CHILD_ID, S_MULTIMETER, "Volts");
	present(CHILD_ID_MQ, S_AIR_QUALITY,"CO2 PPM");
	//present(CHILD_ID_TEMP, S_MULTIMETER, "Temp-comp");
	present(CHILD_ID_VCC, S_BINARY, "Heater on/off");
	
	present(CHILD_ID_DIAGNOSTIC, S_CUSTOM, myVals.c_str());
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
//	Serial.begin(115200);
//	Serial.println("Power sensor On");
	Serial.println("----------------------------");
	Serial.print("v400ppm ");
	Serial.print(v400ppm);
	Serial.print("V          ");
	Serial.print("v10000ppm ");
	Serial.print(v40000ppm);
	Serial.println("V");
	Serial.print("SLEEP_TIME: ");
	Serial.println(SLEEP_TIME);
	Serial.print("SendVolts: ");
	Serial.println(sendVolts);

	Serial.println("----------------------------");
#endif
//    pinMode(BOOL_PIN, INPUT);                        //set pin to input
//    digitalWrite(BOOL_PIN, HIGH);                    //turn on pullup resistors
}
 
void loop()
{
    long percentage;
    float volts;
    float myTemp;

	digitalWrite(CO_VCC, HIGH);
    //myTemp=analogRead(T_COMP) * 2.5 / 1024;

#ifdef MY_DEB        UG
//    Serial.print( "Tcomp:" );
//    Serial.println(myTemp); 
#endif
    volts = MGRead(MG_PIN);
	percentage = MGGetPercentage(volts);
#ifdef MY_DEBUG
	Serial.print("Volts: " );
    Serial.print(volts); 
    Serial.print( "V           " );
    Serial.print("CO2:");
	Serial.print(percentage);
    Serial.print( "ppm" );
    Serial.print( "       Time point:" );
    Serial.print(millis());
    Serial.print("\n");
#endif
#ifdef MY_SENSORS
	if (sendVolts) {
		send(msg.set(volts, 3));
	}
	send(msg2.set(percentage));
     //send(msg3.set(myTemp,2));


#endif
     
//    if (digitalRead(BOOL_PIN) ){
//        Serial.print( "=====BOOL is HIGH======" );
//    } else {
//        Serial.print( "=====BOOL is LOW======" );
//    }
       
	//digitalWrite(CO_VCC, LOW);
	wait(SLEEP_TIME);
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
	v = (v / READ_SAMPLE_TIMES) * 2.5 / 1024;
    return v;  
}
 

long MGGetPercentage(float voltage) {
	float power = ((voltage - v400ppm) / A) + B;
	float co2ppm = pow(10, power);
	return (long)co2ppm;
}

void receive(const MyMessage &message) {
	int myCor = message.getInt();

	switch (message.type) {
	case V_VAR1: //v400ppm in mV, eg 1547 !
		v400ppm = myCor/1000;
		saveState(1, (byte)(myCor & 0xFF));
		saveState(2, (byte)((myCor >> 8) & 0xFF));
		send(msgDiagnostic.set(myCor));
		#ifdef MY_DEBUG
		Serial.print("v400ppm: ");
		Serial.println(myCor);
		#endif
		break;

	case V_VAR2: //v10000ppm in mV, eg 940 !
		v40000ppm = myCor/1000;
		saveState(3, (byte)(myCor & 0xFF));
		saveState(4, (byte)((myCor >> 8) & 0xFF));
		send(msgDiagnostic.set(myCor));
		#ifdef MY_DEBUG
		Serial.print("v10000ppm ");
		Serial.println(myCor);
		#endif
		break;

	case V_VAR4: //V_VAR4 * 10000 - SLEEP_TIME, mls
		SLEEP_TIME = (unsigned long)myCor * 1000;
		saveState(7, (int)(myCor / 10));
		send(msgDiagnostic.set((uint32_t)SLEEP_TIME));
		#ifdef MY_DEBUG
		Serial.print("SLEEP_TIME ");
		Serial.println(SLEEP_TIME);
		#endif
		break;

	case V_VAR5: //sendVolts
		sendVolts = message.getBool();
		saveState(8, (byte)(sendVolts));
		send(msgDiagnostic.set((int)(sendVolts)));
		#ifdef MY_DEBUG
		Serial.print("sendVolts ");
		Serial.println(sendVolts);
		#endif
		break;
	case V_STATUS: //Heater on/off
		digitalWrite(CO_VCC, message.getBool() ? HIGH : LOW);
		send(msg4.set((message.getBool())?"ON":"OFF"));
#ifdef MY_DEBUG
		Serial.print("sendVolts ");
		Serial.println(sendVolts);
#endif
		break;
	default:
		break;
	}
	deltavs = v400ppm - v40000ppm;
	A = deltavs / (log10(400) - log10(40000));
#ifdef MY_DEBUG
	Serial.println("----------------------------");
	Serial.print("Incoming message.type: ");
	Serial.print(message.type);
	Serial.print(" Value ");
	Serial.println(myCor);
	Serial.println(message.getInt());
	Serial.println("----------------------------");
#endif
}