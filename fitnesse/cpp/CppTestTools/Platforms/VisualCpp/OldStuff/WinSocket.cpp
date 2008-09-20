/*
   WinSocket.cpp
 
   Copyright (C) 2002-2004 Ren� Nyffenegger
 
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
 
   Ren� Nyffenegger rene.nyffenegger@adp-gmbh.ch
*/

#include <iostream>
#include "WinSocket.h"
#include "Platform.h"

using namespace std;


int WinSocket::nofSockets_= 0;

void WinSocket::Start()
{

  if (!nofSockets_)
    {
      WSADATA info;
      if (WSAStartup(MAKEWORD(2,0), &info))
        {
          throw "Could not start WSA";
        }
    }
  ++nofSockets_;
}

void WinSocket::End()
{
  WSACleanup();
}

WinSocket::WinSocket() : s_(0)
{
  Start();
  // UDP: use SOCK_DGRAM instead of SOCK_STREAM
  s_ = socket(AF_INET,SOCK_STREAM,0);

  refCounter_ = new int(1);
}

WinSocket::WinSocket(SOCKET s) : s_(s)
{
  Start();
  refCounter_ = new int(1);
};

WinSocket::~WinSocket()
{
  if (! --(*refCounter_))
    {
      Close();
      delete refCounter_;
    }


  --nofSockets_;
  if (!nofSockets_)
    End();
}

WinSocket::WinSocket(const WinSocket& o)
{
  refCounter_=o.refCounter_;
  (*refCounter_)++;
  s_         =o.s_;


  nofSockets_++;
}

WinSocket& WinSocket::operator =(WinSocket& o)
{
  (*o.refCounter_)++;


  refCounter_=o.refCounter_;
  s_         =o.s_;


  nofSockets_++;


  return *this;
}

void WinSocket::Close()
{
  closesocket(s_);
}

std::string WinSocket::ReceiveBytes()
{
  std::string ret;
  char buf[1024];
  int rv=1;

  while (rv != 0 && rv != WSAECONNRESET)
    {
      rv=recv(s_, buf, 1024,0);
      std::string t;
      t.assign(buf,rv);
      ret +=t;
    }

  return ret;
}

std::string WinSocket::Receive(unsigned int size)
{
  std::string ret;
  int status=1;
  char r;
  try
    {
      while(ret.size() < size && status != WSAECONNRESET)
        {
          status = recv(s_, &r, 1, 0);
          ret += r;
          if (status == WSAECONNRESET)
            cerr << "recv WSAECONNRESET" << endl;

        }
    }
  catch (...)
    {
      cerr << "recv threw and exception" << endl;
      // I don't know why recv can throw, but it can.
      // so we'll just pretend that the socket stopped
      // receiving and return what we got so far.
    }

  return ret;
}

std::string WinSocket::ReceiveLine()
{
  std::string ret;
  while (1)
    {
      char r;

      switch(recv(s_, &r, 1, 0))
        {
        case 0: // not connected anymore;
          return "";
        case -1:
          if (errno == EAGAIN)
            {
              return ret;
            }
          else
            {
              // not connected anymore
              return "";
            }
        }

      ret += r;
      if (r == '\n')
        return ret;
    }
}

void WinSocket::SendLine(std::string s)
{
  s += '\n';
  send(s_,s.c_str(),s.length(),0);
}

void WinSocket::SendBytes(const std::string& s)
{
  send(s_,s.c_str(),s.length(),0);
}

WinSocketServer::WinSocketServer(int port, int connections, TypeSocket type)
{
  sockaddr_in sa;

  memset(&sa, 0, sizeof(sa)); /* clear our address */

  sa.sin_family = PF_INET;
  sa.sin_port = htons(port);
  s_ = socket(AF_INET, SOCK_STREAM, 0);
  if (s_ == INVALID_SOCKET)
    {
      throw "INVALID_SOCKET";
    }

  if(type==NonBlockingSocket)
    {
      u_long arg = 1;
      ioctlsocket(s_, FIONBIO, &arg);
    }

  /* bind the socket to the internet address */
  if (bind(s_, (sockaddr *)&sa, sizeof(sockaddr_in)) == SOCKET_ERROR)
    {
      closesocket(s_);
      throw "INVALID_SOCKET";
    }
  listen(s_, connections);
}

Socket* WinSocketServer::Accept()
{
  SOCKET new_sock = accept(s_, NULL, NULL);
  if (new_sock == INVALID_SOCKET)
    {
      int rc = WSAGetLastError();
      if(rc==WSAEWOULDBLOCK)
        {
          return NULL; // non-blocking call, no request pending
        }
      else
        {
          throw "Invalid Socket";
        }
    }



  WinSocket* r = new WinSocket(new_sock);
  return r;
}

WinSocketClient::WinSocketClient(const std::string& host, int port) : WinSocket()
{
  std::string error;


  hostent *he;
  if ((he = gethostbyname(host.c_str())) == NULL)
    {
      error = strerror(errno);
      throw error;
    }


  sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr = *((in_addr *)he->h_addr);
  memset(&(addr.sin_zero), 0, 8);


  if (::connect(s_, (sockaddr *) &addr, sizeof(sockaddr)))
    {
      error = strerror(WSAGetLastError());
      throw error;
    }
}

WinSocketSelect::WinSocketSelect(WinSocket const * const s1, WinSocket const * const s2, TypeSocket type)
{
  FD_ZERO(&fds_);
  FD_SET(const_cast<WinSocket*>(s1)->s_,&fds_);
  if(s2)
    {
      FD_SET(const_cast<WinSocket*>(s2)->
             s_,&fds_);
    }


  TIMEVAL tval;
  tval.tv_sec = 0;
  tval.tv_usec = 1;


  TIMEVAL *ptval;
  if(type==NonBlockingSocket)
    {
      ptval = &tval;
    }
  else
    {
      ptval = NULL;
    }


  if (select (0, &fds_, (fd_set*) 0, (fd_set*) 0, ptval)
      == SOCKET_ERROR) throw "Error in select";
}

bool WinSocketSelect::Readable(WinSocket const * const s)
{
  if (FD_ISSET(s->s_,&fds_))
    return true;
  return false;
}


Socket* Socket::NewSocketClient(const std::string& host, int port)
{
  return new WinSocketClient(host, port);
}

SocketServer* SocketServer::NewSocketServer(int port, int connections, TypeSocket type)
{
  return new WinSocketServer(port, connections, type);
}


