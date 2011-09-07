#include "errors.h"

#include "graphics2.h"

#include <iostream>
#include <string>
#include <sstream>
using std::ostream;
using std::cerr;


stringstream Errors::errorStream("");
Errors::StreamFork Errors::err(std::cerr, errorStream);

ostream& Errors::fatal(ostream& out)
{
  boinc_close_window_and_quit("Aborting... \n");
  return out;
}
