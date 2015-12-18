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
int eeprom_addr = 0;
//datagram description:
//options 1 byte | source 4 bytes | destination 4 bytes | person_ID 1 byte or animation 1 byte or R+G+B (3bytes) | buffer | buffer | buffer | buffer
//options - hello = 0(to server), setID = 1(from server), touched = 2(to server), animate = 3(from server), 
//			setColor = 4(from server), heartbeat = 5(from server)
byte datagram[16];
long master_server;

int hearbeat_wd; //heartbeat watchdog

long cur_time = millis();
long prv_time = 0;

//this var is an indication the system got a heartbeat from master_server in the last minutes. if not its sends "Hello" to get an ID.
bool gotHeartBeat;

//this function send the data to the master server's IP
//TODO: - check if a X tries is needed before announcing "timeout" 
void sendDatagram(byte _datagram[16])
{
	String cmd;
	cmd = "AT+CIPSEND=4,9";
	dbgSerial.println(cmd);
	//check that the server is willing to get the datagram
	if(dbgSerial.find(">"))
	{
		if (debug)
		{
			Serial.print("sending data...");
			Serial.println(datagram);
		}
		dbgSerial.write(_datagram,16);
	}else
	{
		dbgSerial.println("AT+CIPCLOSE");
		if (debug)
			Serial.println("connect timeout");
		delay(1000);
		return;
	}
	delay(1000);
}
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
		dbgSerial.println("AT+CIPCLOSE");
		hello();
	
}
void hello()
{
	String conn_cmd;
	conn_cmd = "AT+CIPSTART=4,\"TCP\",\"";
	conn_cmd += DST_IP;
	conn_cmd += "\",2323";
	if (debug)
	Serial.println(conn_cmd);
	dbgSerial.println(conn_cmd);
	delay(1000);
	datagram[0] = 0;
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
	// Open serial communications and wait for port to open:
	//can't be faster than 19200 for softserial
	dbgSerial.begin(9600);
	if (debug)
		Serial.begin(9600);
		Serial.println("ESP8266");
	//test if the module is ready
	bool isReady = false;
	while (!isReady)
	{
		dbgSerial.println("AT+RST");
		delay(1000);
		if(dbgSerial.find("ready"))
		{
			if (debug)
				Serial.println("Module is ready");
			isReady = true;
		}
		else
		{
			if (debug)
				Serial.println("Module have no response.");
		}
		delay(1000);	
	}
	//connect to the wifi
	boolean connected=false;
	while (!connected)
		if(connectWiFi())
		{
			connected = true;
			break;
		}
	//print the ip addr
	dbgSerial.println("AT+CIFSR");
	Serial.println("ip address:");
	//this time frame is needed in order to let the ESP2866 time to 
	//AT-transmit the command - I guess its because the Arduino MCU is 
	//too fast for it regrading the commands.
	delay(10);
	while (dbgSerial.available())
	{
		Serial.write(dbgSerial.read());
	}
	//set the single connection mode
	dbgSerial.println("AT+CIPMUX=1");
	delay(10);
	String conn_cmd;
	conn_cmd = "AT+CIPSTART=4,\"TCP\",\"";
	conn_cmd += DST_IP;
	conn_cmd += "\",2323";
	if (debug)
		Serial.println(conn_cmd);
	dbgSerial.println(conn_cmd);
	delay(1000);
}
void loop()
{
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
			case 0:
				hello();
				break;
			case 1:
				long val=0;
				val += datagram[5] << 24;
				val += datagram[6] << 16;
				val += datagram[7] << 8;
				val += datagram[8];
				setID(val);
				break;
			//touched
			case 2:
			
				break;
			
			case 3:
			break;
			
			case 4:
			break;
			case 5:
				heartbeat(datagram);
			break;
			default:
			/* Your code here */
			break;
		}
	}
	
}
boolean connectWiFi()
{
	dbgSerial.println("AT+CWMODE=1");
	String cmd="AT+CWJAP=\"";
	cmd+=SSID;
	cmd+="\",\"";
	cmd+=PASS;
	cmd+="\"";
	Serial.println(cmd);
	dbgSerial.println(cmd);
	delay(5000);
	if(dbgSerial.find("OK"))
	{
		Serial.println("OK, Connected to WiFi.");
		return true;
	}else
	{
		Serial.println("Can not connect to the WiFi.");
		return false;
	}
}
