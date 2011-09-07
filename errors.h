#ifndef ERRORS_H_INC
#define ERRORS_H_INC

#include <iostream>
#include <sstream>
#include <string>
using std::ostream;
using std::stringstream;
using std::string;

// The real aim here is to produce a cerr replacement that both outputs to
// cerr, and captures the errors so that we can display them graphically.

namespace Errors
{
  ostream& fatal(ostream& out);

  class StreamFork
  {
    ostream& self_a;
    ostream& self_b;

    public:
      StreamFork(ostream& a, ostream& b )
        : self_a ( a ), self_b ( b )
      {
      }

      template< typename T >
      StreamFork& operator<< (const T& obj)
      {
        self_a << obj;
        self_b << obj;
        return *this;
      }

      StreamFork& operator<< ( ostream& manipFunc( ostream& ) )
      {
        self_a << manipFunc;
        self_b << manipFunc;
        return *this;
      }
  };

  extern StreamFork err;
  extern stringstream errorStream;
};

#endif
