#ifndef D_MockPhoneDialer_H
#define D_MockPhoneDialer_H

#include "VirtualCall.h"
#include "PhoneDialer.h"

struct MockPhoneDialerPrivateData
{
  PhoneDialer baseDialer;
  void* baseDestroyer;
  int policeCalled;
};

typedef struct MockPhoneDialerTag
{
    
    struct MockPhoneDialerPrivateData data;

} MockPhoneDialer;


PhoneDialer* MockPhoneDialer_Create(MockPhoneDialer*);

void MockPhoneDialer_Reset(PhoneDialer* base);
int MockPhoneDialer_WerePoliceCalled();


#endif  // D_MockPhoneDialer_H
