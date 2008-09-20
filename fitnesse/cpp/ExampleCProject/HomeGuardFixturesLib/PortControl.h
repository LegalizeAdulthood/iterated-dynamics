#ifndef D_PortControl_H
#define D_PortControl_H

#include "Platform.h"
#include "Fit/ColumnFixture.h"

class PortControl : public ColumnFixture
{
         
  public:
    explicit PortControl()
    {
      PUBLISH(PortControl, std::string, port);
      PUBLISH(PortControl, int, value);
    }

    virtual ~PortControl() {};

    void execute()
    {
      //report the port setting change
    }

  private:

    std::string port;
    int value;
    PortControl(const PortControl&);
    PortControl& operator=(const PortControl&);

};

#endif  // D_PortControl_H


