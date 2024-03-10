package .ru.dz.demo;
import .internal.long;

class sensor
{
    var times_updated : .internal.long;

    void update(){
        times_updated = times_updated + 1;
    }

    .internal.long getData()
    {
        return times_updated;
    }

    void sensor()
    {
        times_updated = 0;
    }
};