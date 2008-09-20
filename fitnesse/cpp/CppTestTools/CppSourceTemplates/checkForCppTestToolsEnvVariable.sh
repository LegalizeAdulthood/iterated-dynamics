#!/bin/sh
checkForCppTestToolsEnvVariable() {
	if [ -z "$CPP_TEST_TOOLS" ] ; then
	   echo "CPP_TEST_TOOLS not set"
	   exit 1
	fi
	if [ ! -d "$CPP_TEST_TOOLS" ] ; then
	   echo "CPP_TEST_TOOLS not set to a directory"
	   exit 2
	fi
}
