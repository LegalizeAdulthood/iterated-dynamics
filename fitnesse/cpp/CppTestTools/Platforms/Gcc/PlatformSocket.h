#ifndef D_PlatformSocket_H
#define D_PlatformSocket_H

#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>

void SocketInnards::platformInit()
{
    //none needed   
}

void SocketInnards::platformCleanUp()
{
  if ( is_valid() )
    ::close ( sock );
}


#endif
