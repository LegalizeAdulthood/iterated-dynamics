#!/bin/sh +x

if [ ! -d /usr/local/bin ]
  then
    mkdir -p /usr/local/bin
fi

rm -f /usr/local/bin/NewClass

rm -f /usr/local/bin/NewInterface

rm -f /usr/local/bin/NewCModule

rm -f /usr/local/bin/NewVCModule

ln -s  $CPP_TEST_TOOLS/CppSourceTemplates/NewClass.sh /usr/local/bin/NewClass

ln -s  $CPP_TEST_TOOLS/CppSourceTemplates/NewInterface.sh /usr/local/bin/NewInterface

ln -s  $CPP_TEST_TOOLS/CppSourceTemplates/NewCModule.sh /usr/local/bin/NewCModule

ln -s  $CPP_TEST_TOOLS/CppSourceTemplates/NewVCModule.sh /usr/local/bin/NewVCModule
