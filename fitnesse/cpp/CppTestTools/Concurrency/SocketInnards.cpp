// Implementation of the Socket class.


#include "Concurrency/SocketInnards.h"
#include "PlatformSocket.h"
#include <string>
#include <errno.h>
#include <iostream>

const int MAXCONNECTIONS = 5;
const int MAXRECV = 500;


SocketInnards::SocketInnards()
: sock ( -1 )
{
  platformInit();
}

SocketInnards::~SocketInnards()
{
  platformCleanUp();
}

bool SocketInnards::create()
{
  sock = socket ( AF_INET, SOCK_STREAM, 0 );

  if ( ! is_valid() )
    return false;

  int on = 1;
  if ( setsockopt ( sock, SOL_SOCKET, SO_REUSEADDR, ( const char* ) &on, sizeof ( on ) ) == -1 )
    return false;

  return true;
}

bool SocketInnards::bind ( const int port )
{

  if ( ! is_valid() )
      return false;

  sockaddr_in addr;
  memset ( &addr, 0, sizeof ( addr ) );

  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons ( port );

  int bind_return = ::bind ( sock,
                 ( struct sockaddr * ) &addr,
                 sizeof ( addr ) );

  if ( bind_return == -1 )
      return false;

  return true;
}

bool SocketInnards::listen() const
{
  if ( ! is_valid() )
      return false;

  int listen_return = ::listen ( sock, MAXCONNECTIONS );

  if ( listen_return == -1 )
      return false;

  return true;
}


bool SocketInnards::accept ( SocketInnards& new_socket ) const
{
  new_socket.sock = ::accept ( sock, NULL, NULL );

  if ( new_socket.sock <= 0 )
    return false;
  else
    return true;
}


bool SocketInnards::send ( const std::string s ) const
{
  int status = ::send ( sock, s.c_str(), s.size(), 0);
  if ( status == -1 )
      return false;
  else
      return true;
}


int SocketInnards::recv ( std::string& s ) const
{
  s = "";
  
  char buf [ MAXRECV + 1 ];
  memset ( buf, 0, MAXRECV + 1 );

  int status = ::recv ( sock, buf, MAXRECV, 0 );

  if ( status == -1 )
  {
    std::cout << "status == -1   errno == " << errno << "  in SocketInnards::recv\n";
    return 0;
  }
  s = buf;
  return status;
}

bool SocketInnards::connect ( const std::string host, const int port )
{
  struct hostent *he;
  if ((he = gethostbyname(host.c_str())) == NULL)
        return false;

  sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr = *((in_addr *)he->h_addr);
  memset(&(addr.sin_zero), 0, 8);

  if (::connect(sock, (sockaddr *) &addr, sizeof(sockaddr)))
        return false;
    
  return true;    
}

std::string SocketInnards::Receive(unsigned int size)
{
  char* buf = new char[ size + 1 ];

  std::string s = "";
  int status = 0;

  while (s.length() < size && status >= 0)
  {
    memset ( buf, 0, size + 1 );
    status = ::recv ( sock, buf, size - s.length(), 0 );

    if ( status == -1 )
      std::cout << "status == -1   errno == " << errno << "  in SocketInnards::recv\n";
    
    s += buf;
  }
  
  delete [] buf;
  return s;
}

std::string SocketInnards::Receive()
{
  char buf [ MAXRECV + 1 ];

  memset ( buf, 0, MAXRECV + 1 );
  
  int status = ::recv ( sock, buf, MAXRECV, 0 );

  if ( status == -1)
      std::cout << "status == -1   errno == " << errno << "  in SocketInnards::recv\n";
    
  return buf;
}

void SocketInnards::SendBytes(const std::string& s)
{
  send(s);
}
