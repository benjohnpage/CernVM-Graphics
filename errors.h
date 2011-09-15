#ifndef ERRORS_H_INC
#define ERRORS_H_INC

#include <iostream>
#include <sstream>
#include <string>

#include "json/json.h"

namespace Errors
{
  class StreamFork
  {
    std::ostream& self_a;
    std::ostream& self_b;

    public:
      StreamFork(std::ostream& a, std::ostream& b )
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

      StreamFork& operator<< ( std::ostream& manipFunc( std::ostream& ) )
      {
        self_a << manipFunc;
        self_b << manipFunc;
        return *this;
      }
  };

  std::ostream& fatal(std::ostream& out);

  // Messaging streams
  extern StreamFork err;
  extern std::stringstream errorStream;
  extern StreamFork dbg; // Debug info
  extern std::stringstream debugStream; // Debug info

  // Utility functions
  std::string reverseByDelim( std::string reverseMe, char delimiter );
};

#endif
