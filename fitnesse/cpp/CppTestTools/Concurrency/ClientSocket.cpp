// Implementation of the ClientSocket class

#include "ClientSocket.h"
#include "SocketInnards.h"
#include "SocketException.h"
#include <errno.h>


ClientSocket::~ClientSocket ()
{
    delete innards;
}

ClientSocket::ClientSocket ( std::string host, int port )
: host(host)
, port(port)
{
  innards = new SocketInnards();
  if ( ! innards->create() )
    {
      throw SocketException ( "Could not create client socket." );
    }
  if ( ! innards->connect ( host, port ) )
    {
      throw SocketException ( std::string("Could not connect to port.") + strerror(errno) );
    }
}

std::string ClientSocket::Receive(unsigned int size)
{
    return innards->Receive(size);
}

std::string ClientSocket::Receive()
{
    return innards->Receive();
}

void ClientSocket::SendBytes(const std::string& s)
{
    innards->SendBytes(s);
}
