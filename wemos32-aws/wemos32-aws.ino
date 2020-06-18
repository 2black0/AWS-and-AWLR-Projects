#define _TASK_SLEEP_ON_IDLE_RUN
#include <TaskScheduler.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <RtcDS1307.h>
#include <Adafruit_INA219.h>
#include <SPI.h>
#include <SD.h>
#include <SDConfigFile.h>
#include <SHT1x-ESP.h>
#include <SIM800L.h>
#include <WiFi.h>
#include <HTTPClient.h>

#define LoopTime 20000

#define CSPin 5 // CSPin of SDCard Module
#define SHTclkPin 14 // clockPin of SHT11
#define SHTdatPin 33 // dataPin of SHT11
#define SIM800_TX_PIN 16 // TX pin of SIM800L
#define SIM800_RX_PIN 17 // RX pin of SIM800L
#define SIM800_RST_PIN 32 // Reset pin of SIM800L
#define wdtPin 25 // wdtPin Trigger
#define uvPin 34 // UV Sensor Analog Out

const char CONFIG_FILE[] = "/configuration.txt";
boolean DebugLCD;
boolean GPRSSending;
int SendTime;
unsigned long SendTime2;
char *SSID;
char *PASS;
char *APN;
String writeapikey;

String url;
char str[215];
String links  = "http://api.sipasi.online/?key=";
String C00 = "&C0=";
String C01 = "&C1=";
String C02 = "&C2=";
String C03 = "&C3=";
String C04 = "&C4=";
String C05 = "&C5=";
String C06 = "&C6=";
String C07 = "&C7=";
String C08 = "&C8=";
String C09 = "&C9=";
String C10 = "&C10=";
String C11 = "&C11=";
String C15 = "&C15=";

int sendCount = 1;
int loopCount = 0;
int notsendCount = 0;

float volt;
float tempOut;
float humOut;
float windMax;
float rainHour;
float rainDay;
float tempIn;
float pressure;
float uvSun;
int winDir;
int humIn;
double temp;
char databuffer[35];

String svolt;
String stem;
String shum;
String swdir;
String swmx;
String sroh;
String srod;
String stemi;
String shumi;
String sbar;
String suv;
String ssend;
String snot;

String dateString;
String timeString;
String dataString;
String dataString1;
String dataString2;
String dataString3;
String dataString4;

LiquidCrystal_I2C lcd(0x3F, 20, 4);
RtcDS1307<TwoWire> Rtc(Wire);
RtcDateTime now;
Adafruit_INA219 ina219;
SIM800L* sim800l;
SHT1x sht1x(SHTdatPin, SHTclkPin);

Scheduler runner;
void t1Callback();
Task t1(LoopTime, TASK_FOREVER, &t1Callback);

// ######################################################
void setup() {
	pinMode(wdtPin, INPUT);
	pinMode(CSPin, OUTPUT);
	Serial.begin(115200);
	Serial1.begin(9600);
	Serial2.begin(4800, SERIAL_8N1, SIM800_TX_PIN, SIM800_RX_PIN);
	Wire.begin(21,22);
	lcd.init();
	lcd.backlight();
	Rtc.Begin(21,22);
	ina219.begin();
  ina219.setCalibration_16V_400mA();
	SD.begin(CSPin);
	delay(500);
	sim800l = new SIM800L((Stream *)&Serial2, SIM800_RST_PIN, 300, 512);
	delay(100);

	if (!SD.begin(CSPin)) {
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
	showLCD(1, 0, 0, "[ST]AWS by Ardy Seto", 0, 1, "2black0@gmail.com", 0, 2, "", 0, 3, "", 1000);

	if(GPRSSending){
		showLCD(1, 0, 0, "[ST]GPRS Internet", 0, 1, "", 0, 2, "", 0, 3, "", 1000);
		setupGPRSModule();
	}
	else {
		showLCD(1, 0, 0, "[ST]WIFI Internet", 0, 1, "", 0, 2, "", 0, 3, "", 1000);
		setupWIFIModule();
	}

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
	SSID = "wifi-testing";
	PASS = "arDY1234";
  APN = "internet";
	writeapikey = "83d50f00647411219539961dd76ebe6a";
}


// ######################################################
void showConfiguration() {
	String awsNo;
	if(writeapikey == "83d50f00647411219539961dd76ebe6a") awsNo = "AWS-A01";
	if(writeapikey == "07311a9cf292b95a8afa0ecd74b82220") awsNo = "AWS-A02";

	String apnssid;
	if(GPRSSending){
		apnssid = APN;
	}
	else{
		apnssid = SSID;
	}

  showLCD(1,0,0, "1:"  + String(DebugLCD)  + " 2:"  + String(GPRSSending),
            0,1, "3:" + String(LoopTime/1000) + " 4:" + String(SendTime),
            0,2, awsNo, 
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
    ESP.restart();
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
	showLCD(1, 0, 0, "[WDT]Reload WDT!!!", 0, 1, "", 0, 2, "", 0, 3, "", 500);
	pinMode(wdtPin, OUTPUT);
  digitalWrite(wdtPin, LOW);
  delay(300);
  pinMode(wdtPin, INPUT);
  delay(500);
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
	showLCD(1, 0, 0, "[CV]Clear Variable", 0, 1, "", 0, 2, "", 0, 3, "", 500);
	volt = 0;
	tempOut = 0;
	humOut = 0;
	windMax = 0;
	rainHour = 0;
	rainDay = 0;
	tempIn = 0;
	pressure = 0;
	uvSun = 0;
	winDir = 0;
	humIn = 0;

	svolt = "";
	stem = "";
	shum = "";
	swdir = "";
	swmx = "";
	sroh = "";
	srod = "";
	stemi = "";
	shumi = "";
	sbar = "";
	suv = "";
	ssend = "";
	snot = "";

	dataString = "";
	dataString1= "";
	dataString2 = "";
	dataString3 = "";
	dataString4 = "";
}

// ######################################################
void readINA(){
	showLCD(1,0,0,"[S]Read Voltage",0,1, "",0,2, "", 0,3, "", 500);
	volt = ina219.getBusVoltage_V();
}

// ######################################################
void readSHT(){
	showLCD(1,0,0,"[S]Read Temp & Hum",0,1, "",0,2, "", 0,3, "", 500);
	tempOut = sht1x.readTemperatureC();
  humOut = sht1x.readHumidity();
}

// ######################################################
void readWeather(){
	showLCD(1,0,0,"[S]Read Weather",0,1, "",0,2, "", 0,3, "", 500);
  getBuffer();
  
  while (BarPressure() >= 1041 || BarPressure() <=  979 || Temperature() >= 55 || Temperature() <= -5 || Humidity() > 100 || Humidity() < 0 ) {
    delay(250);
    getBuffer();
  }

  int BarMax = (BarPressure() +5);
  int BarMin = (BarPressure() -5);
  int TempMax = (Temperature() +5);
  int TempMin = (Temperature() -5);
  int HumidMax = (Humidity() +5);
  int HumidMin = (Humidity() -5);
  bool NotAccurate;
  getBuffer();

  while (BarPressure() >= 1041 || BarPressure() <=  979 || Temperature() >= 55 || Temperature() <= -5 || Humidity() > 100 || Humidity() < 0 ) {
    delay(250);
    getBuffer();
  }

  if (BarPressure() > BarMax || BarPressure() < BarMin || Temperature() > TempMax || Temperature() < TempMin || Humidity() > HumidMax || Humidity() < HumidMin ) {
    NotAccurate = true;
		showLCD(1,0,0,"[S]Bad weather data",0,1, "",0,2, "", 0,3, "", 500);
  }
  else {
    NotAccurate = false;
  }
  
  if (NotAccurate == false) {
    winDir = WindDirection();
    windMax = WindSpeedMax();
    rainHour = RainfallOneHour();
    rainDay = RainfallOneDay();
    tempIn = Temperature();
    humIn = Humidity();
    pressure = BarPressure();  
  }
}

// ######################################################
void getBuffer() {
  int index;
  for (index = 0;index < 35;index ++) {
    if(Serial1.available()) {
      databuffer[index] = Serial1.read();
      if (databuffer[0] != 'c') {
        index = -1;
      }
    }
    else {
      index --;
    }
  }
}

int transCharToInt(char *_buffer,int _start,int _stop) {
  int _index;
  int result = 0;
  int num = _stop - _start + 1;
  int _temp[num];
  for (_index = _start;_index <= _stop;_index ++) {
    _temp[_index - _start] = _buffer[_index] - '0';
    result = 10*result + _temp[_index - _start];
  }
  return result;
}

int WindDirection() {
  return transCharToInt(databuffer,1,3);
}

float WindSpeedAverage() {
  temp = 0.44704 * transCharToInt(databuffer,5,7);
  return temp;
}

float WindSpeedMax() {
  temp = 0.44704 * transCharToInt(databuffer,9,11);
  return temp;
}

float Temperature() {
  temp = (transCharToInt(databuffer,13,15) - 32.00) * 5.00 / 9.00;
  return temp;
}

float RainfallOneHour() {
  temp = transCharToInt(databuffer,17,19) * 25.40 * 0.01;
  return temp;
}

float RainfallOneDay() {
  temp = transCharToInt(databuffer,21,23) * 25.40 * 0.01;
  return temp;
}

int Humidity() {
  return transCharToInt(databuffer,25,26);
}

float BarPressure() {
  temp = transCharToInt(databuffer,28,32);
  return temp / 10.00;
}

// ######################################################
void readUV(){
	showLCD(1,0,0,"[S]Read Solar Irr.",0,1, "",0,2, "", 0,3, "", 500);
	int uvLevel;
  float uvVolt;
  uvLevel = averageAnalogRead(uvPin);
  uvVolt = 3.3 * uvLevel / 4095;
  uvVolt += 0.173;
  uvSun = mapfloat(uvVolt, 0.99, 2.9, 0.0, 15.0);
  if(uvSun < 0) uvSun = 0;
}

// ######################################################
int averageAnalogRead(int pinToRead){
  byte numberOfReadings = 8;
  unsigned int runningValue = 0; 
  for(int x = 0 ; x < numberOfReadings ; x++) {
    runningValue += analogRead(pinToRead);
  }
  runningValue /= numberOfReadings;
  return(runningValue);  
}

// ######################################################
float mapfloat(float x, float in_min, float in_max, float out_min, float out_max){
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

// ######################################################
void readSensor(){
	showLCD(1,0,0,"[S]Read Sensor",0,1, "",0,2, "", 0,3, "", 500);
	readINA();
	readSHT();
	readWeather();
	readUV();

	//showResult();
}

// ######################################################
void stringData(const RtcDateTime& dt){
	showLCD(1,0,0,"[SD]String Data",0,1, "",0,2, "", 0,3, "", 500);
	dateString = String(dt.Day(),DEC) + "/" + String(dt.Month(),DEC) + "/" + String(dt.Year(),DEC);
	timeString = String(dt.Hour(),DEC) + ":" + String(dt.Minute(),DEC) + ":" + String(dt.Second(),DEC);

  dataString = dateString + "," + timeString + "," + 
               String(volt) + ","+ String(uvSun) + ","+ String(windMax)+ ","+
							 String(winDir) + ","+ String(tempIn) + ","+ String(humIn) + ","+
               String(tempOut) + ","+ String(humOut) + ","+
							 String(rainHour) + ","+ String(rainDay)+ ","+ String(pressure) + ","+ 
               String(sendCount)  + "\r\n";
  
  dataString1 = dateString + "," + timeString;
  dataString2 = String(volt) + "," + String(uvSun) + "," + String(windMax) + ","+ String(winDir);
  dataString3 = String(tempIn) + ","+ String(humIn) + ","+ String(tempOut) + ","+ String(humOut);
  dataString4 = String(rainHour) + ","+ String(rainDay)+ ","+ String(pressure);
}

// ######################################################
void sdData(){
	showLCD(1,0,0,"[SD]Save Data",0,1, "",0,2, "", 0,3, "", 500);

	File file = SD.open("/datalog.txt");
  if(!file) {
		showLCD(1,0,0,"[SD]Create File",0,1, "datalog.txt",0,2, "", 0,3, "", 500);
    writeFile(SD, "/datalog.txt", "Date (DMY), Time (HMS), Voltage (V), Solar Irradiance (Watt/m^2), Wind Max (m/s), Wind Direction (1-8), Temperature In (*C), Humidity In (%), Temperature Out (*C), Humidity Out (%), Rain Hour (mm), Rain Day (mm), Pressure (hPa), Send Count (1-9) \r\n");
  }
  else {
		showLCD(1,0,0,"[SD]File Exist",0,1, "",0,2, "", 0,3, "", 500);
  }
  file.close();

	appendFile(SD, "/datalog.txt", dataString.c_str());
}

// ######################################################
void writeFile(fs::FS &fs, const char * path, const char * message) {
	showLCD(1,0,0,"[SD]Writing File",0,1, path,0,2, "", 0,3, "", 500);

  File file = fs.open(path, FILE_WRITE);
  if(!file) {
		showLCD(1,0,0,"[SD]Write Failed",0,1, "",0,2, "", 0,3, "", 500);
    return;
  }
  if(file.print(message)) {
		showLCD(1,0,0,"[SD]File Written",0,1, "",0,2, "", 0,3, "", 500);
  } else {
		showLCD(1,0,0,"[SD]Write Failed",0,1, "",0,2, "", 0,3, "", 500);
  }
  file.close();
}

// ######################################################
void appendFile(fs::FS &fs, const char * path, const char * message) {
	showLCD(1,0,0,"[SD]Append File",0,1, path,0,2, "", 0,3, "", 500);

  File file = fs.open(path, FILE_APPEND);
  if(!file) {
		showLCD(1,0,0,"[SD]Append Failed",0,1, "",0,2, "", 0,3, "", 500);
    return;
  }
  if(file.print(message)) {
		showLCD(1,0,0,"[SD]Data Appended",0,1, "",0,2, "", 0,3, "", 500);
  } else {
		showLCD(1,0,0,"[SD]Append Failed",0,1, "",0,2, "", 0,3, "", 500);
  }
  file.close();
}

// ######################################################
void buildURL() {
	showLCD(1, 0, 0,"[BU]Build URL", 0, 1, "", 0, 2, "", 0, 3, "", 500);
	url = "";
	svolt = String(volt);
	suv = String(uvSun);
	swmx = String(windMax);
	swdir = String(winDir);
	stemi = String(tempIn);
	shumi = String(humIn);
	stem = String(tempOut);
	shum = String(humOut);
	sroh = String(rainHour);
	srod = String(rainDay);
	sbar = String(pressure);
	ssend = String(sendCount);

	url += links;
	url += writeapikey;
	url += C00;
	url += svolt;
	url += C01;
	url += suv;
	url += C02;
	url += swmx;
	url += C03;
	url += swdir;
	url += C04;
	url += stemi;
	url += C05;
	url += shumi;
	url += C06;
	url += stem;
	url += C07;
	url += shum;
	url += C08;
	url += sroh;
	url += C09;
	url += srod;
	url += C10;
	url += sbar;
	url += C15;
	url += ssend;

	url.toCharArray(str, 215);
	//Serial.println(url);
	//Serial.println(String(str));
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
  showLCD(1,0,0, dataString1, 0,1, dataString2, 0,2, dataString3, 0,3, dataString4, 1);
	showLCD(0,0,0, "", 0,1, "", 0,2, "", 19,3, String(sendCount), 1000);
}