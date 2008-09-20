// Copyright (c) 2004 Micahel Feathers, James Grenning, Micah Martin, Robert Martin.
// Released under the terms of the GNU General Public License version 2 or later.

#ifndef D_FitnesseServer_h
#define D_FitnesseServer_h

#include <string>
#include <memory>

class Socket;
class Fixture;
class FixtureMaker;

using std::string;
using std::auto_ptr;

//---------------------------------
// This class is the communication channel between FitNesse
// and the C++ version of FIT.
//
class FitnesseServer
  {

  public:
    static int					Main(int ac, char** av, FixtureMaker*);

    explicit					FitnesseServer(string host, int port, int socketTicket, FixtureMaker*);
    virtual						~FitnesseServer();

    int							run();
    int							getFailures();

  private:
    bool						connect();
    int							readSizeFromFitnesse();
    string						readFromFitnesse();
    void						reportResultsToFitnesse(Fixture& fixture, const string& out);
    void						writeToFitnesse(const string& value);
    void						writeSizeToFitnesse(int size);
    string						doFixture(const string& in, Fixture& fixture);
    int							toInt(const string& s);
    string						makeRequest();

    Socket* socket;
    string host;
    int port;
    int socketTicket;
    int failures;
    auto_ptr<FixtureMaker>      maker;

    FitnesseServer(const FitnesseServer&);
    FitnesseServer& operator=(const FitnesseServer&);
  };

#endif  // D_FitnesseServer_h


