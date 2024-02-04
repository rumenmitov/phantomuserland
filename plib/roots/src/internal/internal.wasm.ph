/**
 *
 * Phantom OS - Phantom language library
 *
 * Copyright (C) 2005-2019 Dmitry Zavalishin, dz@dz.ru ??????????
 *
 *	???????????????????
 * 
**/

package .internal;

/**
 *
 * This class has internal implementation (as everything in
 * .internal package). It means that VM will never load its
 * bytecode, and internal version will be used instead. This
 * class definition must be synchronized with VM implementation.
 *
**/

/**
 *
 *
**/

class .internal.wasm
{
	/*
	 * Call a wasm function named `funcname` from module `module`, and pass the array
	 * of arguments to the function. Return value from the function is one of 4 wasm
	 * fundamental types
	 */
	.internal.object  call(var module : .internal.string, var funcname : .internal.string, var args : .internal.object) [8] {  }
};
