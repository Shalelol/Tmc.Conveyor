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

#include "handleCON.h"

/*
 * Program Constructor:
 * Accepts IP address to listen on, Number of registers
 * Starts service on DEFAULT port
 */
handleCON::handleCON(modbus_param_t *params, modbus_mapping_t *mapping)
{
	mb_param_session = params;
    mb_mapping_session = mapping;
	sktERROR = false;
}


/*
 * Destructor:
 * Cleans/Frees memory on library termination
 */
handleCON::~handleCON()
{
	free(&socket);
}

/*
 * Thread Start:
 * This function spawns a new thread for the softPLC object and starts its own execution process
 */
void handleCON::run()
{
	while (1)
	{
		// Listen for a connection
		socket = modbus_init_listen_tcp(mb_param_session);
		
		while (1) {
			uint8_t query[MAX_MESSAGE_LENGTH];
			int query_size;
		    
			// Get Query
			ret = modbus_listen(mb_param_session, query, &query_size);
			if (ret == 0) {
				//Respond to query
			    modbus_manage_query(mb_param_session, query, query_size, mb_mapping_session);
			} else if (ret == CONNECTION_CLOSED) {
			    //Connection closed by the client, end of server
				break;
			} else {
				printf("Error in modbus_listen (%d)\n", ret);
				sktERROR = true;
			}
		}
		
		// Close socket - Prep to be reopened
		if (socket) closesocket(socket);
	}
}

/*
 * read_bit:
 * Reads the value of a single bit from a register
 */
bit_value_t handleCON::read_bit(int address, bit_num_t target_bit)
{
	if (address > mb_mapping_session->nb_holding_registers) {
		cout << "ERROR: Provided Register Address (" << address << ") is Outside Bounds!\n";
		return bERROR;
	} else {
		uint16_t mask = getMask(target_bit, true);

		if ( (mb_mapping_session->tab_holding_registers[address] & mask) > 0 ) return bHIGH;
		else return bLOW;
	}
}

/*
 * read8_register:
 * Reads the lower 8 bits from a register (used for 8 bit operation) specified by address
 */
uint8_t handleCON::read8_register(int address)
{
	uint8_t val;

	if (address > mb_mapping_session->nb_holding_registers) {
		cout << "ERROR: Provided Register Address (" << address << ") is Outside Bounds!\n";
		return 0;
	} else {
		val = mb_mapping_session->tab_holding_registers[address] & 0xFF;
		return val;
	}
}

/*
 * read16_register:
 * Reads the entire 16 bit register specified by address
 */
uint16_t handleCON::read16_register(int address)
{
	uint16_t val;

	if (address > mb_mapping_session->nb_holding_registers) {
		cout << "ERROR: Provided Register Address (" << address << ") is Outside Bounds!\n";
		return 0;
	} else {
		val = mb_mapping_session->tab_holding_registers[address];
		return val;
	}
}

/*
 * write_bit:
 * Writes a single bit in the specified register
 */
int handleCON::write_bit(int address, bit_num_t target_bit, bit_value_t state)
{
	if (address > mb_mapping_session->nb_holding_registers) {
		cout << "ERROR: Provided Register Address (" << address << ") is Outside Bounds!\n";
		return -1;
	} else {
		if (state == bHIGH) {
			uint16_t mask = getMask(target_bit, true);
			uint16_t buffer = mb_mapping_session->tab_holding_registers[address] | mask;
			return write16_register(address, buffer);
		} else if (state == bLOW) {
			uint16_t mask = getMask(target_bit, false);
			uint16_t buffer = mb_mapping_session->tab_holding_registers[address] & mask;
			return write16_register(address, buffer);
		} else return -1;
	}
}

/*
 * write8_register:
 * Writes an 8 bit value to the lower half of the register specified by Address
 */
int handleCON::write8_register(int address, uint8_t data)
{
		//cout << "handleCON: write Reg 8\n";
	uint16_t buf;

	if (address > mb_mapping_session->nb_holding_registers) {
		cout << "ERROR: Provided Register Address (" << address << ") is Outside Bounds!\n";
		return -1;
	} else {
		buf = 0x00 + data;
		//mb_mapping.tab_holding_registers[address] = buf;
		//return 1;
		return write16_register(address, buf);
	}
}

/*
 * write16_register:
 * Writes a 16 bit value to the register specified by Address
 */
int handleCON::write16_register(int address, uint16_t data)
{
	if (address > mb_mapping_session->nb_holding_registers) {
		cout << "ERROR: Provided Register Address (" << address << ") is Outside Bounds!\n";
		return -1;
	} else {
		mb_mapping_session->tab_holding_registers[address] = data;
		return 1;
	}
}

/*
 * inERROR:
 * Public function to alert parent to terminate
 */
bool handleCON::inERROR()
{
	return sktERROR;
}

/*
 * getMask:
 * Private member function to generate a mask corresponding
 * to the specified Bit Position
 */
uint16_t handleCON::getMask(bit_num_t target_bit, bool inverted)
{
	uint16_t mask;

		switch (target_bit)
		{
		case B15:
			mask = 0x7FFF;
			break;
		case B14:
			mask = 0xBFFF;
			break;
		case B13:
			mask = 0xDFFF;
			break;
		case B12:
			mask = 0xEFFF;
			break;
		case B11:
			mask = 0xF7FF;
			break;
		case B10:
			mask = 0xFBFF;
			break;
		case B9:
			mask = 0xFDFF;
			break;
		case B8:
			mask = 0xFEFF;
			break;
		case B7:
			mask = 0xFF7F;
			break;
		case B6:
			mask = 0xFFBF;
			break;
		case B5:
			mask = 0xFFDF;
			break;
		case B4:
			mask = 0xFFEF;
			break;
		case B3:
			mask = 0xFFF7;
			break;
		case B2:
			mask = 0xFFFB;
			break;
		case B1:
			mask = 0xFFFD;
			break;
		case B0:
			mask = 0xFFFE;
			break;
		}

		if (inverted == true) return (mask ^ 0xFFFF);
		else return mask;
}