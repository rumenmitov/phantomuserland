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
import .internal.wasm;
import .internal.window;
import .internal.bitmap;

class wasm
{
    void run(var console : .internal.io.tty)
    {
        console.putws("Started demo WebAssembly scenario\n");

        var wamr : .internal.wasm;
        var result : .internal.object;
        wamr = new .internal.wasm();

        wamr.loadModule(getWasiBinary());

        var strArgs : .internal.string[];
        strArgs = new .internal.string[]();
        // sample environment variables
        strArgs[0] = "DISPLAY=:0";
        strArgs[1] = "PATH=/opt/wasi-sdk/bin/";

        wamr.wasiSetEnvVariables(strArgs);

        strArgs[0] = "env_test"; // subprogram name
        result = wamr.wasiInvokeStart(strArgs);
        console.putws("WASI execution result (env. vars test): ");
        console.putws(result.toString());
        console.putws("\n");

        strArgs[0] = "argv_test"; // subprogram name
        strArgs[1] = "Hello,";
        strArgs[2] = "world!";

        result = wamr.wasiInvokeStart(strArgs); // args are ignored
        console.putws("WASI execution result (argc/argv test): ");
        console.putws(result.toString());
        console.putws("\n");

        wamr.loadModule(getWasmBinary());

        var args : .internal.object[];
        args = new .internal.object[]();
        var tmp : .internal.object;

        wamr.invokeWasm("hello_world", args);
        wamr.loadModule(getBufferBinary());

        // decode image + show in window demo
        tmp = wamr.shareObject(getJpeg());
        args[0] = tmp;
        wamr.invokeWasm("decode_show_image", args);

        // decode image and return result demo
        tmp = wamr.shareObject(getJpeg());
        args[0] = tmp;
        result = wamr.invokeWasm("decode_image", args);

        var bmp : .internal.bitmap;
        bmp = wamr.retreiveObject((.internal.long) result, /* remove = */ 1);

        // display decoded image
        var win : .internal.window;
        win = new .internal.window();
        win.setTitle("Image");
        win.drawImage( 0, 0, bmp );

        // ########################################
        // #########   Weather app demo   #########
        // ########################################

        wamr.loadModule(getWasmBinary());
        args = new .internal.object[]();
        tmp = 250;
        args[0] = tmp;
        wamr.invokeWasm("weather_demo", args);

        tmp = (.internal.long) 5000000;
        args[0] = tmp;
        tmp = 1;
        args[1] = tmp;
        tmp = (.internal.long) 0;
        args[2] = tmp;

        while (1) {
            result = wamr.invokeWasm("long_sum_test", args);

            console.putws("Result of WASM execution: `");
            console.putws(result.toString());
            console.putws("`\n");
            
            var temp : .internal.long;
            temp = (.internal.long)args[0];
            tmp = temp + 1;
            args[0] = tmp;

            console.putws("Next parameter: ");
            console.putws(tmp.toString());
            console.putws("\n");
        }

        tmp = 1000000000;
        args[0] = tmp;
        tmp = 100;
        args[1] = tmp;
        tmp = 0;
        args[2] = tmp;

        wamr.invokeWasm("print_prime_numbers", args);
    }

    .internal.string getWasmBinary()
    {
        return import "../resources/wasm/simple_functions.wasm" ;
    }
	
    .internal.string getWasiBinary()
    {
        return import "../resources/wasm/wasi_functions.wasm" ;
    }

    .internal.string getBufferBinary()
    {
        return import "../resources/wasm/img_decoder.wasm" ;
    }

    .internal.string getJpeg()
    {
        return import "../resources/test_images/cat.jpg" ;
    }
};

