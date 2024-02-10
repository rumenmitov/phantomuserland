/**
 *
 * Phantom OS - Phantom language library
 *
**/

package .ru.dz.demo;

import .phantom.os;
import .internal.io.tty;
import .internal.long;
import .internal.double;
import .internal.float;
import .internal.string;
import .internal.wasm; // <- new

class wasm
{
    void run(var console : .internal.io.tty)
    {

        var res : int;
        res = 0;

	console.putws("Started demo WebAssembly scenario");

        var wamr : .internal.wasm;
        wamr = new .internal.wasm();
        wamr.loadModule(getWasmBinary());

        var args : .internal.object[];
        args = new .internal.object[]();
        var tmp : .internal.object;

        var result : .internal.object;

        wamr.invokeWasm("hello_world", (.internal.object) args);

        // while (1) {
        //         wamr.call(code, "hello_world", (.internal.object) args);
        // }

        tmp = 1000000000;
        args[0] = tmp;
        tmp = 100;
        args[1] = tmp;
        tmp = 0;
        args[2] = tmp;

        wamr.invokeWasm("print_prime_numbers", (.internal.object) args);

// 
        // var precision : int;
        // precision = 200000;
// 
        // while (1) 
        // {
        //     tmp = precision;
        //     args[0] = tmp;
// 
        //     console.putws("< START SYSCALL >");
// 
        //     result = wamr.call(code, "count_primes", (.internal.object) args);
// 
        //     console.putws("< SYSCALL END >");
// 
        //     precision = precision + 1;
        // }
    }

    .internal.string getWasmBinary()
    {
        return import "../resources/wasm/simple_functions.wasm" ;
    }
	
};

