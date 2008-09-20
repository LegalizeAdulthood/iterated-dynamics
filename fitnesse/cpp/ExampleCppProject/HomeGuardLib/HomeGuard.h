#ifndef D_HomeGuard_H
#define D_HomeGuard_H

///////////////////////////////////////////////////////////////////////////////
//
//  HomeGuard.h
//
//  HomeGuard is responsible for ...
//
///////////////////////////////////////////////////////////////////////////////

class FrontPanel;

class HomeGuard
  {
  public:
    explicit HomeGuard(FrontPanel*);
    virtual ~HomeGuard();

  private:

    HomeGuard(const HomeGuard&);
    HomeGuard& operator=(const HomeGuard&);

    FrontPanel* frontPanel;
  };

#endif  // D_HomeGuard_H
