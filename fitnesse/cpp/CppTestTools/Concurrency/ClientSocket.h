// Definition of the ClientSocket class

#ifndef ClientSocket_class
#define ClientSocket_class

#include "Socket.h"
#include <string>


class ClientSocket : public Socket
{
 public:

    ClientSocket (std::string host, int port );
    virtual ~ClientSocket();  
    
    virtual std::string Receive(unsigned int size);
    virtual std::string Receive();
    virtual void SendBytes(const std::string&);

private:
    std::string host;
    int port;
    
    SocketInnards* innards;
    ClientSocket(const ClientSocket&);
    ClientSocket& operator=(const ClientSocket&);
};


#endif
