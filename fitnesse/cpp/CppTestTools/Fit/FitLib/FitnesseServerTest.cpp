// Copyright (c) 2004 Micahel Feathers, James Grenning, Micah Martin, Robert Martin.
// Released under the terms of the GNU General Public License version 2 or later.

#include "UnitTestHarness/TestHarness.h"
#include "Platform.h"
#include <sstream>
#include <iostream>
#include <iomanip>
#include "Concurrency/Runnable.h"
#include "Concurrency/Thread.h"
#include "Concurrency/Delay.h"
#include "FitnesseServer.h"
#include "FitFixtureMaker.h"
#include "Concurrency/ServerSocket.h"
#include "Helpers/SimpleStringExtensions.h"

using namespace std;

EXPORT_TEST_GROUP(FitnesseServer);



class FitnesseRunnable : public Runnable
  {
  public:
    FitnesseRunnable(FitnesseServer* s) : server(s)
    {}
    void run()
    {
      try
        {
          server->run();
        }
      catch (...)
        {
          cerr << "exception in thread" << endl << flush;
        }
    }
  private:
    FitnesseServer* server;
  };


//-----------------------------------
//Common stuff for testing
namespace
  {
  FitnesseServer* fitnesseServer = 0;
  Socket* fitnesseSocket = 0;
  int port = 8881;
  Thread* thread = 0;
  FixtureMaker* maker = 0;

  string toString(int i)
  {
    stringstream s;
    s << i;
    string result;
    s >> result;
    return result;
  }

  void SetUp()
  {
    fitnesseServer = 0;
    fitnesseSocket = 0;
    ServerSocket* server;

    try
      {
        server = new ServerSocket(port);
      }
    catch (const char* msg)
      {
        cout << "\nException in socket creation, port probably in use: "
        << port << "\nMessage: " << msg << '\n';
        exit(-1);
      }

    maker = new FitFixtureMaker();
    fitnesseServer = new FitnesseServer("localhost", port, 23, maker);
    thread = new Thread(new FitnesseRunnable(fitnesseServer));
    thread->start();
    fitnesseSocket = server->Accept();
    delete server;
  }


  string GetInitialMessageToFitnesse()
  {
    string request = fitnesseSocket->Receive(52);
    fitnesseSocket->SendBytes("0000000000");
    return request;
  }

  void TearDown()
  {
    thread->join();
    delete fitnesseServer;
    delete fitnesseSocket;
    delete thread;
    Delay::ms(100);  //I am not sure why this helps, but it prevents leak reports
  }

}

//-----------------------------
//The tests



TEST(FitNesseServer, SocketDrivenFitnesseServerTest)
{
  string received = GetInitialMessageToFitnesse();
  //                    1         2         3         4         5
  //           1234567890123456789012345678901234567890123456789012
  CHECK_EQUAL("GET /?responder=socketCatcher&ticket=23 HTTP/1.1\r\n\r\n", received);
  fitnesseSocket->SendBytes("0000000000");
}



TEST(FitNesseServer, OnePass)
{
  GetInitialMessageToFitnesse();

  string input = "0000000040"
                 "<table><tr><td>Fixture</td></tr></table>"
                 //         1         2         3         4
                 //1234567890123456789012345678901234567890
                 "0000000000";

  fitnesseSocket->SendBytes(input);
  string output = fitnesseSocket->Receive(10+40+50);
  int failures = fitnesseServer->getFailures();

  LONGS_EQUAL(0, failures);
  const string expected =
    input +
    "0000000000" //right
    "0000000000" //wrong
    "0000000000" //ignore
    "0000000000";//exceptions
  CHECK_EQUAL(expected, output);
  StringFrom(string("hey there"));

}

TEST(FitNesseServer, Error)
{
  GetInitialMessageToFitnesse();
  string input = "0000000020"  "012345678901234567890"  "0000000000";
  fitnesseSocket->SendBytes(input);
  string output = fitnesseSocket->Receive(10+43+50);
  //                    1         2         3         4
  //           1234567890123456789012345678901234567890123
  string expected =
    "0000000043Unexpected Exception: Can't find tag: table"
    "0000000000"
    "0000000000" //right
    "0000000000" //wrong
    "0000000000" //ignore
    "0000000001";//exceptions
  CHECK_EQUAL(expected, output);
  LONGS_EQUAL(1, fitnesseServer->getFailures());
}


TEST(FitNesseServer, TwoPass)
{
  GetInitialMessageToFitnesse();

  string input = "0000000040"
                 "<table><tr><td>Fixture</td></tr></table>"
                 "0000000040"
                 "<table><tr><td>Fixture</td></tr></table>"
                 "0000000000";

  fitnesseSocket->SendBytes(input);
  int failures = fitnesseServer->getFailures();
  string output = fitnesseSocket->Receive(2*(10+40+50));

  LONGS_EQUAL(0, failures);
  string expected =
    "0000000040<table><tr><td>Fixture</td></tr></table>"
    "0000000000"
    "0000000000" //right
    "0000000000" //wrong
    "0000000000" //ignore
    "0000000000"//exceptions
    "0000000040<table><tr><td>Fixture</td></tr></table>"
    "0000000000"
    "0000000000" //right
    "0000000000" //wrong
    "0000000000" //ignore
    "0000000000";//exceptions
  CHECK_EQUAL(expected, output);

}

TEST(FitNesseServer, BigMessage)
{
  GetInitialMessageToFitnesse();

  string input = "0000000400"
                 "<table><tr><td>Fixture</td></tr></table>"
                 "<table><tr><td>Fixture</td></tr></table>"
                 "<table><tr><td>Fixture</td></tr></table>"
                 "<table><tr><td>Fixture</td></tr></table>"
                 "<table><tr><td>Fixture</td></tr></table>"
                 "<table><tr><td>Fixture</td></tr></table>"
                 "<table><tr><td>Fixture</td></tr></table>"
                 "<table><tr><td>Fixture</td></tr></table>"
                 "<table><tr><td>Fixture</td></tr></table>"
                 "<table><tr><td>Fixture</td></tr></table>"
                 "0000000000";

  fitnesseSocket->SendBytes(input);
  int failures = fitnesseServer->getFailures();
  string output = fitnesseSocket->Receive(10+400+50);

  LONGS_EQUAL(0, failures);
  string expected =
    input +
    "0000000000" //right
    "0000000000" //wrong
    "0000000000" //ignore
    "0000000000";//exceptions
  CHECK_EQUAL(expected, output);

}


TEST(FitNesseServer, MakeSureEmbeddedNewLinesCountAs1)
{
  GetInitialMessageToFitnesse();
  //MS insists on sending \r\n for a \n for stdio.
  //Sockets don't have the same problem, but this test
  //makes sure

  string input = "0000000045"
                 "\n\n<table><tr><td>\n\nFixture</td>\n</tr></table>"
                 "0000000000";

  fitnesseSocket->SendBytes(input);
  int failures = fitnesseServer->getFailures();
  string output = fitnesseSocket->Receive(10+45+50);

  LONGS_EQUAL(0, failures);
  string expected =
    input +
    "0000000000" //right
    "0000000000" //wrong
    "0000000000" //ignore
    "0000000000";//exceptions
  CHECK_EQUAL(expected, output);
}
