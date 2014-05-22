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


//Suppress warnings from modbus C library
#define _CRT_SECURE_NO_WARNINGS

#ifndef _HANDLE_CON_
#define _HANDLE_CON_

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
#include <Windows.h>

#include "Thread.cpp"
#include "modbus.h"

using namespace std;

//Enumerated Types
enum bit_value_t { bHIGH, bLOW, bERROR };
enum bit_num_t { B15, B14, B13, B12, B11, B10, B9, B8, B7, B6, B5, B4, B3, B2, B1, B0 };

class handleCON : public Thread {
public:
	// Constructur, Destructor and Thread start
	handleCON(modbus_param_t *params, modbus_mapping_t *mapping);
	~handleCON();
	void run();

	// Read value of specified bit
	bit_value_t read_bit(int address, bit_num_t target_bit);

	// Read 8 bit register (lower 8 bits)
	uint8_t read8_register(int address);

	// Read Entire 16 bit register
	uint16_t read16_register(int address);

	// Write and individual bit
	int write_bit(int address, bit_num_t target_bit, bit_value_t state);

	// Write 8 bit register (lower 8 bits)
	int write8_register(int address, uint8_t data);

	// Write entire 16 bit register
	int write16_register(int address, uint16_t data);

	//Error alerting
	bool inERROR();

// Private constructs and functions
private:
	int socket;
	bool sktERROR;
    modbus_param_t *mb_param_session;
    modbus_mapping_t *mb_mapping_session;
    int ret;

	uint16_t getMask(bit_num_t target_bit, bool inverted);
};

#endif