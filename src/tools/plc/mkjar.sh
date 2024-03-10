#!/bin/sh

cd bin
jar cvfe ../../../build/jar/plc.jar ru.dz.plc.PlcMain .

jar xvf ../lib/soot-2.5.0.jar 
jar cvfe ../../../build/jar/jpc.jar ru.dz.soot.SootMain .

# # cd ../lib
# # jar uvfe ../../../build/jar/jpc.jar ru.dz.soot.SootMain ../lib/soot-2.5.0.jar 



#cd bin
#jar cfe ../../../build/jar/plc.jar ru.dz.plc.PlcMain .

#jar xf ../lib/soot-2.5.0.jar 
#jar cfe ../../../build/jar/jpc.jar ru.dz.soot.SootMain .

#jar cvfe ../../../build/jar/plc.jar ru.dz.plc.PlcMain .

#jar xvf ../lib/soot-2.5.0.jar 
#jar cvfe ../../../build/jar/jpc.jar ru.dz.soot.SootMain .


#cd ../lib
#jar uvfe ../../../build/jar/jpc.jar ru.dz.soot.SootMain ../lib/soot-2.5.0.jar 

# cd bin
# jar cfe ../../../build/jar/plc.jar ru.dz.plc.PlcMain .

# jar xf ../lib/soot-2.5.0.jar
# jar cfe ../../../build/jar/jpc.jar ru.dz.soot.SootMain .

