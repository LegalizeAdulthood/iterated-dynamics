#!/bin/bash
# This script is for testing, but if you use both cygwin/gcc and MSDEV you 
# can build both environments using this script
#
# Run from the CppTestTools directory after unzipping
SUMMARY=$(pwd)/allTargets.tmp

echo "made these:" >${SUMMARY}

msMake() {
	echo "$(pwd) msdev *.dsw /make all" >>${SUMMARY}
	(msdev *.dsw /make all)
	echo "$(pwd) MS DONE"
}

gccMake() {
	echo "$(pwd) make all test" >>${SUMMARY}
	(make all test)
	echo "$(pwd) GCC DONE"
}

reportExists() {
  if [ -e ${1} ]
     then
       echo "${1} built OK" >>${SUMMARY}
     else
       echo "${1} missing" >>${SUMMARY}
  fi

}

report() {
  reportExists HomeGuardFitnesseServer/HomeGuardFitnesseServer.exe
  reportExists HomeGuardFitnesseServer/Debug/HomeGuardFitnesseServer.exe
}

gccMake
msMake
AllTests/Debug/AllTests.exe -r
echo "#!/bin/bash" >tmpscript.sh

pushd ../examples
  find . -maxdepth 1 -type d  | while read directory; \
  do ( if [ "$directory" != "." ]; then echo "pushd $directory; echo "$directory"; gccMake; msMake; report; popd;" >>tmpscript.sh; fi ); done

  # I had to make this loop create another script because msdev causes the loop to terminate for some unknown reason (to me)
  # I wasted a lot of time on the next line discoving that.
  # do ( if [ "$directory" != "." ]; then pushd $directory; echo "$directory"; gccMake; msMake; popd; fi ); done

  source tmpscript.sh
  rm tmpscript.sh

popd

cat ${SUMMARY}
rm ${SUMMARY}
