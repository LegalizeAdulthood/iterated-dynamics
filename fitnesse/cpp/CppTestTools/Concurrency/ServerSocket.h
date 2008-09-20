// Definition of the ServerSocket class

#ifndef ServerSocket_class
#define ServerSocket_class

#include "Socket.h"

class SocketInnards;


class ServerSocket
{
 public:
    ServerSocket ( int port );
    ServerSocket ();
    virtual ~ServerSocket();
    virtual Socket* Accept();

private:
    SocketInnards* innards;
    ServerSocket(const ServerSocket&);
    ServerSocket& operator=(const ServerSocket&);
  
};


#endif
