// Copyright (c) 2004 Micahel Feathers, James Grenning, Micah Martin, Robert Martin.
// Released under the terms of the GNU General Public License version 2 or later.

#include "UnitTestHarness/TestHarness.h"

#include "ClientSocket.h"
#include "ServerSocket.h"
#include "SocketException.h"
#include "Thread.h"
#include "Runnable.h"
#include <iostream>
#include <sstream>

using namespace std;

EXPORT_TEST_GROUP(Socket)

//-----------------------------------
//Common stuff for testing


class DoubleBouncer : public Runnable
  {
  public:
    DoubleBouncer(ServerSocket* s, int messageSize)
        : socket(s)
        , messageSize(messageSize)
        , done(false)
    {}

    ~DoubleBouncer()
    {}

    void run()
    {
        try
          {
            Socket * s = socket->Accept();
            string message;
            message = s->Receive();
            s->SendBytes(message+message);
            delete s;
          }
        catch (...)
          {
            cerr << "exception in thread" << endl << flush;
              }
          done = true;
    }

    bool isDone()
    {
      return done;
    }
  private:
    ServerSocket* socket;
    int messageSize;
    bool done;
  };


namespace
  {
  ServerSocket* server;
  ClientSocket* client;

  void SetUp()
  {
  	try 
  	{
    server = new ServerSocket(8882);
    client = new ClientSocket("localhost", 8882);
  	}
  	catch (SocketException& e)
  	{
  		cout << "Socket Exception: " << e.description() << "\n";
  		exit(1);
  	}
  }

  void TearDown()
  {
    delete client;
    delete server;
  }

}

//-----------------------------
//The tests

TEST(Socket, OutAndInInsameThread)
{
    client->SendBytes("Hello");
    string received;
    Socket* socket = server->Accept ();

    received = socket->Receive(5);
    CHECK(received.find("Hello") == 0);
    delete socket;
    
}

TEST(Socket, OutAndInInsameThreadStream)
{
    *client << "Hello";
    string received;
    Socket* socket = server->Accept ();

    *socket >> received;
    delete socket;
    
    CHECK(received.find("Hello") == 0);
    
}

TEST(Socket, BounceOffThread)
{
  string sent = "0123456789";
  DoubleBouncer* bouncer = new DoubleBouncer(server, sent.length());
  Thread bouncerThread(bouncer);
  bouncerThread.start();

  *client << sent;
  string received;
  *client >> received;
  CHECK(received.find(sent+sent) == 0);

  bouncerThread.join();

  CHECK(bouncer->isDone());
}

TEST(Socket, SendBytesAcceptReceiveTest)
{
  client->SendBytes("123456789");
  Socket* s = server->Accept();

  string received = s->Receive(5);
  CHECK(received.find("12345") == 0);

  received = s->Receive(4);
  CHECK(received.find("6789") == 0);

  delete s;
}

TEST(Socket, BounceStringOffThread)
{
  string sent = "0123456789";
  DoubleBouncer* bouncer = new DoubleBouncer(server, sent.length());
  Thread bouncerThread(bouncer);
  bouncerThread.start();

  client->SendBytes(sent);
  string received = client->Receive(2*sent.size());
  CHECK(received.find(sent) == 0);

  bouncerThread.join();

  CHECK(bouncer->isDone());
}


TEST(Socket, CreateDestroy)
{}

