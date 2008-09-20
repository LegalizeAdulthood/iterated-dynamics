#include "MockPhoneDialer.h"
#include <stdlib.h>
#include <memory.h>

static struct MockPhoneDialerPrivateData data;

static PhoneDialer* MockPhoneDialer_destroy(PhoneDialer* self)
{
    MockPhoneDialer* mock = (MockPhoneDialer*)(self);
    (*castToDestroyer(PhoneDialer)(mock->data.baseDestroyer))(self);
    return 0;
}

static void MockPhoneDialer_CallPolice(PhoneDialer* base)
{
    data.policeCalled = 1;
}

PhoneDialer* MockPhoneDialer_Create(MockPhoneDialer* self)
{
   if (self == 0)
        self = malloc(sizeof(MockPhoneDialer));

    memset(self, 0, sizeof(MockPhoneDialer));
    PhoneDialer_Create(&self->data.baseDialer);
    self->data.baseDestroyer = self->data.baseDialer.Destroy;

    //Public function initializers here 
    vBind((&self->data.baseDialer), Destroy, MockPhoneDialer_destroy);
    vBind((&self->data.baseDialer), CallPolice, MockPhoneDialer_CallPolice);

    //Member initializers
    MockPhoneDialer_Reset(&self->data.baseDialer);

    return &(self->data.baseDialer);
}

void MockPhoneDialer_Reset(PhoneDialer* base)
{
    data.policeCalled = 0;
}

int MockPhoneDialer_WerePoliceCalled()
{
  return data.policeCalled;
}
