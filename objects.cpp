#include <string>
#include <sstream>
#include <cmath>
#include <cstdio>
#include <map>
#include <vector>

using std::string;
using std::stringstream;
using std::fmod;
using std::sprintf;
using std::map;
using std::vector;

#include "json/json.h"

//BOINC
#include "graphics2.h"

//Our things
#include "graphics.h"
#include "objects.h"
#include "boincShare.h"
#include "resources.h"


Objects::ViewList Objects::viewList;
Objects::View*    Objects::activeView;

void Objects::loadObjects( Json::Value objects )
{
  // This function loads all the objects in the provided Json structure into
  // their appropriate views.
  // If the Json structure is an array of objects then there is only one
  // view which is viewList[0]. If it is an array of arrays of objects then
  // each element represents a view, indexed in order of passing.

  Json::Value views;

  if ( objects[0u].isArray() )
    views = objects;
  else
    views.append( objects );

  for (size_t viewI = 0; viewI < views.size(); viewI++)
  {
    // Make a new view
    Json::Value   viewObjects = views[ viewI ];
    Objects::View view;

    // Add objects into it
    for (size_t n = 0; n < viewObjects.size(); n++)
    {
      Objects::Object * newObj;
      Json::Value objectData = viewObjects[n];
  
      //Call correct construtor
      if (objectData["type"] == "boincValue")
        newObj = new Objects::BoincValue(objectData);
      if (objectData["type"] == "strings")
        newObj = new Objects::StringDisplay(objectData);
      if (objectData["type"] == "slideshow")
        newObj = new Objects::Slideshow(objectData);
      if (objectData["type"] == "gridshow")
        newObj = new Objects::Gridshow(objectData);
      if (objectData["type"] == "spriteDisplay")
        newObj = new Objects::SpriteDisplay(objectData);
      if (objectData["type"] == "panSprite")
        newObj = new Objects::PanSprite(objectData);
  
      view . push_back( newObj );
    }

    Objects::viewList . push_back( view );
  }
  
  // Set first view to active view (this should ALWAYS exist)
  Objects::activeView = &Objects::viewList[0];
}

void Objects::removeObjects()
{
  using Objects::viewList;
  using Objects::activeView;

  for (size_t i = 0; i < viewList.size(); i++)
  {
    for (size_t j = 0; j < viewList[i].size(); j++)
    {
      delete viewList[i][j];
    }
  }

  viewList.clear();
  activeView = NULL;
}

Objects::Object::Object(Json::Value data) :
 self_data( data ), self_x(0), self_y(0)
{
  if ( data["dimensions"].isObject() )
  {
    if ( data["dimensions"]["x"].isInt() )
    {
      self_x         = data["dimensions"]["x"].asInt();
      self_y         = data["dimensions"]["y"].asInt();
      self_coordType = Objects::NON_NORM;
    }

    if ( data["dimensions"]["x"].isDouble() )
    {
      self_x         = data["dimensions"]["x"].asDouble();
      self_y         = data["dimensions"]["y"].asDouble();
      self_coordType = Objects::NORM;
    }
  }
}

Objects::Slideshow::Slideshow( Json::Value data ) :
  Objects::Object( data ), self_w(0), self_h(0), self_time(0),
  self_timeout(30)
{
  Json::Value dimensions = data["dimensions"];
  if ( dimensions.isString() )
  {
    //If it is a string, dimensions should say "fullscreen". If it doesn't,
    //we pretend it was meant to and complain
    if (dimensions != "fullscreen")
    {
      string error =  "Slideshow - provided dimensions as string, but does";
             error += "not want fullscreen, interpretting as fullscreen";
             error += "anyway \n";
      fprintf(stderr, "ERROR: %s",  error.c_str() );
      printf ("ERROR: %s", error.c_str() );
    }

    self_x = -0.5f;
    self_y = -0.5f;
    self_w = 1.0;
    self_h = 1.0;
   
    self_coordType = Objects::NORM;
  }
  else
  {
    if ( self_coordType == Objects::NON_NORM )
    {
      //If they aren't there, then this defaults to 0, which is used to
      //signify "natural size" in the drawing routine.
      self_w = data["dimensions"]["w"].asInt();
      self_h = data["dimensions"]["h"].asInt();
    }

    if ( self_coordType == Objects::NORM )
    {
      //These NEED to be provided - FIXME update docs.
      self_w = data["dimensions"]["w"].asDouble();
      self_h = data["dimensions"]["h"].asDouble();
    }
  }
  
  self_spriteGroup = data["sprites"].asString();
  self_spriteIter = Graphics::sprites[ self_spriteGroup ].begin();

  self_timeout = data["timeout"].asInt();
}


void Objects::Slideshow::render()
{
  //Iteratiiiiing
  if (self_time  == self_timeout)
  {
    self_time = 0;
    self_spriteIter++;

    if (self_spriteIter == Graphics::sprites[self_spriteGroup].end())
      self_spriteIter = Graphics::sprites[self_spriteGroup].begin();
  }
  else
    self_time++;

  if ( self_coordType == Objects::NORM )
    self_spriteIter -> second -> draw(self_x, self_y, self_w, self_h);

  if ( self_coordType == Objects::NON_NORM )
    self_spriteIter -> second -> draw( (int)self_x, (int)self_y, 
                                       (int)self_w, (int)self_h);
}


Objects::BoincValue::BoincValue(Json::Value data) : 
  Objects::Object( data )
{
  self_prefix = data["prefix"].asString();
  self_type   = data["valueType"].asString();
}

void Objects::BoincValue::render()
{
  //The assumption is that the shared memory is updated regularly in the
  //graphics loop

  stringstream output;
  output << self_prefix;
  if (self_type == "username")
    output << Share::data->username;
  if (self_type == "credit")
    output << Share::data->credit;

  if (self_coordType == Objects::NORM)
    Graphics::drawText(output.str(), self_x, self_y);

  if (self_coordType == Objects::NON_NORM)
    Graphics::drawText(output.str(), (int)self_x, (int)self_y);
}



Objects::StringDisplay::StringDisplay(Json::Value data) :
  Objects::Object( data )
{
  if (data["maxLines"].isNull())
    self_maxLines = -1;
  else
    self_maxLines  = data["maxLines"].asInt();

  self_lineWidth = data["lineWidth"].asInt();
  self_delimiter = data["delimiter"].asString();
  
  Json::Value strings;
  strings = data["strings"];

  if (data["external"].isBool())
    if (data["external"] == true)
    {
      string resource = data["resource"].asString();
      string node     = data["node"].asString();
      strings = Resources::getResourceNode(resource, node);
    }
  

  //Create array of human readable strings. New entry every "self_maxLines"
  //lines
  stringstream outputStream;
  int lineN = 0;
  for (Json::ValueIterator itr = strings.begin(); 
       itr != strings.end(); 
       itr++)
  {
    //Stored as "key" : "value", so extract these
    string key = itr.key().asString();
    Json::Value value = strings[key];
 
    //buffer key and do appropriate thing for value
    outputStream << key << self_delimiter;
    if (value.type() == Json::stringValue)
      outputStream << value.asString();
    else if (value.isNumeric())
      outputStream << value.asDouble();
    outputStream << "\n";

    lineN++;
    if (lineN == self_maxLines)
    {
      self_displayStrings.push_back(outputStream.str());
      lineN = 0;
      outputStream.str("");
    }

  }
  //Add remaining string
  self_displayStrings.push_back( outputStream.str() );
}

void Objects::StringDisplay::render()
{
  if ( self_coordType == Objects::NON_NORM )
  {
    int pixelWidthPerChar = 12;
    for (size_t i = 0; i < self_displayStrings.size(); i++)
    {
      int drawX = self_x + i * pixelWidthPerChar * self_lineWidth;
      Graphics::drawText(self_displayStrings[i], drawX, (int)self_y);
    }
  }

  if ( self_coordType == Objects::NORM )
  {
    double fracWidthPerChar = 0.011;
    for (size_t i = 0; i < self_displayStrings.size(); i++)
    {
      double drawX = self_x + i * fracWidthPerChar * self_lineWidth;
      Graphics::drawText(self_displayStrings[i], drawX, self_y);
    }
  }
}

Objects::SpriteDisplay::SpriteDisplay(Json::Value data) :
  Objects::Object( data )
{
  self_spriteName = data["sprite"].asString();

  Json::Value dimensions = data["dimensions"];
  if ( dimensions["w"].isInt() )
  {
    self_w = dimensions["w"].asInt();
    self_h = dimensions["h"].asInt();
  }
  if ( dimensions["w"].isDouble() )
  {
    self_w = dimensions["w"].asInt();
    self_h = dimensions["h"].asInt();
  }
}
void Objects::SpriteDisplay::render()
{
  if ( self_coordType == Objects::NON_NORM )
   Graphics::getSprite( self_spriteName ) -> draw( (int)self_x, 
                                                   (int)self_y,
                                                   (int)self_w, 
                                                   (int)self_h );

  if ( self_coordType == Objects::NORM )
    Graphics::getSprite( self_spriteName ) -> draw( self_x, self_y,
                                                    self_w, self_h );
}

Objects::Gridshow::Gridshow(Json::Value data) :
  Objects::Object( data )
{
  self_spriteGroup = data["sprites"].asString();

  Json::Value dimensions = data["dimensions"];

  //Position and cell dimensions
  if ( dimensions["cellWidth"].isInt() )
  {
    self_cellWidth  = dimensions["cellWidth"].asInt();
    self_cellHeight = dimensions["cellHeight"].asInt();
  }
  if ( dimensions["cellWidth"].isDouble() )
  {
    self_cellWidth  = dimensions["cellWidth"].asDouble();
    self_cellHeight = dimensions["cellHeight"].asDouble();
  }

  //Cell settings
  self_cellsWide  = dimensions["cellsWide"].asInt();
  self_numCells   = data["numCells"].asInt();

  self_time       = 0;
  self_timeout    = data["timeout"].asInt();

  //We start the drawing with the first sprite in the group
  self_spriteIter = Graphics::sprites[self_spriteGroup].begin() ;
}

void Objects::Gridshow::render()
{

  int gridX = 0;
  int gridY = 0;

  //Drawing loop - start at the current sprite
  Graphics::spriteGroup::iterator drawIter = self_spriteIter;
  //We draw "self_numCells" sprites
  for (int i = 0; i < self_numCells; i++)
  {
    if ( self_coordType == Objects::NON_NORM )
    {
      int cellX = self_x + gridX * self_cellWidth;
      int cellY = self_y + gridY * self_cellHeight;

      drawIter -> second -> draw( cellX, cellY, (int)self_cellWidth, 
                                  (int) self_cellHeight );
    }
    if ( self_coordType == Objects::NORM )
    {
      double cellX = self_x + gridX * self_cellWidth;
      double cellY = self_y + gridY * self_cellHeight;

      drawIter -> second -> draw( cellX, cellY, self_cellWidth, 
                                  self_cellHeight );
    }

    //Do appropriate moving of drawing position
    gridX++;
    if (gridX >= self_cellsWide)
    {
      gridX = 0;
      gridY++;
    }

    //Do appropriate incriment of draw iterator (including looping).
    drawIter++;
    if (drawIter == Graphics::sprites[self_spriteGroup].end())
      drawIter = Graphics::sprites[self_spriteGroup].begin();
  }

  // Increment the timer
  self_time++;
  // If the timer == the timeout then it's time to switch to the new grid
  // The new grid will begin with the sprites after the ones just displayed
  // So we simply set self_spriteIter to drawIter.
  // (Think about it for a second :) )
  if (self_time == self_timeout)
  {
    self_spriteIter = drawIter;
    self_time = 0;
  }
}

Objects::PanSprite::PanSprite( Json::Value data ) :
  Objects::Object( data )
{
  self_sprite = data["sprite"].asString();

  Json::Value dimensions = data["dimensions"];

  if (dimensions["displayW"].isNull() == dimensions["displayH"].isNull() )
  {
    fprintf(stderr, "Both/neither of 'displayW' and 'displayH' are set.\n");
    boinc_close_window_and_quit("Aborting...");
  }

  if ( dimensions["w"].isInt() )
  {
    self_w = dimensions["w"].asInt();
    self_h = dimensions["h"].asInt();

    if ( dimensions["displayW"].isInt() )
    {
      self_displayDim = dimensions["displayW"].asInt();
      self_panAxis = HORIZONTAL;
    }
    if ( dimensions["displayH"].isInt() )
    {
      self_displayDim = dimensions["displayH"].asInt();
      self_panAxis = VERTICAL;
    }
  }

  if ( dimensions["w"].isDouble() )
  {
    self_w = dimensions["w"].asInt();
    self_h = dimensions["h"].asInt();

    if ( dimensions["displayW"].isDouble() )
    {
      self_displayDim = dimensions["displayW"].asDouble();
      self_panAxis = HORIZONTAL;
    }
    if ( dimensions["displayH"].isDouble() )
    {
      self_displayDim = dimensions["displayH"].asDouble();
      self_panAxis = VERTICAL;
    }
  }

  self_panDirection = FORWARDS;

  if ( data["period"].isNull() )
  {
    fprintf(stderr, "PanSprite period is NULL, but required.\n");
  }

  self_panPeriod    = data["period"].asInt();
}

void Objects::PanSprite::render()
{
  using namespace Graphics;
  Sprite* drawnSprite = getSprite( self_sprite );
  int imgW = drawnSprite -> self_imageWidth;
  int imgH = drawnSprite -> self_imageHeight;

  float panFraction = float(self_panTime)/self_panPeriod;

  if ( self_coordType == Objects::NON_NORM )
  {
    int imgX = 0;
    int imgY = 0;
    int drawnW = imgW;
    int drawnH = imgH;

    if ( self_panAxis == HORIZONTAL )
    {
      imgX = panFraction * (imgW - self_displayDim);
      drawnW = self_displayDim;
    }
    if ( self_panAxis == VERTICAL )
    {
      imgY = panFraction * (imgH - self_displayDim);
      drawnH = self_displayDim;
    }

    drawnSprite -> drawArea( (int)self_x, (int)self_y,
                             (int)self_w, (int)self_h,
                             imgX, imgY, drawnW, drawnH );
  }

  if ( self_coordType == Objects::NORM )
  {
    double imgX = 0;
    double imgY = 0;
    double drawnW = 1;
    double drawnH = 1;

    if ( self_panAxis == HORIZONTAL )
    {
      drawnW = self_displayDim;
      imgX = panFraction * (1.0 - drawnW);
    }

    if ( self_panAxis == VERTICAL )
    {
      drawnH = self_displayDim;
      imgY = panFraction * (1.0 - drawnH);
    }

    fprintf(stderr, "imgX:%f, imgW:%f, drawnW:%f, drawnH:%f\n", imgX, (double)imgW, drawnW, drawnH);

    drawnSprite-> drawArea( self_x, self_y, self_w, self_h,
                            imgX,   imgY,   drawnW, drawnH );
  }

  self_panTime += self_panDirection;
  if ( self_panTime == self_panPeriod || self_panTime == 0 )
    self_panDirection = (Direction) -self_panDirection;
}
