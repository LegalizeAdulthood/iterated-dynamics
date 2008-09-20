#ifndef D_PlatformSocket_H
#define D_PlatformSocket_H

#include <WinSock2.h>

class WSA_Startup_Cleanup 
{
public:
  WSA_Startup_Cleanup()
  {
  WSADATA info;
  if (WSAStartup(MAKEWORD(2,0), &info))
    {
      throw "Could not start WSA";
    }
  }

  ~WSA_Startup_Cleanup()
  {
      //Do a WSACleanup while statics are being destroyed after main exists
      WSACleanup();    
  }
};

void SocketInnards::platformInit()
{
    //Do a WSAStartup on the first socket initialization 
    static WSA_Startup_Cleanup oneTimeOnly;
}

void SocketInnards::platformCleanUp()
{
    closesocket(sock);
}

#endif
