#!/bin/sh
source checkForCppTestToolsEnvVariable.sh

checkForCppTestToolsEnvVariable

EXE_DIR=${EXE_DIR:-/usr/local/bin}
test -f ${EXE_DIR} || mkdir -p ${EXE_DIR}

for file in NewClass NewInterface NewCModule NewVCModule; do
   rm -f ${EXE_DIR}/${file}
   ln -s ${CPP_TEST_TOOLS}/CppSourceTemplates/${file}.sh ${EXE_DIR}/${file}
   chmod a+x ${EXE_DIR}/${file}
done
