#ifndef D_FrontPanel_H
#define D_FrontPanel_H

#include "VirtualCall.h"

struct FrontPanelPrivateData
{
    int exampleMember;
};

typedef struct FrontPanelTag
{
    struct FrontPanelTag* (*Destroy)(struct FrontPanelTag* self);
     
    //Public override able methods
    void (*ArmOn)(struct FrontPanelTag* self);
    void (*ArmOff)(struct FrontPanelTag* self);
    void (*AudioAlarmOn)(struct FrontPanelTag* self);
    void (*AudioAlarmOff)(struct FrontPanelTag* self);
    void (*VisualAlarmOn)(struct FrontPanelTag* self);
    void (*VisualAlarmOff)(struct FrontPanelTag* self);
    void (*Display)(struct FrontPanelTag* self, char* message);
    
    struct FrontPanelPrivateData data;

} FrontPanel;

FrontPanel* FrontPanel_Create();

#endif  // D_FrontPanel_H
