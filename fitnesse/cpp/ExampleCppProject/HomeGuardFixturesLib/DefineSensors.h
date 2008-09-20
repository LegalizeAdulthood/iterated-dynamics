#ifndef D_DefineSensors_h
#define D_DefineSensors_h

#include "Platform.h"
#include "Fit/ColumnFixture.h"
#include <string>


class DefineSensors : public ColumnFixture
{
         
  public:
    explicit DefineSensors()
      {
      PUBLISH(DefineSensors,int,port);
      PUBLISH(DefineSensors,std::string,name);
      PUBLISH(DefineSensors,std::string,type);
      PUBLISH(DefineSensors,std::string,submit);
      PUBLISH(DefineSensors,std::string,reason);
    }

    virtual ~DefineSensors() {};

    std::string submit()
    {
   		theReason = std::string("Unknown sensor type");
    	return "false";
    }

    std::string reason()
    {
      return theReason;
    }

  private:

    int port;
    std::string name;
    std::string type;
    std::string theReason;
    
    DefineSensors(const DefineSensors&);
    DefineSensors& operator=(const DefineSensors&);

};

#endif  // D_DefineSensors_h


