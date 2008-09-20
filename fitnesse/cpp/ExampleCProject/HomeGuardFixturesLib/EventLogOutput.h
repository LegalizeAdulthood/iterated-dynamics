#ifndef D_EVENTLOGOUTPUT_H
#define D_EVENTLOGOUTPUT_H

#include "Platform.h"
#include "Fit/RowFixture.h"

class EventLogOutput : public RowFixture
{
public:
  EventLogOutput();
  ~EventLogOutput();

	class EventHolder : public Fixture
	{
	public:
	EventHolder()
	: sequenceNumber(-1)
    , event("none")
    , parameter(-1) 
    { 
        initialize();
    }

	EventHolder(int sequenceNumber, string event, int parameter)
	: sequenceNumber(sequenceNumber)
    , event(event)
    , parameter(parameter) 
    { 
        initialize();
    }

    void initialize()
    { 
	PUBLISH(EventHolder,int,sequenceNumber);
	PUBLISH(EventHolder,string,event);
	PUBLISH(EventHolder,int,parameter);
    }

    int sequenceNumber;
    string event;
    int parameter;

	};

	virtual RowFixture::ObjectList query() const;
	virtual const Fixture	*getTargetClass() const;

  private:

      EventHolder exampleEventHolder;
      mutable RowFixture::ObjectList rows;

      EventLogOutput(const EventLogOutput&);
      EventLogOutput& operator=(const EventLogOutput&);
};

#endif  // D_EVENTLOGOUTPUT_H


