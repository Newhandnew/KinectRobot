// SerialTest.cpp : Defines the entry point for the console application.
//

#include <iostream>
#include <windows.h>

#include "Serial.h"
#include "stdafx.h"

using namespace std;

#define RX_BUFFSIZE 20

void printUsage(_TCHAR progName[]);
void printPort(_TCHAR portName[]);


int _tmain(int argc, _TCHAR* argv[])
{
	if(argc != 2)
	{

		cout << "press any key and enter to quit" << endl;
		char temp;
		cin >> temp;

		return 10;
	}

		try
	{
		wcout << "open port at : " << argv[1] << endl;
		tstring commPortName(argv[1]);
		Serial serial(commPortName, 115200);

		serial.flush();
		char cBuff[10]="";
		char serialBuff[RX_BUFFSIZE]="";
		char hi[] = "Hello";

		//serial.write(hi);
		while (1) {
			do
			{
				serial.read(serialBuff, RX_BUFFSIZE);
				Sleep(100);
			} while (serialBuff[0] == '\0');
			cout << "serial port read: " << serialBuff << endl;
			serialBuff[0] = '\0';
			cin >> cBuff;
			serial.write(cBuff);
		}

	}catch(const char *msg)
	{
		cout << msg << endl;
	}

	//cout << "press any key and enter to quit" << endl;
	//char temp;
	//cin >> temp;

	return 0;
}

void printUsage(_TCHAR progName[])
{
#if defined(UNICODE)
	wcout << progName << " <comm port>" << endl
		 << "e.g., " << progName << " COM1" << endl;
#else
	cout << progName << " <comm port>" << endl
		 << "e.g., " << progName << " COM1" << endl;
#endif
	
}
