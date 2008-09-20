#ifndef D_Timer_h
#define D_Timer_h

class Action;
struct TimeEntry;

class Timer
  {
  public:
    explicit Timer();

    virtual ~Timer();

    void notifyPeriodically(long ms, Action* callback);

    void tic();

  private:

    TimeEntry* itsEntry;

    Timer(const Timer&);
    Timer& operator=(const Timer&);

  };

#endif  // D_Timer_h
