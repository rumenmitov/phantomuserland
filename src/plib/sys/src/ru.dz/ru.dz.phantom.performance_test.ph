/**
 *
 * Phantom OS - Phantom language library
 *
 * This program is meant for measuring execution time of different programs
 * on Phantom OS. It focuses on comparing PVM vs. WAMR runtimes.
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
import .internal.time;
import .internal.connection;

class performance_test
{
    var wamr : .internal.wasm;
    var console : .internal.io.tty;
    var time : .internal.time;
    var sleep : .internal.connection;
    var start_time : .internal.long;
    var end_time : .internal.long;

    .internal.long sum_perf_test(var iter_count : .internal.object) 
    {
        var sum : .internal.long;
        var i : .internal.long;
        var iter_count_l : .internal.long;

        sum = (.internal.long) 0;
        i = (.internal.long) 0;
        iter_count_l = (.internal.long) iter_count;

        while(i < iter_count_l) {
            i = i + 1;
            sum = sum + i;
        }

        return sum;
    }

    .internal.long mem_perf_test(var iter_count : .internal.object, var cell_count : .internal.object) 
    {
        var iter_count_l : .internal.int;
        var cell_count_l : .internal.int;
        var i : .internal.int;
        var j : .internal.int;
        var result : .internal.long;
        var cells : .internal.object[];
        var tmp : .internal.object;

        iter_count_l = (.internal.int) iter_count;
        cell_count_l = (.internal.int) cell_count;
        cells = new .internal.object[]();
        result = 0;

        // allocation
        i = 0;
        tmp = (.internal.long) 0;
        while (i < cell_count_l) {
            cells[i] = tmp;
            i = i + 1;
        }

        // main loop
        i = 1;
        while (i <= iter_count_l) {
            j = 0;
            while (j < cell_count_l) {
                tmp = ((.internal.long) cells[j]) + 1;
                cells[j] = tmp;
                j = j + 1;
            }

            j = 0;
            while (j < cell_count_l) {
                if (((.internal.long) cells[j]) != i) {
                    throw "Error : cell value invalid!";
                }
                j = j + 1;
            }

            i = i + 1;
        }

        // result calculation
        j = 0;
        while (j < cell_count_l) {
            result = result + ((.internal.long) cells[j]);

            j = j + 1;
        }

        return result;
    }

    .internal.long report_result(var label : .internal.object, var result : .internal.object)
    {
        var delta : .internal.long;
        delta = ((.internal.long) end_time) - ((.internal.long) start_time); 

        console.putws((.internal.string) label);
        console.putws(": ");
        console.putws(result.toString());

        console.putws("; Duration: ");
        console.putws((delta).toString());
        console.putws(" ms\n");

        return delta;
    }

    .internal.long wasm_perf_test(var iter_count : .internal.object) {
        var temp : .internal.long;
        var args : .internal.object[];
        var tmp : .internal.object;

        args = new .internal.object[]();
        tmp = (.internal.long) iter_count;
        args[0] = tmp;

        start_time = time.uptime();
        temp = wamr.invokeWasm("sum_perf_test", args);
        end_time = time.uptime();

        return temp;
    }

    .internal.long wasm_mem_perf_test(var iter_count : .internal.object) {
        var temp : .internal.long;
        var args : .internal.object[];
        var tmp : .internal.object;

        args = new .internal.object[]();
        tmp = (.internal.int) iter_count;
        args[0] = tmp;

        start_time = time.uptime();
        temp = wamr.invokeWasm("memory_perf_test", args);
        end_time = time.uptime();

        return temp;
    }

    .internal.long pvm_perf_test(var iter_count : .internal.object) {
        var temp : .internal.long;

        start_time = time.uptime();
        temp = sum_perf_test((.internal.long) iter_count);
        end_time = time.uptime();

        return temp;
    }

    .internal.long pvm_mem_perf_test(var iter_count : .internal.object, var cell_count : .internal.object) {
        var temp : .internal.long;

        start_time = time.uptime();
        temp = mem_perf_test(iter_count, cell_count);
        end_time = time.uptime();

        return temp;
    }

    void run(var cns : .internal.io.tty)
    {
        var result_w : .internal.object;
        var result_p : .internal.object;
        var perf_param : .internal.long;
        var counter : .internal.int;

        cns.putws("Initializing performance test...\n");

        console = cns;
        wamr = new .internal.wasm();
        time = new .internal.time();
        sleep = new .internal.connection();

        sleep.connect("tmr:");
        wamr.loadModule(getWasmBinary());

        perf_param = (.internal.long) 100000000;

        console.putws("Initialization complete, starting in 1...\n");
        sleep.block(null, 1000);

        counter = 10;
        while (counter > 0) {
            counter = counter - 1;
            report_result("WASM_perf_test", wasm_perf_test(perf_param));
        }

        counter = 10;
        while (counter > 0) {
            counter = counter - 1;
            report_result("PVM_perf_test", pvm_perf_test(perf_param));
        }

        counter = 10;
        while (counter > 0) {
            counter = counter - 1;
            report_result("WASM_mem_test", wasm_mem_perf_test(4000));
        }

        counter = 10;
        while (counter > 0) {
            counter = counter - 1;
            report_result("PVM_mem_test", pvm_mem_perf_test(4000, 30000));
        }
    }

    .internal.string getWasmBinary()
    {
        return import "../resources/wasm/simple_functions.wasm" ;
    }
};

