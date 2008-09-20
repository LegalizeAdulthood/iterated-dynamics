Precondition: FitNesse is installed and you can see the welcome page.

1. Unzip CppTestTools into your fitnesse directory, resulting in
		<somewhere>/fitnesse/cpp/CppTestTools/
		<somewhere>/fitnesse/cpp/ExampleCppProject
		<somewhere>/fitnesse/cpp/ExampleCProject
		<somewhere>/fitnesse/cpp/CppTestToolsPages/

2. Move 
    <somewhere>/fitnesse/cpp/CppTestToolsPages/
  to
    <somewhere>fitnesse/FitNesseRoot/CppTestToolsPages/

NOTE: unix-like systems can use symbolic links instead of copying in 1 and 2 above
  fitnesse/cpp -> the cpp directory from the zip file
  fitnesse/FitNesseRoot/CppTestToolsPages -> CppTestToolsPages

3. Put a wiki page link to CppTestToolsPages on your fitnesse FrontPage

4. Set the environement variable CPP_TEST_TOOLS to 
		<somewhere>/fitnesse/cpp/CppTestTools
		the makeExample.sh file will temporarily set this variable
		you will need it for VisualStudio, eclispe, and when using the 
		example makefiles outside of makeExample.sh 

NOTE: use only forward slashes, even for Windows and Visual Studio. 

5a. For unix/gcc and C++ example
 > cd <somewhere>/fitnesse/cpp
 > ./makeExample 

5b. For unix/gcc and  example
 > cd <somewhere>/fitnesse/cpp
 > ./makeExample C

5c. For Windows with Visual Studion
  double click CppTestTools.dsw, if you are using VS.NET, let VS.NET convert all
  Build the ExampleCppProject or ExampleCProject.

Unit tests should report OK (...)
NOTE: VC6 and older cygwin installs may report memory leaks.

6. To run the fitnesse tests
  Navigate to CppTestToolsPages.HomeGuardTests
  Edit the COMMAND_PATTERN as necessary for your directory structure
    (it should be correct if you put the used the
       suggested directory structure)
    The 
  The fitnesse tests are expected to fail (RED). 
  The tests are not expected to to have any fixture errors (YELLOW) 
