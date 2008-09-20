#include "HomeGuard.h"
#include "FrontPanel.h"

HomeGuard::HomeGuard(FrontPanel* p)
{
    frontPanel = p;
    frontPanel->armOn();
}

HomeGuard::~HomeGuard()
{
    delete frontPanel;
}

