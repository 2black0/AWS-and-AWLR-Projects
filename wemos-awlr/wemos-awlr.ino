#define _TASK_SLEEP_ON_IDLE_RUN
#include <TaskScheduler.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_INA219.h>
#include <SIM800L.h>
#include <HCSR04.h>
#include <RtcDS1307.h>
#include <SPI.h>
#include <SD.h>
#include <SDConfigFile.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

#define LoopTime 60000

#define SIM800_RST_PIN 5
#define TrigPin 15
#define EchoPin 16

const char CONFIG_FILE[] = "configuration.txt";
boolean DebugLCD;
boolean GPRSSending;
int SendTime;
unsigned long SendTime2;
int HeightParam;
char *SSID;
char *PASS;
char *APN;
String writeapikey;

String url;
char str[175];
String link  = "http://api.sipasi.online/?key=";
String C00 = "&C0=";
String C01 = "&C1=";
String C02 = "&C2=";
String C15 = "&C15=";

int sendCount = 1;
int loopCount = 0;
int notsendCount = 0;

float volt;
long rawdis;
float dis;
float height;

String svolt;
String sdis;
String sheight;
String ssend;

String dataString;
String dateString;
String timeString;
String dataString1;
String dataString2;

LiquidCrystal_I2C lcd(0x3F, 20, 4);
RtcDS1307<TwoWire> Rtc(Wire);
RtcDateTime now;
Adafruit_INA219 ina219;
HCSR04 ultrasonicSensor(TrigPin, EchoPin, 20, 300);
SIM800L* sim800l;

Scheduler runner;
void t1Callback();
Task t1(LoopTime, TASK_FOREVER, &t1Callback);

// ######################################################
void setup() {
	pinMode(TrigPin, OUTPUT);
	pinMode(EchoPin, INPUT);
	Serial.begin(4800);
	Wire.begin(0,2);
	lcd.begin(0,2);
	Rtc.Begin(0,2);
	ultrasonicSensor.begin();
	ina219.begin();
  ina219.setCalibration_16V_400mA();
	SD.begin(TrigPin);
	delay(500);
	sim800l = new SIM800L((Stream *)&Serial, SIM800_RST_PIN, 200, 512);
	delay(100);

	if (!SD.begin(TrigPin)) {
		showLCD(1,0,0,"[SD]Not Found",0,1, "Insert SD Card!",0,2, "", 0,3, "", 1000);
		return;
	}
	readConfiguration();

	for(uint8_t t=3; t>0; t--){
		showLCD(1,0,0,"[ST]WAIT " + String(t),0,1, "",0,2, "", 0,3, "", 500); 
	}

	RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
	if (!Rtc.IsDateTimeValid()) {
		showLCD(1,0,0,"[RTC]Lost",0,1, "",0,2, "", 0,3, "", 1000);
		Rtc.SetDateTime(compiled);
	}
	if (!Rtc.GetIsRunning()) {
		showLCD(1,0,0,"[RTC]Starting",0,1, "",0,2, "", 0,3, "", 1000);
		Rtc.SetIsRunning(true);
	}

	now = Rtc.GetDateTime();
	if (now < compiled) {
		showLCD(1,0,0,"[RTC]Update",0,1, "",0,2, "", 0,3, "", 1000);
		Rtc.SetDateTime(compiled);
	}
	else if (now > compiled) {
		showLCD(1,0,0,"[RTC]Newer",0,1, "",0,2, "", 0,3, "", 1000);  
	}
	else if (now == compiled) {
		showLCD(1,0,0,"[RTC]Same",0,1, "",0,2, "", 0,3, "", 1000);   
	}
	Rtc.SetSquareWavePin(DS1307SquareWaveOut_Low);

	showConfiguration();
	showLCD(1, 0, 0, "[ST]AWLR by setoFPV", 0, 1, "2black0@gmail.com", 0, 2, "", 0, 3, "", 1000);

	if(GPRSSending){
		showLCD(1, 0, 0, "[ST]GPRS Internet", 0, 1, "", 0, 2, "", 0, 3, "", 1000);
		setupGPRSModule();
	}
	else {
		showLCD(1, 0, 0, "[ST]WIFI Internet", 0, 1, "", 0, 2, "", 0, 3, "", 1000);
		setupWIFIModule();
	}

	ESP.wdtEnable(LoopTime+60000);
	delay(100);
	runner.init();
  runner.addTask(t1);
  t1.enable();
}

// ######################################################
boolean readConfiguration() {
	showLCD(1,0,0,"[CF]Read Config",0,1, "",0,2, "", 0,3, "", 500);
	readDefaultSetting();
	const uint8_t CONFIG_LINE_LENGTH = 127;
	SDConfigFile cfg;

	if (!cfg.begin(CONFIG_FILE, CONFIG_LINE_LENGTH)) {
		showLCD(1,0,0,"[CF]Failed Open",0,1, CONFIG_FILE,0,2, "", 0,3, "", 1000);
		return false;
	}

	while (cfg.readNextSetting()) {
		if (cfg.nameIs("DebugLCD")) {
			DebugLCD = cfg.getBooleanValue();
		}
		else if (cfg.nameIs("GPRSSending")) {
			GPRSSending = cfg.getBooleanValue();
		}
		else if (cfg.nameIs("SendTime")) {
			SendTime = cfg.getIntValue();
		}
		else if (cfg.nameIs("HeightParam")) {
			HeightParam = cfg.getIntValue();
		}
		else if (cfg.nameIs("SSID")) {
			SSID = cfg.copyValue();
		} 
		else if (cfg.nameIs("PASS")) {
			PASS = cfg.copyValue();
		} 
    else if (cfg.nameIs("APN")) {
			APN = cfg.copyValue();
		} 
		else if (cfg.nameIs("writeapikey")) {
			writeapikey = cfg.copyValue();
		}  
	}
	cfg.end();
	SendTime2 = SendTime * 1000;
	showLCD(1, 0, 0, "[RC]Read Config OK", 0, 1, "", 0, 2, "", 0, 3, "", 500);
	return true;
}

// ######################################################
void readDefaultSetting(){
	showLCD(1,0,0,"[CF]Read Default",0,1, "",0,2, "", 0,3, "", 500);
	DebugLCD = true;
	GPRSSending = true;
	SendTime = 300;
	HeightParam = 300;
	SSID = "wifi-testing";
	PASS = "arDY1234";
  APN = "internet";
	writeapikey = "83d50f00647411219539961dd76ebe6a";
}

// ######################################################
void showConfiguration() {
	String awlrNo;
	if(writeapikey == "83d50f00647411219539961dd76ebe6a") awlrNo = "AWLR-A01";
	if(writeapikey == "07311a9cf292b95a8afa0ecd74b82220") awlrNo = "AWLR-A02";
	if(writeapikey == "de05f3f22fd6340997f9638a2f2b3bd9") awlrNo = "AWLR-A03";
	if(writeapikey == "8c3beb53438f6b1be8ba3990da378dd8") awlrNo = "AWLR-A04";
	if(writeapikey == "f178cf4dd4e022f5cdcb0fc2de42eb3e") awlrNo = "AWLR-A05";
	if(writeapikey == "7704fd077b5b7e8b0eb670fa2ee0e51f") awlrNo = "AWLR-A06";
	if(writeapikey == "c52f5fc194be09c5ff1c01392f0f7517") awlrNo = "AWLR-A08";
	if(writeapikey == "ce03f925b28cd6d8ed9fed90663d90c6") awlrNo = "AWLR-A01";
	if(writeapikey == "4a5522fd4feda9b91f0b19b5154e1642") awlrNo = "AWLR-A09";
	if(writeapikey == "cf902753b7b4cec3ccface1728d67cea") awlrNo = "AWLR-A10";

	String apnssid;
	if(GPRSSending){
		apnssid = APN;
	}
	else{
		apnssid = SSID;
	}

  showLCD(1,0,0, "1:"  + String(DebugLCD)  + " 2:"  + String(GPRSSending),
            0,1, "3:" + String(LoopTime/1000) + " 4:" + String(SendTime),
            0,2, awlrNo, 
            0,3, apnssid, 2500);
}

// ######################################################
void setupGPRSModule() {
	showLCD(1,0,0,"[SM]Setup GPRS",0,1, "",0,2, "", 0,3, "", 500);
	while(!sim800l->isReady()) {
		showLCD(1, 0, 0, "[SM]AT Command", 0, 1, "Retry in 1 Sec", 0, 2, "", 0, 3, "", 1);
		delay(1000);
	}
	showLCD(1, 0, 0, "[SM]Setup Process", 0, 1, "", 0, 2, "", 0, 3, "", 1000);

	uint8_t signal = sim800l->getSignal();
	while(signal <= 0) {
		delay(1000);
		signal = sim800l->getSignal();
	}
	showLCD(1, 0, 0, "[SM]Signal OK", 0, 1, "strenght: " + String(signal), 0, 2, "", 0, 3, "", 1);
	delay(1000);

	NetworkRegistration network = sim800l->getRegistrationStatus();
	while(network != REGISTERED_HOME && network != REGISTERED_ROAMING) {
		delay(1000);
		network = sim800l->getRegistrationStatus();
	}
	showLCD(1, 0, 0, "[SM]Network OK", 0, 1, "", 0, 2, "", 0, 3, "", 1);
	delay(1000);

	bool success = sim800l->setupGPRS(APN);
	while(!success) {
		success = sim800l->setupGPRS(APN);
		delay(5000);
	}
	showLCD(1, 0, 0, "[SM]GPRS Config OK", 0, 1, "", 0, 2, "", 0, 3, "", 1000);

	bool connected = false;
	for(uint8_t i = 0; i < 5 && !connected; i++) {
		delay(1000);
		connected = sim800l->connectGPRS();
	}

	if(connected) {
		showLCD(1, 0, 0, "[SM]GPRS Connected", 0, 1, "", 0, 2, "", 0, 3, "", 1000);
	} 
	else {
		showLCD(1, 0, 0, "[SM]GPRS Not Connect", 0, 1, "Reset SIM800L", 0, 2, "", 0, 3, "", 1000);
		sim800l->reset();
		setupGPRSModule();
		return;
	}
}

// ######################################################
void setupWIFIModule() {
	showLCD(1,0,0,"[SM]Setup WIFI",0,1, "",0,2, "", 0,3, "", 500);
	WiFi.begin(SSID, PASS);
	int i = 0;
	showLCD(1, 0, 0, "", 0, 1, "", 0, 2, "", 0, 3, "", 1);
	while (WiFi.status() != WL_CONNECTED) {
		showLCD(0, i, 0, ".", 0, 1, "", 0, 2, "", 0, 3, "", 500);
		i += 1;
		if (i>=15) {
			i = 0;
			showLCD(1, 0, 0, "", 0, 1, "", 0, 2, "", 0, 3, "", 1);
		}
	}
	showLCD(1, 0, 0, "[ST]WIFI Connected", 0, 1, "", 0, 2, "", 0, 3, "", 1000);
}

// ######################################################
void loop() {
  runner.execute();
}

// ######################################################
void t1Callback(){
  reloadWDT();
	now = Rtc.GetDateTime();

  if((sendCount >= 10) || (notsendCount >= 2)){
    showLCD(1, 0, 0, "[R]10 Data Sent!", 0, 1, "or not Sent", 0, 2, "Reset System", 0, 3, "", 1000);
    ESP.reset();
  }

	if((sendCount == 1) || (loopCount >= SendTime2 / LoopTime)) {
		clearVar();
		readSensor();
		stringData(now);
		sdData();
		delay(250);
		buildURL();
		bool status = sendGPRS(GPRSSending);
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
	showLCD(1, 0, 0, "[WDT]Reload WDT!!!", 0, 1, "", 0, 2, "", 0, 3, "", 1000);
	ESP.wdtFeed();
	showResult();
}

// ######################################################
void showLCD(bool clr, int r1, int c1, String chara1, int r2, int c2, String chara2, int r3, int c3, String chara3, int r4, int c4, String chara4, int tdelay) {
  if(DebugLCD) {
    if (clr == 1){
      lcd.clear();
    }
    lcd.setCursor(r1, c1);
    lcd.print(chara1);
    lcd.setCursor(r2, c2);
    lcd.print(chara2);
    lcd.setCursor(r3, c3);
    lcd.print(chara3);
    lcd.setCursor(r4, c4);
    lcd.print(chara4);
  }
  delay(tdelay);
}

// ######################################################
void clearVar() {
	showLCD(1, 0, 0, "[CV]Clear Variable", 0, 1, "", 0, 2, "", 0, 3, "", 1000);
	volt    = 0;
	dis     = 0;
	height  = 0;

	svolt 	= "";
	sdis    = "";
	sheight = "";
	ssend   = "";

	dataString = "";
	dataString1= "";
	dataString2 = "";
}

// ######################################################
void readDistance() {
	showLCD(1,0,0,"[S]Read Distance",0,1, "",0,2, "", 0,3, "", 500);	
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
void readINA(){
	showLCD(1,0,0,"[S]Read Voltage",0,1, "",0,2, "", 0,3, "", 500);
	volt = ina219.getBusVoltage_V();
}

// ######################################################
void readSensor(){
	showLCD(1,0,0,"[S]Read Sensor",0,1, "",0,2, "", 0,3, "", 500);
	readDistance();
	readINA();

  //showLCD(1,0,0,"[SENSOR]Read All ",0,1, "volt:" + String(volt) + "V",0,2, "dis:" + String(dis) + "cm", 0,3, "height:" + String(height) + "cm", 1);
}

// ######################################################
void stringData(const RtcDateTime& dt){
	showLCD(1,0,0,"[SD]String Data",0,1, "",0,2, "", 0,3, "", 500);
	dateString = String(dt.Day(),DEC) + "/" + String(dt.Month(),DEC) + "/" + String(dt.Year(),DEC);
	timeString = String(dt.Hour(),DEC) + ":" + String(dt.Minute(),DEC) + ":" + String(dt.Second(),DEC);

  dataString = dateString + "#" + timeString + "#" + 
               String(volt) + "#" + String(dis)  + "#" + String(height)  + "#" + 
               String(sendCount)  + "\r\n";

  dataString1 = dateString + "#" + timeString;
  dataString2 = String(volt) + "#" + String(dis)  + "#" + String(height);
}

// ######################################################
void sdData(){
	showLCD(1,0,0,"[SD]Save Data",0,1, "",0,2, "", 0,3, "", 500);

	File file = SD.open("/datalog.txt", FILE_WRITE);
  if(!file) {
		showLCD(1,0,0,"[SD]Create File",0,1, "datalog.txt",0,2, "", 0,3, "", 500);
		file.print("Date (DMY), Time (HMS), Voltage (V), Distance (cm), Height (cm) \r\n");
  }
  else {
		showLCD(1,0,0,"[SD]File Exist",0,1, "",0,2, "", 0,3, "", 500);
  }
	file.print(dataString);
  file.close();
}

// ######################################################
void buildURL() {
	showLCD(1, 0, 0, "[BU]Build URL", 0, 1, "", 0, 2, "", 0, 3, "", 1000);
	url = "";

	svolt	   = String(volt);
	sdis 	   = String(dis);
	sheight	 = String(height);
	ssend    = String(sendCount);

	url += link;
	url += writeapikey;
	url += C00;
	url += svolt;
	url += C01;
	url += sdis;
	url += C02;
	url += sheight;
	url += C15;
	url += ssend;

	url.toCharArray(str, 175);
	//showLCD(1, 0, 0, String(str), 0, 1, "", 0, 2, "", 0, 3, "", 2000);
}

// ######################################################
bool sendGPRS(bool GPRSSending) {
	bool status;
	if(GPRSSending){
		status = GPRSProcess();
		}
	else{
		status = WIFIProcess();
	}
	showResult();
	return status;
}

// ######################################################
bool GPRSProcess() {
	showLCD(1, 0, 0, "[SG]Start HTTP Get", 0, 1, "", 0, 2, "", 0, 3, "", 1000);
	bool status;

	uint16_t rc = sim800l->doGet(str, 10000);
	if(rc == 200) {
		showLCD(1, 0, 0, "[SG]HTTP Get OK", 0, 1, "", 0, 2, "", 0, 3, "", 1000);
		String payload = sim800l->getDataReceived();
		showLCD(1, 0, 0, "[SW]Payload:", 0, 1, payload.substring(1,10), 0, 2, "", 0, 3, "", 1000);
		status = 1;
	} 
	else {
		showLCD(1, 0, 0, "[SG]HTTP GET Error", 0, 1, "", 0, 2, "", 0, 3, "", 1000);
		status = 0;
	}
	return status;
}

// ######################################################
bool WIFIProcess() {
	showLCD(1, 0, 0, "[SW]Start HTTP Get", 0, 1, "", 0, 2, "", 0, 3, "", 1000);
	bool status;

	if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
		http.begin(url);
		int httpCode = http.GET();
		if (httpCode > 0) {
			showLCD(1, 0, 0, "[SW]HTTP Get OK", 0, 1, "Response Code:", 0, 2, String(httpCode), 0, 3, "", 1000);
			String payload = http.getString();
			//{"valid":1,
			showLCD(1, 0, 0, "[SW]Payload:", 0, 1, payload.substring(1,10), 0, 2, "", 0, 3, "", 1000);
			status = 1;
    }
		else {
			//showLCD(1, 0, 0, "[SW]Error Code:", 0, 1, http.errorToString(httpCode).c_str(), 0, 2, "", 0, 3, "", 1000);
			showLCD(1, 0, 0, "[SW]HTTP GET Error", 0, 1, "Error Code:", 0, 2, String(httpCode), 0, 3, "", 1000);
			status = 0;
    }
		http.end();
	}
	else {
		setupWIFIModule();
		showLCD(1, 0, 0, "[SW]WIFI NoT Connect", 0, 1, "", 0, 2, "", 0, 3, "", 1000);
		status = 0;
	}
	return status;
}

// ######################################################
void showResult() {
	showLCD(1,0,0, dataString1, 0,1, dataString2, 0,2, "", 0,3, "", 1);
	showLCD(0,0,0, "", 0,1, "", 0,2, "", 19,3, String(sendCount), 1000);
}