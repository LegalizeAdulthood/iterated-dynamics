// Definition of the Socket class
//origin http://linuxgazette.net/issue74/tougher.html

#ifndef Socket_class
#define Socket_class

#include <string>

class SocketInnards;

class Socket
{
  public:
    Socket() {};
    virtual ~Socket() {};
      
    virtual std::string Receive(unsigned int size) = 0;
    virtual std::string Receive() = 0;
    virtual void SendBytes(const std::string&) = 0;
    
    Socket& operator << ( const std::string& s)
        { SendBytes(s); return *this; }
    Socket& operator >> ( std::string& s)
        { s = Receive(); return *this;}

 private:
    Socket(const Socket&);
    Socket& operator=(const Socket&);
};


#endif
