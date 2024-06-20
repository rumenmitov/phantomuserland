/**
 *
 * Phantom OS - Phantom language library
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
 * NOTE: method signatures often use .internal.object instead of an array 
 *  type since PLC has problems compiling array arguments
 *
**/

class .internal.wasm
{
    // Load a module from a string containing Wasm code
    void                loadModule(var module : .internal.string) [8] { }

    // Invoke a function with a given name from the module loaded using loadModule().
    //     `args` : an array of objects - function parameters (of numerical types)
    //     returns an object containing return value of Wasm function call
    .internal.object    invokeWasm(var funcname : .internal.string, var args) [9] { }

    // Invokes a module as a WASI target: calls `_start` functions and sets CLI arguments as specified
    //     `argv` : an array of string arguments, that will be passed to program
    //     returns a null object unless program finished via `proc_exit(exitcode)`, in which case
    //     integer exitcode is returned 
    .internal.object    wasiInvokeStart(var argv) [10] { }

    // Sets environment variables to be used when invoking WASI-enabled Wasm program
    //      `envs` : an array of strings of "<key>=<value>" form
    void                wasiSetEnvVariables(var envs) [11] { }

    // add object to wasm pool and get the index 
    .internal.long      shareObject(var object) [12] { }

    // retreive the object from the wasm pool using its index, optionally remove it from the pool
    .internal.object    retreiveObject(var id : .internal.long, var remove : .internal.int) [13] { }

    // remove all objects from wasm object pool
    void                releaseObjects() [14] {}
};
