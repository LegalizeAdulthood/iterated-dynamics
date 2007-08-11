#include "stdafx.h"
#include <cmath>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <vector>

#if 0
// iomanip_setw.cpp 
// compile with: /EHsc
// Defines the entry point for the console application.
//
// Sample use of the following manipulators:
//   resetiosflags
//   setiosflags
//   setbase
//   setfill
//   setprecision
//   setw

#include <iostream>
#include <iomanip>

using namespace std;

const double   d1 = 1.23456789;
const double   d2 = 12.3456789;
const double   d3 = 123.456789;
const double   d4 = 1234.56789;
const double   d5 = 12345.6789;
const long      l1 = 16;
const long      l2 = 256;
const long      l3 = 1024;
const long      l4 = 4096;
const long      l5 = 65536;
int         base = 10;

void DisplayDefault( )
{
   cout << endl << "default display" << endl;
   cout << "d1 = " << d1 << endl;
   cout << "d2 = " << d2 << endl;
   cout << "d3 = " << d3 << endl;
   cout << "d4 = " << d4 << endl;
   cout << "d5 = " << d5 << endl;
}

void DisplayWidth( int n )
{
   cout << endl << "fixed width display set to " << n << ".\n";
   cout << "d1 = " << setw(n) << d1 << endl;
   cout << "d2 = " << setw(n) << d2 << endl;
   cout << "d3 = " << setw(n) << d3 << endl;
   cout << "d4 = " << setw(n) << d4 << endl;
   cout << "d5 = " << setw(n) << d5 << endl;
}

void DisplayLongs( )
{
   cout << setbase(10);
   cout << endl << "setbase(" << base << ")" << endl;
   cout << setbase(base);
   cout << "l1 = " << l1 << endl;
   cout << "l2 = " << l2 << endl;
   cout << "l3 = " << l3 << endl;
   cout << "l4 = " << l4 << endl;
   cout << "l5 = " << l5 << endl;
}

int main( int argc, char* argv[] )
{
   DisplayDefault( );

   cout << endl << "setprecision(" << 3 << ")" << setprecision(3);
   DisplayDefault( );

   cout << endl << "setprecision(" << 12 << ")" << setprecision(12);
   DisplayDefault( );

   cout << setiosflags(ios_base::scientific);
   cout << endl << "setiosflags(" << ios_base::scientific << ")";
   DisplayDefault( );

   cout << resetiosflags(ios_base::scientific);
   cout << endl << "resetiosflags(" << ios_base::scientific << ")";
   DisplayDefault( );

   cout << endl << "setfill('" << 'S' << "')" << setfill('S');
   DisplayWidth(15);
   DisplayDefault( );

   cout << endl << "setfill('" << ' ' << "')" << setfill(' ');
   DisplayWidth(15);
   DisplayDefault( );

   cout << endl << "setprecision(" << 8 << ")" << setprecision(8);
   DisplayWidth(10);
   DisplayDefault( );

   base = 16;
   DisplayLongs( );

   base = 8;
   DisplayLongs( );

   base = 10;
   DisplayLongs( );

   return   0;
}
#endif

std::ostream &operator<<(std::ostream &stream, const SimpleString &text)
{
	stream << text.asCharString();
	return stream;
}

class idTestResult : public TestResult
{
public:
	idTestResult() {}
	virtual ~idTestResult() {}
	virtual void testsStarted()
	{
		m_start = clock();
	}
	virtual void addFailure(const Failure &failure)
	{
		m_failures.push_back(failure);
		std::cerr << failure.fileName << "(" << failure.lineNumber << "): " << failure.message << "\n";

	}
	std::string ElapsedTime()
	{
		std::ostringstream buff;

		double seconds = (m_end - m_start)/1000.0;
		if (seconds < 60)
		{
			buff << seconds << "s";
		}
		else
		{
			double minutes = std::floor(seconds/60);
			seconds -= minutes*60;
			if (minutes >= 60)
			{
				double hours = std::floor(minutes/60);
				minutes -= hours*60;
				if (hours >= 24)
				{
					double days = std::floor(hours/24.0);
					hours -= days*24;
					buff << days << " days ";
				}
				buff << std::setw(2) << std::setfill('0') << hours << ":";
			}
			buff << std::setw(2) << std::setfill('0') << minutes << ":"
				<< std::setfill('0') << std::setw(2) << seconds;
		}

		return buff.str();
	}
	virtual void testsEnded()
	{
		m_end = clock();

		if (m_failures.size() > 0)
		{
			std::cerr << m_failures.size() << " failures, elapsed time is "
				<< ElapsedTime() << "\n";

		}
	}
	int Failures() const { return int(m_failures.size()); }

private:
	std::time_t m_start;
	std::time_t m_end;
	std::vector<Failure> m_failures;
};

int main(int argc, char *argv[])
{
	idTestResult results;
	TestRegistry::runAllTests(results);

	return results.Failures();
}
