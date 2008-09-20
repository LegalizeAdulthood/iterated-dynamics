#ifndef D_LogEntry_H
#define D_LogEntry_H

///////////////////////////////////////////////////////////////////////////////
//
//  LogEntry is responsible for holding the information associated 
//  with a single EventLog entry.  
//
///////////////////////////////////////////////////////////////////////////////

class LogEntry
  {
  public:
    explicit LogEntry();
    virtual ~LogEntry();

    void Set(const char* e, int p)
    {
        event = e;
        parameter = p;
    }

    const char* GetEvent() { return event; }

    int GetParameter() { return parameter; }

  private:
    const char* event;
    int parameter;

    LogEntry(const LogEntry&);
    LogEntry& operator=(const LogEntry&);

  };

#endif  // D_LogEntry_H
