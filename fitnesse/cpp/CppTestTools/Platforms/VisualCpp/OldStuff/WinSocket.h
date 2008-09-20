/*
   WinSocket.h
 
   Copyright (C) 2002-2004 René Nyffenegger
 
   This source code is provided 'as-is', without any express or implied
   warranty. In no event will the authors be held liable for any damages
   arising from the use of this software.
 
   Permission is granted to anyone to use this software for any purpose,
   including commercial applications, and to alter it and redistribute it
   freely, subject to the following restrictions:
 
   1. The origin of this source code must not be misrepresented; you must not
      claim that you wrote the original source code. If you use this source code
      in a product, an acknowledgment in the product documentation would be
      appreciated but is not required.
 
   2. Altered source versions must be plainly marked as such, and must not be
      misrepresented as being the original source code.
 
   3. This notice may not be removed or altered from any source distribution.
 
   René Nyffenegger rene.nyffenegger@adp-gmbh.ch
*/


#ifndef __WINSOCKET_H__
#define __WINSOCKET_H__
#include <WinSock2.h>

#include <string>
#include "../../Concurrency/Socket.h"
#include "../../Concurrency/SocketServer.h"

class WinSocket : public Socket
  {
  public:

    virtual ~WinSocket();
    WinSocket(const WinSocket&);
    WinSocket& operator=(WinSocket&);

    std::string ReceiveLine();
    std::string ReceiveBytes();
    std::string Receive(unsigned int size);

    void   Close();

    // The parameter of SendLine is not a const reference
    // because SendLine modifes the std::string passed.
    void   SendLine (std::string);

    // The parameter of SendBytes is a const reference
    // because SendBytes does not modify the std::string passed
    // (in contrast to SendLine).
    void   SendBytes(const std::string&);

  protected:
    friend class WinSocketServer;
    friend class WinSocketSelect;

    WinSocket(SOCKET s);
    WinSocket();


    SOCKET s_;

    int* refCounter_;

  private:
    static void Start();
    static void End();
    static int  nofSockets_;

  };

class WinSocketClient : public WinSocket
  {
  public:
    WinSocketClient(const std::string& host, int port);
  };


class WinSocketServer : public SocketServer, public WinSocket
  {
  public:
    WinSocketServer(int port, int connections, TypeSocket type=BlockingSocket);

    Socket* Accept();

  };

// http://msdn.microsoft.com/library/default.asp?url=/library/en-us/winsock/wsapiref_2tiq.asp
class WinSocketSelect
  {
  public:
    WinSocketSelect(WinSocket const * const s1, WinSocket const * const s2=NULL, TypeSocket type=BlockingSocket);

    bool Readable(WinSocket const * const s);

  private:
    fd_set fds_;
  };



#endif
