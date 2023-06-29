/**
 *
 * Phantom OS - Phantom language library
 *
 * Copyright (C) 2005-2019 Dmitry Zavalishin, dz@dz.ru
 *
 * Simple weater widget
 *
 *
**/

package .ru.dz.demo;

import .phantom.os;
import .internal.io.tty;
// import .internal.window;
import .internal.bitmap;
// import .internal.tcp;
// import .internal.connection;
import .internal.directory;
import .internal.long;
//import .ru.dz.phantom.system.runnable;

import .ru.dz.demo.sensor;

import .ru.dz.demo.sensors_group;

attribute const * ->!;

// graph color: 180 R, 205 G, 147 B
// ABGR hex = 0xFF93CDB4

class weather
{
    var i : int;
    var groupsCount : int;

    //var wbmp : .internal.bitmap[];
    // var sgList : sensors_group[];
    var sg : sensors_group;
    // var sleep : .internal.connection;


    void run(var console : .internal.io.tty)
    {

        var res : int;
        res = 0;

		console.putws("Initializing demo scenario\n");

        sg = new sensors_group();

		console.putws("Started demo scenario\n");

        while (1) 
        {
            console.putws("Updating data\n");

            sg.update();

            console.putws("Veryfing data\n");

            res = sg.verify();

            if (res) {
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
	
};

