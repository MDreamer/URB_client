//opens a connection to the server
void connect_to_server()
{
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
//say hello to server for the first time or if the server changed
void helloserver()
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
//resets the module and tests if the module is ready
void resetModule()
{
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
}
// this function tries to connect to the WIFI - 
//it returns True if it succeeded or False if it didn't.
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
			Serial.write(datagram,16);
			Serial.println();
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
