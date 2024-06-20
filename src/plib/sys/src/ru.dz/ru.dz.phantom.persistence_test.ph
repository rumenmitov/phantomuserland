/**
 *
 * Phantom OS - Phantom language library
 *
**/

package .ru.dz.phantom;

import .phantom.os;
import .internal.io.tty;
import .internal.long;
import .internal.double;
import .internal.float;
import .internal.string;
import .internal.wasm;
import .ru.dz.demo.sensors_group;

class persistence_test
{
    var sg : .ru.dz.demo.sensors_group;
    var wamr : .internal.wasm;

    void run(var console : .internal.io.tty)
    {
        console.putws("Initializing persistence test...\n");

        var result : .internal.object;
        var args : .internal.object[];
        var tmp : .internal.object;

        args = new .internal.object[]();
        wamr = new .internal.wasm();
        sg = new .ru.dz.demo.sensors_group();

        wamr.loadModule(getWasmBinary());

        console.putws("Persistence test started!\n");

        // some hello worlds
        wamr.invokeWasm("hello_world", args);
        wamr.invokeWasm("phantom_hello_world", args);

        tmp = (.internal.long) 1;
        args[0] = tmp;

        while (1) {
            result = wamr.invokeWasm("sum_perf_test", args);

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

            console.putws("Updating data\n");
            sg.update();

            console.putws("Veryfing data\n");
            result = sg.verify();

            if (result) {
                console.putws("Ok!\n");
                console.putws("Current result: ");
                var sample_res : .internal.long;
                sample_res = sg.sample();
                console.putws(sample_res.toString());
                console.putws("\n");
            } else {
                console.putws("Failed!\n");
            }
        }
    }

    .internal.string getWasmBinary()
    {
        return import "../resources/wasm/simple_functions.wasm" ;
    }
};

