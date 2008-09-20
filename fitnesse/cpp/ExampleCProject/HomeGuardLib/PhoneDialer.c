#include "PhoneDialer.h"
#include <stdlib.h>
#include <memory.h>

static PhoneDialer* PhoneDialer_destroy(PhoneDialer* phoneDialer)
{
    //Add clean up code here
    free(phoneDialer);
    return 0;
}

static void PhoneDialer_setPoliceNumber(PhoneDialer* self, char* s)
{
}

PhoneDialer* PhoneDialer_Create(PhoneDialer* self)
{
    if (self == 0)
        self = malloc(sizeof(PhoneDialer));

    memset(self, 0, sizeof(PhoneDialer));

    //Public function initializers here 
    vBind(self, Destroy, PhoneDialer_destroy);
    vBind(self, SetPoliceNumber, PhoneDialer_setPoliceNumber);

    //Member initializers
    self->data.police = "";
    
    return self;
}
