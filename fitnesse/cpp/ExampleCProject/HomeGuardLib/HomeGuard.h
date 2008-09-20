#ifndef D_HomeGuard_H
#define D_HomeGuard_H

#include "VirtualCall.h"
#include "FrontPanel.h"

struct HomeGuardPrivateData
{
    FrontPanel* frontPanel;
};

typedef struct HomeGuardTag
{
    struct HomeGuardTag* (*Destroy)(struct HomeGuardTag* self);
     
    //Public override able methods
    void (*Arm)(struct HomeGuardTag* self);
    void (*DisArm)(struct HomeGuardTag* self);
    int (*DefineSensor)(struct HomeGuardTag* self,
              int port, int type, char* name);
    void (*PortActive)(struct HomeGuardTag* self, int port);
    void (*PortInActive)(struct HomeGuardTag* self, int port);
    
   
    struct HomeGuardPrivateData data;

} HomeGuard;


HomeGuard* HomeGuard_Create(FrontPanel*);

#define PORT_A1 0
#define PORT_A2 1
#define PORT_A3 2
#define PORT_A4 3

#define WINDOW_SENSOR     0
#define DOOR_SENSOR       1 
#define WATERLEVEL_SENSOR 2
#define MOTION_SENSOR     3 



#endif  // D_HomeGuard_H
