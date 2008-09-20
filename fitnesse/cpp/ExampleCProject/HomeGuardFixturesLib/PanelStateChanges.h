ifndef D_PANELSTATECHANGES_H
#define D_PANELSTATECHANGES_H

#include "Platform.h"
#include "Fit/RowFixture.h"

class PanelStateChanges : public RowFixture
{
public:
  PanelStateChanges();
  ~PanelStateChanges();

	class FrontPanelState : public Fixture
	{
	public:
		FrontPanelState()
			: name("-"), state("-"), sequenceNumber(0)
		{ initialize(); }

		FrontPanelState(int sequenceNumber, string name, string state)
			: sequenceNumber(sequenceNumber), name(name), state(state) 
		{ 
      initialize(); 
    }

		void initialize()
		{
			PUBLISH(FrontPanelState,string,name);
			PUBLISH(FrontPanelState,string,state);
			PUBLISH(FrontPanelState,int,sequenceNumber);
		}

    string name;
    string state;
    int sequenceNumber;

	};

	virtual RowFixture::ObjectList query() const;
	virtual const Fixture	*getTargetClass() const;

  void arm();
  void disarm();
  void audibleAlarmOn();
  void visualAlarmOn();
  void display(std::string& message);
 
  private:

    int sequenceNumber;
	  FrontPanelState	exampleFrontPanelState;
    mutable RowFixture::ObjectList rows;
    void addNewState(std::string name, std::string state);

    PanelStateChanges(const PanelStateChanges&);
    PanelStateChanges& operator=(const PanelStateChanges&);
};



#endif  // D_PANELSTATECHANGES_H


