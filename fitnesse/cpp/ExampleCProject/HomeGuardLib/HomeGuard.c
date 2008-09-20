#include "HomeGuard.h"
#include <stdlib.h>
#include <memory.h>
#include <string.h>


static HomeGuard* HomeGuard_destroy(HomeGuard* self)
{
    free(self);
    return 0;
}

static void HomeGuard_arm(HomeGuard* self)
{
}

HomeGuard* HomeGuard_Create(FrontPanel* frontPanel)
{
    HomeGuard* self = malloc(sizeof(HomeGuard));
    memset(self, 0, sizeof(HomeGuard));

    //Public function initializers here 
    vBind(self, Destroy, HomeGuard_destroy);
    vBind(self, Arm, HomeGuard_arm);

    //Member initializers
    self->data.frontPanel = frontPanel;

    return self;
}
