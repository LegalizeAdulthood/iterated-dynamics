
// Definition of the Socket class
//origin http://linuxgazette.net/issue74/tougher.html

#ifndef D_SocketInnards_H
#define D_SocketInnards_H

#include "Socket.h"


class SocketInnards : public Socket
{
 public:
  SocketInnards();
  virtual ~SocketInnards();
  
  std::string Receive(unsigned int size);
  std::string Receive();
  void SendBytes(const std::string&);

  //Platform specific
  void platformInit();
  void platformCleanUp();

  // Server initialization
  bool create();
  bool bind ( const int port );
  bool listen() const;
  bool accept ( SocketInnards& ) const;

  // Client initialization
  bool connect ( const std::string host, const int port );

  bool is_valid() const { return sock != -1; }

 private:

  int sock;
  std::string inBuffer;

  // Data Transimission
  bool send ( const std::string ) const;
  int recv ( std::string& ) const;

  SocketInnards(const SocketInnards&);
  SocketInnards& operator=(const SocketInnards&);
};


#endif
