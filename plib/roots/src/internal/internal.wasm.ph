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
	// Load a module from a string containing Wasm code
	void 				loadModule(var module : .internal.string) [8] { }
	// Invoke a function with a given name from the module loaded using loadModule().
	// 		`args` : an array of objects which represent the function parameters
	// 		returns an object containing return value of Wasm function call
	.internal.object 	invokeWasm(var funcname : .internal.string, var args : .internal.object) [9] {  }
};
