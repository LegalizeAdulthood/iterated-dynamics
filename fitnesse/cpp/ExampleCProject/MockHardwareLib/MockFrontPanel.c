#include "MockFrontPanel.h"
#include <stdlib.h>
#include <memory.h>
#include <string.h>

FrontPanel* MockFrontPanel_Create(void)
{
    FrontPanel* self = FrontPanel_Create();

    //vBind(self, armOn, yourLocalArmOn);
    //etc.

    return self;
}

int MockFrontPanel_IsArmed()
{
  return 0;
}

char* MockFrontPanel_GetDisplay(void)
{
  return "none";
  
}
