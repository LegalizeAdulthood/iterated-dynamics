// Implementation of the ServerSocket class

#include "ServerSocket.h"
#include "SocketInnards.h"
#include "SocketException.h"


ServerSocket::ServerSocket ( int port )
{
    innards = new SocketInnards();
    
  if ( ! innards->create() )
    {
      throw SocketException ( "Could not create server socket." );
    }

  if ( ! innards->bind ( port ) )
    {
      throw SocketException ( "Could not bind to port." );
    }

  if ( ! innards->listen() )
    {
      throw SocketException ( "Could not listen to socket." );
    }

}

ServerSocket::~ServerSocket()
{
    delete innards;
}

Socket* ServerSocket::Accept()
{
    SocketInnards* socket = new SocketInnards();
    if ( ! innards->accept ( *socket ) )
    {
        throw SocketException ( "Could not accept socket." );
    }
    return socket;
}
