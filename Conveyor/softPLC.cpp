/*
 * Copyright © 2011 Dean McNiven <dean.mcniven@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser Public License for more details.
 *
 * You should have received a copy of the GNU Lesser Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * This class extends and provides a C++ wrapper for:
 * 
 * libmodbus (Copyright © 2001-2008 Stéphane Raimbault <stephane.raimbault@gmail.com>)
 * http://libmodbus.org/
 *
 * and
 * 
 * libmodbus Win32 port (Copyright © thepyper)
 * https://code.launchpad.net/~thepyper/libmodbus/win32-native
 */

#include "softPLC.h"

/*
 * Program Constructor:
 * Accepts IP address to listen on, Number of registers
 * Starts service on DEFAULT port
 */
softPLC::softPLC(const char *ip_address, int nb_registers)
{
	int resp;

	// Initialise TCP Socket
	modbus_init_tcp(&mb_param, ip_address, MODBUS_TCP_DEFAULT_PORT);
	
	// Map registers into memory
	resp = modbus_mapping_new(&mb_mapping, 0, 0, nb_registers, 0);
    if (resp == FALSE) {
        printf("Memory allocation failed\n");
        exit(1);
    }
}

/*
 * Program Constructor:
 * Accepts IP address to listen on, Port number, Number of registers
 */
softPLC::softPLC(const char *ip_address, int port, int nb_registers)
{
	int resp;

	// Initialise TCP Socket
	modbus_init_tcp(&mb_param, ip_address, port);
	
	// Map registers into memory
	resp = modbus_mapping_new(&mb_mapping, 0, 0, nb_registers, 0);
    if (resp == FALSE) {
        printf("Memory allocation failed\n");
        exit(1);
    }
}


/*
 * Destructor:
 * Cleans/Frees memory on library termination
 */
softPLC::~softPLC()
{
	modbus_mapping_free(&mb_mapping);
    modbus_close(&mb_param);
}

/*
 * Thread Start:
 * This function spawns a new thread for the softPLC object and starts its own execution process
 */
void softPLC::run()
{
	while (1)
	{
		//Create Connection Handler
		con_manager = new handleCON(&mb_param, &mb_mapping);
		
		//Start new Thread
		con_manager->start();
		
		//Sleep(1000);
		
		while (!con_manager->inERROR())
		{
			//Do nothing
			Sleep(500);
		}

		//If we get here an ERROR was found!
		cout << "WARNING: problem in listener, Stopping Thread ...\n";
		con_manager->stop();

		//Wait for system to settle
		cout << "WARNING: Preparing to restart socket ...\n";
		Sleep(1000);

	}
}

/*
 * read_bit:
 * Reads the value of a single bit from a register
 */
bit_value_t softPLC::read_bit(int address, bit_num_t target_bit)
{
	return con_manager->read_bit(address, target_bit);
}

/*
 * read8_register:
 * Reads the lower 8 bits from a register (used for 8 bit operation) specified by address
 */
uint8_t softPLC::read8_register(int address)
{
	return con_manager->read8_register(address);
}

/*
 * read16_register:
 * Reads the entire 16 bit register specified by address
 */
uint16_t softPLC::read16_register(int address)
{
	return con_manager->read16_register(address);
}

/*
 * write_bit:
 * Writes a single bit in the specified register
 */
int softPLC::write_bit(int address, bit_num_t target_bit, bit_value_t state)
{
	return con_manager->write_bit(address, target_bit, state);
}

/*
 * write8_register:
 * Writes an 8 bit value to the lower half of the register specified by Address
 */
int softPLC::write8_register(int address, uint8_t data)
{
	//	cout << "softPLC: write Reg 8\n";
	return con_manager->write8_register(address, data);
}

/*
 * write16_register:
 * Writes a 16 bit value to the register specified by Address
 */
int softPLC::write16_register(int address, uint16_t data)
{
	return con_manager->write16_register(address, data);
}