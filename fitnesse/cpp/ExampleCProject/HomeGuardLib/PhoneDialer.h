#ifndef D_PhoneDialer_H
#define D_PhoneDialer_H

#include "VirtualCall.h"

struct PhoneDialerPrivateData
{
    char* police;
};

typedef struct PhoneDialerTag
{
    struct PhoneDialerTag* (*Destroy)(struct PhoneDialerTag*);
     
    //Public methods
    void (*SetPoliceNumber)(struct PhoneDialerTag*, char* policeNumber);
    void (*CallPolice)(struct PhoneDialerTag*);

    struct PhoneDialerPrivateData data;

} PhoneDialer;


PhoneDialer* PhoneDialer_Create(PhoneDialer*);

#endif  // D_PhoneDialer_H
