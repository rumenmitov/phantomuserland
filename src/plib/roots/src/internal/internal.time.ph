/**
 *
 * Phantom OS - Phantom language library
 *
 * Copyright (C) 2005-2013 Dmitry Zavalishin, dz@dz.ru
 *
 * Internal: yes
 * Preliminary: yes
 *
 *
**/

package .internal;

import .internal.long;

/**
 *
 * This class has internal implementation (as everything in
 * .internal package). It means that VM will never load its
 * bytecode, and internal version will be used instead. This
 * class definition must be synchronized with VM implementation.
 *
**/

class .internal.time
{

	// system uptime in microseconds
	.internal.long uptime() [9] {}

};



