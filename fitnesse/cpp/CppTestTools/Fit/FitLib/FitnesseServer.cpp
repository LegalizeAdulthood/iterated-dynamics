// Copyright (c) 2004 Michael Feathers, James Grenning, Micah Martin, Robert Martin.
// Released under the terms of the GNU General Public License version 2 or later.

#include "Platform.h"
#include <iostream>
#include <sstream>
#include <iomanip>

#include "Parse.h"
#include "Fixture.h"
#include "FixtureMaker.h"
#include "Concurrency/ClientSocket.h"
#include "FitnesseServer.h"

using namespace std;

FitnesseServer::FitnesseServer(string host, int port, int socketTicket, FixtureMaker* maker)
    : host(host)
    , port(port)
    , socketTicket(socketTicket)
    , failures(0)
    , maker(maker)
{
  socket = 0;
}

FitnesseServer::~FitnesseServer()
{
  if (socket)
    delete socket;
}

int FitnesseServer::Main(int ac, char **av, FixtureMaker* maker)
{

  if (ac != 4)
    {
      cerr << "usage: FitServer host port socketTicket\n";
      return -1;
    }


  string host(av[1]);
  int port = atoi(av[2]);
  int socketTicket = atoi(av[3]);

  FitnesseServer server(host, port, socketTicket, maker);
  return server.run();
}

//-------------------
// This is the main function.  It connects to Fitnesse and then
// acts as a conduit from FitNesse to Fit and back.
// it returns the number of test failures, or -1 if it fails
// to connect.
//
int FitnesseServer::run()
{
  failures = 0;
  if (connect())
    {
      string inputString = readFromFitnesse();
      for (; inputString.size() != 0; inputString = readFromFitnesse())
        {
          Fixture fixture;
          string out = doFixture(inputString, fixture);
          failures += (fixture.wrongs + fixture.exceptions);
          reportResultsToFitnesse(fixture, out);
        }
      return failures;
    }
  return -1;
}

int FitnesseServer::getFailures()
{
  return failures;
}

string FitnesseServer::doFixture(const string& in, Fixture& fixture)
{
  string out;
  try
    {
      Parse p(in);
      fixture.setMaker(maker.get());
      fixture.doTables(&p);
      p.print(out);
    }
  catch (exception& e)
    {
      out = "Unexpected Exception: ";
      out += e.what();
      fixture.exceptions++;
    }
  return out;
}

void FitnesseServer::reportResultsToFitnesse(Fixture& fixture, const string& out)
{
  writeSizeToFitnesse(out.length());
  writeToFitnesse(out);
  writeSizeToFitnesse(0);
  writeSizeToFitnesse(fixture.rights);
  writeSizeToFitnesse(fixture.wrongs);
  writeSizeToFitnesse(fixture.ignores);
  writeSizeToFitnesse(fixture.exceptions);
}

int FitnesseServer::toInt(const string& s)
{
  return atoi(s.c_str());
}

int FitnesseServer::readSizeFromFitnesse()
{
  string s = socket->Receive(10);
  return toInt(s);
}

string FitnesseServer::readFromFitnesse()
{
  string buffer = socket->Receive(readSizeFromFitnesse());
  return buffer;
}

void FitnesseServer::writeSizeToFitnesse(int size)
{
  ostringstream sizeStream;
  sizeStream << setfill('0') << setw(10) << size;
  socket->SendBytes(sizeStream.str());
}

void FitnesseServer::writeToFitnesse(const string& value)
{
  socket->SendBytes(value);
}


bool FitnesseServer::connect()
{
  string request = makeRequest();

  socket = new ClientSocket(host, port);
  socket->SendBytes(request);
  int size = readSizeFromFitnesse();
  if (size == 0) // Zero is the "hello" message from FitNesse
    return true;
  else
    {
      string message = socket->Receive(size);
      cout << "Connection failed: " << message << endl;
      return false;
    }
}

string FitnesseServer::makeRequest()
{
  ostringstream request;
  request << "GET /?responder=socketCatcher&ticket=" << socketTicket << " HTTP/1.1\r\n\r\n";
  return request.str();
}

