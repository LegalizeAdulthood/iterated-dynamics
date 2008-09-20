#include "FrontPanel.h"
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>

static FrontPanel* FrontPanel_destroy(FrontPanel* self)
{
    //Add clean up code here
    free(self);
    return 0;
}

static void stub0(FrontPanel* self)
{
  printf("FrontPanel stub0 called\n");
}

static void stub1(FrontPanel* self, char* s)
{
  printf("FrontPanel stub1 called with <%s>\n", s);
}

FrontPanel* FrontPanel_Create()
{
    FrontPanel* self = malloc(sizeof(FrontPanel));
    memset(self, 0, sizeof(FrontPanel));

    //Public function initializers here 
    vBind(self, Destroy, FrontPanel_destroy);
    vBind(self, ArmOn, stub0);
    vBind (self, ArmOff, stub0);
    vBind (self, AudioAlarmOn, stub0);
    vBind (self, AudioAlarmOff, stub0);
    vBind (self, VisualAlarmOn, stub0);
    vBind (self, VisualAlarmOff, stub0);
    vBind (self, Display, stub1);
    return self;
}

