#include "errors.h"
#include "objects.h"
#include "graphics.h"

//BOINC
#include "graphics2.h"

//JsonCpp
#include "json/json.h"

//std
#include <iostream>
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

Objects::ErrorDisplay::ErrorDisplay( Json::Value data )
  : Objects::Object( data )
{}

void Objects::ErrorDisplay::render()
{
  // Simple rendering routine, we're just displaying the error stream as
  // text. It could probably do with an abilityto not fill over the screen
  // but principles of YNGNI suggest that someone should cross that bridge
  // when they come to it.

  string displayText = Errors::errorStream.str();

  if (self_coordType == Objects::NON_NORM)
    Graphics::drawText( displayText, (int)self_x, (int)self_y );

  if (self_coordType == Objects::NORM)
    Graphics::drawText( displayText, self_x, self_y );
}