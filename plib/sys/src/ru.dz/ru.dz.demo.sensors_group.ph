package .ru.dz.demo;

import .internal.long;
import .ru.dz.demo.sensor;

class sensors_group 
{
    var sensors : sensor[];
    var size : int;

    void sensors_group()
    {
        var i : int;
        size = 30000;
        sensors = new int[]();

        i = 0;
        while (i < size)
        {
            sensors[i] = new sensor();
            i = i + 1;
        }
    }

    void update()
    {
        var i : int;
        i = 0;
        while (i < size)
        {
            sensors[i].update();
            i = i + 1;
        }
    }

    int verify()
    {
        var i : int;
        var data : .internal.long;
        data = sensors[i].getData();
        i = 1;
        while (i < size)
        {
            if (sensors[i].getData() != data)
            {
                return 0;
            }
            i = i + 1;
        }
        return 1;
    }

    .internal.long sample()
    {
        var res : .internal.long;
        res = sensors[0].getData();
        return res;
    }

};