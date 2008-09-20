#ifndef D_MockFrontPanel_H
#define D_MockFrontPanel_H

#include "VirtualCall.h"
#include "FrontPanel.h"

FrontPanel* MockFrontPanel_Create(void);

int MockFrontPanel_IsArmed(void);
char* MockFrontPanel_GetDisplay(void);
int MockFrontPanel_IsAudioAlarmOn();
int MockFrontPanel_IsVisualAlarmOn();

#endif  // D_MockFrontPanel_H

