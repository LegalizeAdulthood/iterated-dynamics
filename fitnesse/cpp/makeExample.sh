#/bin/sh
LANGUAGE=${1:-Cpp}

export CPP_TEST_TOOLS=$(pwd)/CppTestTools

make -C CppTestTools clean depend all
make -C Example${LANGUAGE}Project clean depend all
echo "Run all the unit tests"
make -s -C CppTestTools/AllTests test
make -s -C Example${LANGUAGE}Project/AllTests test

