#include <SoftwareSerial.h>
#include <EEPROM.h>
#define SSID "Nyan_cat"
#define PASS "meowmeow"
#define DST_IP "192.168.10.5" 
SoftwareSerial dbgSerial(10, 11); // RX, TX
byte arr[]={0,0,0,0,0,0,0,0,0};
bool debug = true;
bool gotData = false;
bool sendData = false;
bool isTouched = false;
int eeprom_addr = 0;
//datagram description:
//options 1 byte | source 4 bytes | destination 4 bytes | person_ID 1 byte or animation 1 byte or R+G+B (3bytes) | buffer | buffer | buffer | buffer
//options - hello = 0(to server), setID = 1(from server), touched = 2(to server), animate = 3(from server), 
//			setColor = 4(from server), heartbeat = 5(from server)
byte datagram[16];
long val_set=0;
long master_server=0;

//animations:
//1 - charge (suck)
//2 - discharge
//3 - sparkle
bool oneshotCharge = false;
bool oneshotDischarge = false;
bool oneshotSparkle = false;

long cur_time = millis();
long prv_time = 0;
long last_heart_beat = 0;
//this var is an indication the system got a heartbeat from master_server in the last minutes. if not its sends "Hello" to get an ID.
bool gotHeartBeat;


//this function checks the heartbeat content and make sure that the server hasn't changed
//if it did its asks for a new ID so the new server could manage it
void heartbeat(byte _datagram[16])
{
	long val=0;
	val += _datagram[1] << 24;
	val += _datagram[2] << 16;
	val += _datagram[3] << 8;
	val += _datagram[4];
	
	if (master_server!= val)
	{
		close_connection();
		helloserver();
	}
	last_heart_beat = millis();
}

void setID(long _id)
{
	EEPROM.write(eeprom_addr, _id);
}
byte getID()
{
	return EEPROM.read(eeprom_addr);
}

void setup()
{
	datagram[0] = -1 ;
	// Open serial communications and wait for port to open:
	//can't be faster than 19200 for softserial
	dbgSerial.begin(9600);
	if (debug)
		Serial.begin(9600);
		Serial.println("beginning ESP8266... ");
	resetModule();
	//connect to the wifi
	boolean connected=false;
	while (!connected)
		if(connectWiFi())
		{
			connected = true;
			break;
		}
	//print the ip address - only for debugging
	if (debug)
	{
		dbgSerial.println("AT+CIFSR");
		Serial.println("ip address:");	
	}
	//this time frame is needed in order to let the ESP2866 time to 
	//AT-transmit the command - I guess its because the Arduino MCU is 
	//too fast for it regrading the commands.
	delay(10);
	while (dbgSerial.available())
	{
		Serial.write(dbgSerial.read());
	}
	connect_to_server();
}
void loop()
{
	//this is the part where the sensor integration comes in.
	if (isTouched)
	{
		datagram[0] = 2;
		//source - the bush
		long val_ID = getID();
		datagram[1] = val_ID << 24;
		datagram[2] = val_ID << 16;
		datagram[3] = val_ID << 8;
		datagram[4] = val_ID;
		// destination - the server
		datagram[5] = master_server << 24;
		datagram[6] = master_server << 16;
		datagram[7] = master_server << 8;
		datagram[8] = master_server;
		//datagram[9] = sensor data..
	}
		
	String cmd;
	cmd = "AT+CIPSEND=4,9";
	dbgSerial.println(cmd);
	if(dbgSerial.find(">"))
	{
		dbgSerial.write(arr,9);
	}else
	{
		dbgSerial.println("AT+CIPCLOSE");
		Serial.println("connect timeout");
		delay(1000);
		return;
	}
	delay(1000);
	//dbgSerial.println("+IPD");
	//dbgSerial.find("+IPD:");
	//	Serial.println("got IPD");
	while (dbgSerial.available())
	{
		char c = dbgSerial.read();
		//TODO - do we need the 10ms delay?
		delay(10);
		Serial.write(c);
		if(c=='\r') Serial.print('\n');
	}
	delay(1000);
	if (gotData or sendData)
	{
		//after we get a datagram we need to check the "option" byte of it in order to know which function to trigger
		switch (datagram[0])
		{
			//says hello to server
			case 0:
				helloserver();
				break;
			//gets an ID from server
			case 1:
				val_set += datagram[5] << 24;
				val_set += datagram[6] << 16;
				val_set += datagram[7] << 8;
				val_set += datagram[8];
				setID(val_set);
				break;
			//touched
			case 2:
				sendDatagram(datagram);
				break;
			//animate
			case 3:
				break;
			//set color
			case 4:
				break;
			//heartbeat from server
			case 5:
				heartbeat(datagram);
				break;
			default:
				break;
		}
	}
	//we expect heartbeat every 1 minute - if it fails the bush closes the current connection, 
	//opens and new one and asks for a new ID.
	if (abs(cur_time - prv_time)>1000*60 && master_server != 0)
		if (abs(last_heart_beat - cur_time) > 1000*60)
		{
			close_connection();
			helloserver();
		}
}
