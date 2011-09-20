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
#include <cstdio>
using std::ostream;
using std::cerr;
using std::stringstream;
using std::string;
using std::getline;


// Messaging streams
stringstream Errors::errorStream("");
stringstream Errors::debugStream("");
Errors::StreamFork Errors::err(std::cerr, errorStream);
Errors::StreamFork Errors::dbg(std::cerr, debugStream);;

// Messaging views
Objects::View Objects::errorView;
Objects::View Objects::debugView;

ostream& Errors::fatal(ostream& out)
{
  boinc_close_window_and_quit("Aborting... \n");
  return out;
}

string Errors::reverseByDelim( string reverseMe, char delimiter )
{
  stringstream reverseStream( reverseMe );
  string reversedText;
  while (reverseStream)
  {
    string line;
    getline(reverseStream, line, '\n');
    reversedText = line + '\n' + reversedText;
  }

  return reversedText;
}

Objects::ErrorDisplay::ErrorDisplay( Json::Value data )
  : Objects::Object( data )
{}

void Objects::ErrorDisplay::render(double timestamp)
{
  // Simple rendering routine, we're just displaying the error stream as
  // text. It could probably do with an abilityto not fill over the screen
  // but principles of YNGNI suggest that someone should cross that bridge
  // when they come to it.

  string displayText;
  displayText = Errors::reverseByDelim( Errors::errorStream.str(), '\n' );

  if (self_coordType == Objects::NON_NORM)
    Graphics::drawText( displayText, (int)self_x, (int)self_y );

  if (self_coordType == Objects::NORM)
    Graphics::drawText( displayText, self_x, self_y);
}

Objects::DebugDisplay::DebugDisplay( Json::Value data )
  : Objects::Object( data )
{}

void Objects::DebugDisplay::render( double timestamp )
{
  // Simple rendering routine, we're just displaying the error stream as
  // text. It could probably do with an abilityto not fill over the screen
  // but principles of YNGNI suggest that someone should cross that bridge
  // when they come to it.

  string displayText;
  displayText = Errors::reverseByDelim( Errors::debugStream.str(), '\n' );

  if (self_coordType == Objects::NON_NORM)
    Graphics::drawText( displayText, (int)self_x, (int)self_y );

  if (self_coordType == Objects::NORM)
    Graphics::drawText( displayText, self_x, self_y);
}
