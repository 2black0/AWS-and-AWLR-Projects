#define _TASK_SLEEP_ON_IDLE_RUN
#include <TaskScheduler.h>
#include <SIM800L.h>
#include <HCSR04.h>
#include <IWatchdog.h>

#define LoopTime 60000
#define SendTime 300000

#define EchoPin A0
#define TrigPin A1
#define SIM800_RST_PIN PB5 

//************************* Change this Prameter *************************//
const char APN[] = "internet";
String writeapikey = "4c739a8d1435fde3c3f81befd2b50404"; // 1
int HeightParam = 300;
float voltParam = 43.889; // tegangan baterai / adc terukur (C15)
// No. ADC # 3.3V # Bat
// 1. B-01 9f20aaca2c1e81d073a63ebc18731fcf
// 2. B-02 95d4cfa15a55edf7950e4c2e9fb01425
// 3. B-03 a74f63fdc6383fa1cb0fc3512c483fa4
// 4. B-04 4c739a8d1435fde3c3f81befd2b50404
//************************* Change this Prameter *************************//

String url;
char str[175];
String link  = "http://api.sipasi.online/?key=";
String C00 = "&C0=";
String C01 = "&C1=";
String C02 = "&C2=";
String C14 = "&C14=";
String C15 = "&C15=";

int sendCount = 1;
int loopCount = 0;
int notsendCount = 0;

float volt;
float rawvolt;
long rawdis;
float dis;
float height;

String svolt;
String sdis;
String sheight;
String srawvolt;
String ssend;

HardwareSerial Serial2(PA3, PA2);
HCSR04 ultrasonicSensor(TrigPin, EchoPin, 20, 300);
SIM800L* sim800l;

Scheduler runner;
void t1Callback();
Task t1(LoopTime, TASK_FOREVER, &t1Callback);

// ######################################################
void setup() {
	pinMode(TrigPin, OUTPUT);
	pinMode(EchoPin, INPUT);
	Serial.begin(115200);
	Serial2.begin(4800);
	ultrasonicSensor.begin();
	delay(500);
	
	sim800l = new SIM800L((Stream *)&Serial2, SIM800_RST_PIN, 200, 512);
	Serial.println(F("[Start]AWLR by Ardy Seto"));
	Serial.println(F("[Start]2black0@gmail.com"));

	setupModule();
	IWatchdog.begin((LoopTime+60000)*1000);
	delay(100);
	runner.init();
  runner.addTask(t1);
  t1.enable();
}

// ######################################################
void setupModule() {
	Serial.println(F("[Setup Module]Setup Module Started"));
	while(!sim800l->isReady()) {
		Serial.println(F("[Setup Module]Problem to initialize AT command, retry in 1 sec"));
		delay(1000);
	}
	Serial.println(F("[Setup Module]Setup Complete!"));

	uint8_t signal = sim800l->getSignal();
	while(signal <= 0) {
		delay(1000);
		signal = sim800l->getSignal();
	}
	Serial.print(F("[Setup Module]Signal OK (strenght: "));
	Serial.print(signal);
	Serial.println(F(")"));
	delay(1000);

	NetworkRegistration network = sim800l->getRegistrationStatus();
	while(network != REGISTERED_HOME && network != REGISTERED_ROAMING) {
		delay(1000);
		network = sim800l->getRegistrationStatus();
	}
	Serial.println(F("[Setup Module]Network registration OK"));
	delay(1000);

	bool success = sim800l->setupGPRS(APN);
	while(!success) {
		success = sim800l->setupGPRS(APN);
		delay(5000);
	}
	Serial.println(F("[Setup Module]GPRS config OK"));

	bool connected = false;
	for(uint8_t i = 0; i < 5 && !connected; i++) {
		delay(1000);
		connected = sim800l->connectGPRS();
	}

	if(connected) {
		Serial.println(F("[Setup Module]GPRS connected !"));
	} 
	else {
		Serial.println(F("[Setup Module]GPRS not connected !"));
		Serial.println(F("[Setup Module]Reset the module."));
		sim800l->reset();
		setupModule();
		return;
	}
}

// ######################################################
void loop() {
  runner.execute();
}

// ######################################################
void t1Callback(){
  reloadWDT();

  if((sendCount >= 11) || (notsendCount >= 2)){
		Serial.println(F("[Reset]10 Data Sent!"));
		Serial.println(F("[Reset]Reset System"));
		delay(1000);
		NVIC_SystemReset();
  }

	if((sendCount == 1) || (loopCount >= SendTime / LoopTime)) {
		clearVar();
		readSensor();
		delay(250);
		buildURL();
		bool status = sendGPRS();
		if(status == 1){
			sendCount = sendCount + 1;
			loopCount = 0;
		}
		else{
			notsendCount = notsendCount + 1;
			loopCount = 0;
		}
	}
	loopCount++;
}

// ######################################################
void reloadWDT() {
	Serial.println(F("[WatchDog Timer]Reload WDT!!!"));
	IWatchdog.reload();
}

// ######################################################
void clearVar() {
	Serial.println(F("[Clear Var]Clear Variable!"));
	volt    = 0.0;
	rawvolt = 0.0;
	dis     = 0.0;
	height  = 0.0;
	
	svolt 	= "";
	sdis    = "";
	sheight = "";
	srawvolt = "";
	ssend   = "";
}

// ######################################################
void readDistance() {
	Serial.println(F("[Read Sensor]Read Distance"));
	retryreadDistances:

	float totaldis = 0;
	for(int i=0; i<5; i++){
		dis = ultrasonicSensor.getMedianFilterDistance();
		if (dis != HCSR04_OUT_OF_RANGE) {
			dis = dis;
		}
		else {
			dis = ultrasonicSensor.getMedianFilterDistance();
		}
		totaldis = totaldis + dis;
	}
	dis = totaldis / 5;
  height = HeightParam - dis;

	if(dis >= 300){
		goto retryreadDistances;
	}
}

// ######################################################
void readVoltage(){
	Serial.println(F("[Read Sensor]Read Voltage"));
	float voltTotal = 0.0;
	float rawvoltTotal = 0.0;
	for (int i=0; i<20; i++) {
		rawvolt = analogRead(A4);
		volt = analogRead(A4) / voltParam;
    rawvoltTotal += rawvolt;
		voltTotal += volt;
  }
	rawvolt = rawvoltTotal/20;
	volt = voltTotal/20;
}

// ######################################################
void readSensor(){
	readDistance();
	readVoltage();
	delay(100);
	
	Serial.print(F("[Read Sensor]Volt:"));
	Serial.print(volt);
	Serial.print(F("V; ADC:"));
	Serial.print(rawvolt);
	Serial.print(F("; Distance:"));
	Serial.print(dis);
	Serial.print(F(" cm; Height:"));
	Serial.print(height);
	Serial.println(F("cm"));
}

// ######################################################
void buildURL() {
	Serial.println(F("[Build URL]Build URL"));
	url     = "";
	
	svolt	   = String(volt);
	sdis 	   = String(dis);
	sheight	 = String(height);
	srawvolt = String(rawvolt);
	ssend    = String(sendCount);

	url += link;
	url += writeapikey;
	url += C00;
	url += svolt;
	url += C01;
	url += sdis;
	url += C02;
	url += sheight;
	url += C14;
	url += srawvolt;
	url += C15;
	url += ssend;

	url.toCharArray(str, 175);
	Serial.print(F("[Build URL]URL: "));
  Serial.println(str);
	delay(1500);
}

// ######################################################
bool sendGPRS() {
	Serial.println(F("[Send GPRS]Start HTTP Get"));
	bool status;
	
	uint16_t rc = sim800l->doGet(str, 10000);
	if(rc == 200) {
		Serial.println(F("[Send GPRS]HTTP Get OK"));
		String payload = sim800l->getDataReceived();
		Serial.print(F("[Send GPRS]Payload:"));
		Serial.println(payload);
		status = 1;
	} 
	else {
		Serial.println(F("[Send GPRS]HTTP GET Error"));
		status = 0;
	}
	return status;
}