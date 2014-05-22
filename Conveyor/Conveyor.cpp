// Conveyor.cpp : main project file.
// Daniel Tran 2011
// I know this is terrible code :P

#include "stdafx.h"
#include "softPLC.h"

using namespace System;
using namespace System::IO::Ports;
using namespace System::Threading;
using namespace System::Diagnostics;
using namespace System::Timers;

// scada register constants
const int sc_command  = 0x0;
const int sc_status   = 0x1;
const int sc_speed    = 0x2;
const int sc_response = 0x3;

const int sc_command_init         = 0x1;
const int sc_command_right_to_mid = 0x2;
const int sc_command_mid_to_left  = 0x3;
const int sc_command_left_to_mid  = 0x4;
const int sc_command_shutdown     = 0x5;
const int sc_command_stop         = 0x8;
const int sc_command_resume       = 0x9;

const int sc_status_offline = 0x1;
const int sc_status_init    = 0x2;
const int sc_status_online  = 0x3;
const int sc_status_active  = 0x4;
const int sc_status_stopped = 0x5;
const int sc_status_error   = 0x255;

const int sc_speed_0   = 0x1;
const int sc_speed_25  = 0x2;
const int sc_speed_50  = 0x3;
const int sc_speed_75  = 0x4;
const int sc_speed_100 = 0x5;

const int sc_response_completed = 0x1;

	
public ref class Conveyor
{
private:
	static SerialPort^ serialPort;
	static String^ indata = "";
	// by default the current command is set to 0 speed
    // (0% speed actually still moves the conveyor.. really really slowly.. we don't bother with timings for 0%
    // since it is so slow..
	static array<unsigned char>^ current_command = {0x02, 0x00, 0x85, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x81};
	static array<unsigned char>^ speed = {0x20, 0x00};
	static int speedPercent = 100;
	static int direction = 0;
	static System::Threading::Thread^ thread;
    static String^ status = "offline";
	

	static void DataReceivedHandler(Object^ sender, SerialDataReceivedEventArgs^ e)
	{
		SerialPort^ sp = (SerialPort^)sender;
		
		indata = sp->ReadExisting();
	}

public:
	static softPLC *plc;
	static unsigned char generateBCC(array<unsigned char>^ packet)
	{
		unsigned char bcc;
		
		for each (char byte in packet)
		{
			bcc ^= byte;
		}

		return bcc;
	}

	static void initialise()
	{
        // refer to SEW documentation for packet structure information

		// activate factory setting
		sendCommand(gcnew array<unsigned char> {0x02, 0x00, 0x86, 0x33, 0x00, 0x21, 0x92, 0x00, 0x00, 0x00, 0x01, 0x05}, 12);

		// set setpoint source to RS485
		sendCommand(gcnew array<unsigned char> {0x02, 0x00, 0x86, 0x33, 0x00, 0x21, 0x0D, 0x00, 0x00, 0x00, 0x02, 0x99}, 12);

		// set control source to RS485
		sendCommand(gcnew array<unsigned char> {0x02, 0x00, 0x86, 0x33, 0x00, 0x21, 0x0E, 0x00, 0x00, 0x00, 0x01, 0x99}, 12);
		
        /* more positioning code
		// set PO1 setpoint - control word 2
		sendCommand(gcnew array<unsigned char> {0x02, 0x00, 0x86, 0x33, 0x00, 0x21, 0x70, 0x00, 0x00, 0x00, 0x0A, 0xEC}, 12);
		
		// set PO2 setpoint - position hi
		sendCommand(gcnew array<unsigned char> {0x02, 0x00, 0x86, 0x33, 0x00, 0x21, 0x71, 0x00, 0x00, 0x00, 0x04, 0xE3}, 12);

		// set PO3 setpoint - position lo
		sendCommand(gcnew array<unsigned char> {0x02, 0x00, 0x86, 0x33, 0x00, 0x21, 0x72, 0x00, 0x00, 0x00, 0x03, 0xE7}, 12);

		// set reference travel type
		sendCommand(gcnew array<unsigned char> {0x02, 0x00, 0x86, 0x33, 0x00, 0x21, 0xB2, 0x00, 0x00, 0x00, 0x03, 0x27}, 12);
		*/

		// set PO1 setpoint - control word 1
		sendCommand(gcnew array<unsigned char> {0x02, 0x00, 0x86, 0x33, 0x00, 0x21, 0x70, 0x00, 0x00, 0x00, 0x09, 0xEF}, 12);
		
		// set PO2 setpoint - speed [%]
		sendCommand(gcnew array<unsigned char> {0x02, 0x00, 0x86, 0x33, 0x00, 0x21, 0x71, 0x00, 0x00, 0x00, 0x0B, 0xEC}, 12);

		// set PO3 setpoint - no function
		sendCommand(gcnew array<unsigned char> {0x02, 0x00, 0x86, 0x33, 0x00, 0x21, 0x72, 0x00, 0x00, 0x00, 0x00, 0xE4}, 12);

		// enable PO Data
		sendCommand(gcnew array<unsigned char> {0x02, 0x00, 0x86, 0x33, 0x00, 0x21, 0xAE, 0x00, 0x00, 0x00, 0x01, 0x39}, 12);
	}
	
	static void leftToMiddle()
	{
		Conveyor::plc->write16_register(sc_status, sc_status_active);
        Conveyor::setDirection(1);

		unsigned char bcc = generateBCC(gcnew array<unsigned char> {0x02, 0x00, 0x85, 0x00, 0x06, speed[0], speed[1], 0x00, 0x00});
		current_command = gcnew array<unsigned char> {0x02, 0x00, 0x85, 0x00, 0x06, speed[0], speed[1], 0x00, 0x00, bcc};

		thread = gcnew System::Threading::Thread( gcnew ThreadStart(&Conveyor::sendControlCommands));

		thread->Start();

		System::Threading::Thread::Sleep(calculateMoveDuration(2740));
		thread->Abort();
		Conveyor::plc->write16_register(sc_status, sc_status_online);
		Conveyor::plc->write16_register(sc_response, sc_response_completed);
	}
	
	static void middleToRight()
	{
		Conveyor::plc->write16_register(sc_status, sc_status_active);
        Conveyor::setDirection(1);
		
		unsigned char bcc = generateBCC(gcnew array<unsigned char> {0x02, 0x00, 0x85, 0x00, 0x06, speed[0], speed[1], 0x00, 0x00});
		current_command = gcnew array<unsigned char> {0x02, 0x00, 0x85, 0x00, 0x06, speed[0], speed[1], 0x00, 0x00, bcc};

		thread = gcnew System::Threading::Thread( gcnew ThreadStart(&Conveyor::sendControlCommands));

		thread->Start();

		System::Threading::Thread::Sleep(calculateMoveDuration(2150));
		thread->Abort();
		Conveyor::plc->write16_register(sc_status, sc_status_online);
		Conveyor::plc->write16_register(sc_response, sc_response_completed);
	}

	static void rightToMiddle()
	{
		//Conveyor::plc->write16_register(sc_status, sc_status_active);
  //      // align the box
  //      Conveyor::setDirection(1);

		unsigned char bcc = generateBCC(gcnew array<unsigned char> {0x02, 0x00, 0x85, 0x00, 0x06, speed[0], speed[1], 0x00, 0x00});
		/*current_command = gcnew array<unsigned char> {0x02, 0x00, 0x85, 0x00, 0x06, speed[0], speed[1], 0x00, 0x00, bcc};

		thread = gcnew System::Threading::Thread( gcnew ThreadStart(&Conveyor::sendControlCommands));

		thread->Start();

		System::Threading::Thread::Sleep(calculateMoveDuration(1500));
		thread->Abort();*/

        //

        Conveyor::setDirection(0);

		bcc = generateBCC(gcnew array<unsigned char> {0x02, 0x00, 0x85, 0x00, 0x06, speed[0], speed[1], 0x00, 0x00});
		current_command = gcnew array<unsigned char> {0x02, 0x00, 0x85, 0x00, 0x06, speed[0], speed[1], 0x00, 0x00, bcc};

		thread = gcnew System::Threading::Thread( gcnew ThreadStart(&Conveyor::sendControlCommands));

		thread->Start();

		//System::Threading::Thread::Sleep(calculateMoveDuration(2150));
        System::Threading::Thread::Sleep(calculateMoveDuration(3150));
		thread->Abort();
		Conveyor::plc->write16_register(sc_status, sc_status_online);
		Conveyor::plc->write16_register(sc_response, sc_response_completed);
	}

	static void middleToLeft()
	{
		Conveyor::plc->write16_register(sc_status, sc_status_active);
		Conveyor::setDirection(0);

		unsigned char bcc = generateBCC(gcnew array<unsigned char> {0x02, 0x00, 0x85, 0x00, 0x06, speed[0], speed[1], 0x00, 0x00});
		current_command = gcnew array<unsigned char> {0x02, 0x00, 0x85, 0x00, 0x06, speed[0], speed[1], 0x00, 0x00, bcc};

		thread = gcnew System::Threading::Thread( gcnew ThreadStart(&Conveyor::sendControlCommands));

		thread->Start();
        //System::Threading::Thread::Sleep(calculateMoveDuration(2540));
        
        System::Threading::Thread::Sleep(calculateMoveDuration(3340));

		thread->Abort();
		Conveyor::plc->write16_register(sc_status, sc_status_online);
		Conveyor::plc->write16_register(sc_response, sc_response_completed);
	}

	static void move()
	{ 
		unsigned char bcc = generateBCC(gcnew array<unsigned char> {0x02, 0x00, 0x85, 0x00, 0x06, speed[0], speed[1], 0x00, 0x00});
		current_command = gcnew array<unsigned char> {0x02, 0x00, 0x85, 0x00, 0x06, speed[0], speed[1], 0x00, 0x00, bcc};

		thread = gcnew System::Threading::Thread( gcnew ThreadStart(&Conveyor::sendControlCommands));

		thread->Start();
	}

	static int calculateMoveDuration(int speedDuration)
	{
		int duration = 0;
		int maxSpeedDuration = speedDuration;//2410; //2700;
		
		switch (speedPercent) {
			case 25:
				duration = maxSpeedDuration * 4;
				break;
				
			case 50:
				duration = maxSpeedDuration * 2;
				break;
				
			case 75:
				duration = maxSpeedDuration * 1.5;
				break;
				
			case 100:
				duration = maxSpeedDuration;
				break;

			default:
				duration = 0;
				break;
		}
		
		return duration;
	}
	
	static void setDirection(int newDirection)
	{
		// direction setting can be improved.. probably minus 2^16 and add 2^16 or change..
		direction = newDirection;
        setSpeed(speedPercent);
	}

	static void setSpeed(int newSpeedPercent)
	{
        speedPercent = newSpeedPercent;
		if (direction == 0)
		{
		
			switch (newSpeedPercent)
			{
                case 0:
                    speed[0] = 0x00;
					speed[1] = 0x00;
                    break;
				case 25:
					speed[0] = 0x08;
					speed[1] = 0x00;
					break;

				case 50:
					speed[0] = 0x10;
					speed[1] = 0x00;
					break;

				case 75:
					speed[0] = 0x18;
					speed[1] = 0x00;
					break;

				case 100:
					speed[0] = 0x20;
					speed[1] = 0x00;
					break;
			}
		} else {
			switch (newSpeedPercent)
			{
                case 0:
                    speed[0] = 0x00;
					speed[1] = 0x00;
                    break;
				case 25:
					speed[0] = 0xF8;
					speed[1] = 0x00;
					break;

				case 50:
					speed[0] = 0xF0;
					speed[1] = 0x00;
					break;

				case 75:
					speed[0] = 0xE8;
					speed[1] = 0x00;
					break;

				case 100:
					speed[0] = 0xE0;
					speed[1] = 0x00;
					break;
			}
		}
	}

	static String^ getStatus()
	{
		return status;
	}

	static int getSpeed()
	{
		return speedPercent;
	}

	static bool stop()
	{
        thread->Suspend();
    
		Conveyor::plc->write16_register(sc_status, sc_status_stopped);
		return true;
	}

    static bool resume()
    {
        try
        {
            thread->Resume();
        }
        catch (Exception^ e)
        {
            Console::WriteLine(e);
        }

        return true;
    }

	static void sendCommand(array<unsigned char>^ input, int length)
	{
		serialPort->Write(input, 0, length);
		serialPort->DataReceived += gcnew SerialDataReceivedEventHandler(DataReceivedHandler);

		System::Threading::Thread::Sleep(200);
	}

	static void sendControlCommands()
	{
		while (1)
		{
            // packets need to sent the whole time during a move command
			sendCommand(current_command, 10);
		}
	}

	static bool start(System::String ^portName)
	{  
		// Initialise the serial port
		
        serialPort = gcnew SerialPort();
		serialPort->PortName = portName;
		serialPort->BaudRate = 57600; // 9600 does not work for some reason??
		serialPort->Parity = (Parity)Enum::Parse(Parity::typeid, "Even");
		serialPort->DataBits = 8;
		serialPort->StopBits = StopBits::One;
		serialPort->Handshake = (Handshake)Enum::Parse(Handshake::typeid, "None");
		serialPort->ReadTimeout = 50;

		serialPort->Open();

        initialise();
        

        status = "initialised";
                
		return true;
	}

    static bool Conveyor::shutdown()
    {
        try
        {
            serialPort->Close();
            status = "offline";
        }
        catch (Exception^ e)
        {
            Console::WriteLine(e);
        }
        return true;
    }

	static void Conveyor::initialisePLC()
	{
		plc = new softPLC("192.168.1.150", 5);
		plc->start();
	}
};

void determineSpeed(int speed)
{
    switch (speed)
    {
        case sc_speed_0:
            Conveyor::setSpeed(0);
            break;

        case sc_speed_25:
            Conveyor::setSpeed(25);
            break;

        case sc_speed_50:
            Conveyor::setSpeed(50);
            break;

        case sc_speed_75:
            Conveyor::setSpeed(75);
            break;

        case sc_speed_100:
            Conveyor::setSpeed(100);
            break;
    }
}

int main()
{
	/*plc = new softPLC("127.0.0.1", 5);
	plc->start();*/
	Conveyor::initialisePLC();

    System::Threading::Thread::Sleep(100);
	
    System::Threading::Thread^ commandThread;
      
    Conveyor::start("COM6");
	while(1)
	{
		Console::WriteLine("Enter command: ");
		String^ input = Console::ReadLine();
	
		if (input == "stop") {
			Conveyor::stop();
			//Conveyor::plc->write16_register(sc_status, sc_status_stopped);
        } 
        else if (input == "move")
        {
			Conveyor::move();
		}
        else if (input == "resume")
        {
			Conveyor::resume();

		}
        else if (input == "speed 1")
        {
			Conveyor::setSpeed(25);
			Conveyor::plc->write16_register(sc_speed, sc_speed_25);
			Conveyor::move();

				
		}
        else if (input == "speed 2")
        {
			Conveyor::setSpeed(50);
			Conveyor::plc->write16_register(sc_speed, sc_speed_50);
			Conveyor::move();

				
		}
        else if (input == "speed 3")
        {
			Conveyor::setSpeed(75);
			Conveyor::plc->write16_register(sc_speed, sc_speed_75);
			Conveyor::move();

				
		}
        else if (input == "speed 4")
        {
			Conveyor::setSpeed(100);
			Conveyor::plc->write16_register(sc_speed, sc_speed_100);
			Conveyor::move();

				
		}
        else if (input == "direction 1")
        {
			Conveyor::setDirection(1);
			Conveyor::setSpeed(100);
			Conveyor::move();
				
		}
        else if (input == "direction 2")
		{
            Conveyor::setDirection(100);
			Conveyor::setSpeed(4);
			Conveyor::move();

		}
        else if (input == "left to middle")
        {
			//Conveyor::plc->write16_register(sc_status, sc_status_active);
			Conveyor::leftToMiddle();
			/*Conveyor::plc->write16_register(sc_status, sc_status_online);
			Conveyor::plc->write16_register(sc_response, sc_response_completed);*/

		}
        else if (input == "middle to right")
		{
			/*Conveyor::plc->write16_register(sc_status, sc_status_active);*/
            Conveyor::middleToRight();	
			/*Conveyor::plc->write16_register(sc_status, sc_status_online);
			Conveyor::plc->write16_register(sc_response, sc_response_completed);*/

		}
        else if (input == "right to middle")
		{
			//Conveyor::plc->write16_register(sc_status, sc_status_active);
            Conveyor::rightToMiddle();
			//Conveyor::plc->write16_register(sc_status, sc_status_online);
			//Conveyor::plc->write16_register(sc_response, sc_response_completed);

		}
        else if (input ==  "middle to left")
        {
			//Conveyor::plc->write16_register(sc_status, sc_status_active);
			Conveyor::middleToLeft();
			//Conveyor::plc->write16_register(sc_status, sc_status_online);
			//Conveyor::plc->write16_register(sc_response, sc_response_completed);

		}

	}
    
	Conveyor::plc->stop();
	
    return 0;
}
