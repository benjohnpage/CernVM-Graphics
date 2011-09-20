// Standard
#include <string>
#include <sstream>
#include <iostream>
#include <cstdio>
#include <iterator>

using std::endl;
using std::string;
using std::stringstream;

#include "json/json.h"

//BOINC
#include "graphics2.h"

//Our things
#include "graphics.h"
#include "errors.h"
#include "objects.h"
#include "boincShare.h"
#include "resources.h"
using Graphics::Sprite;


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
  
      // Call correct construtor
      // (NB These will be scattered around different headers because a
      // single header will become too long.)
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
      if (objectData["type"] == "errorDisplay")
        newObj = new Objects::ErrorDisplay(objectData);
      if (objectData["type"] == "debugDisplay")
        newObj = new Objects::DebugDisplay(objectData);
  
      view . push_back( newObj );
    }

    Objects::viewList . push_back( view );
  }
  
  // Set first view to active view (this should ALWAYS exist)
  Objects::activeView = &Objects::viewList[0];
}

void Objects::updateObjects()
{
  using Objects::viewList;
  
  for ( size_t viewIndex = 0; viewIndex < viewList.size(); viewIndex++)
  {
    for ( size_t objIndex = 0; objIndex < viewList[viewIndex].size(); 
          objIndex++ )
    {
      viewList[viewIndex][objIndex] -> update();
    }
    
  }
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
 self_data( data ), self_x(0), self_y(0), self_w(0), self_h(0)
{
  Json::Value dimensions = data["dimensions"];
  if ( dimensions.isObject() )
  {
    // Coordinates are stored as doubles, but interpretted as either
    // pixel coordinates or fractional. If ANY dimension is a double, then
    // they all should be (why would you use a double coordinate otherwise?)

    self_coordType = Objects::NON_NORM;

    for ( Json::ValueIterator itr  = dimensions.begin();
                              itr != dimensions.end();
                              itr ++ )
    {
      string key = itr.key().asString();

      if (dimensions[ key ] . isDouble() )
      {
        self_coordType = Objects::NORM;
        break;
      }
    }

    self_x = dimensions["x"].asDouble();
    self_y = dimensions["y"].asDouble();

  }
}

Errors::StreamFork& Objects::Object::err()
{
  return Errors::err << self_data["type"].asString() << ": ";
}

Errors::StreamFork& Objects::Object::dbg()
{
  return Errors::dbg << self_data["type"].asString() << ": ";
}

void Objects::Object::autoDimensions(Sprite* sprite)
{
  // This function contains a lot of oft used functionality for importing
  // widths and heights for objects. It takes care of automatically
  // extracting missing dimensions from the aspect ratio of images.

  self_w = self_data["dimensions"]["w"].asDouble();
  self_h = self_data["dimensions"]["h"].asDouble();

  //Safety
  if ( (self_w == 0) and (self_h == 0) )
  {
    string objectType = self_data["type"].asString();
    this -> err() << "No dimensions given. Attempting zero dimensions." << endl;
    return;
  }

  // If a dimension is missing, extract it from the aspect ratio
  if ( self_w == 0 || self_h == 0 )
  {
    // Safety
    if ( sprite == NULL )
    {
      this -> err() << "Null sprite provided in autoDimensions." << endl
                    << "Unable to extract dimensions." << endl;
      return;
    }
    // Get a screen based aspect ratio
    double drawnAspect = sprite -> aspectRatio();

    if (self_coordType == Objects::NORM)
      drawnAspect /= Graphics::screenAspect();

    if (self_w == 0) 
      self_w = self_h * drawnAspect;
    if (self_h == 0)
      self_h = self_w / drawnAspect;
  }
}

void Objects::Object::update()
{
  // This is a placeholder function, to allow objects to update with new
  // content. They don't *have* to do this, however - so it's only virtual.
  
  // DESIGN NOTE - Please place error messages here. Error messages in the
  // render function are near useless due to the frequency of the calling.
}

void Objects::Object::keyHandler(int key)
{
  // This is a placeholder function, to allow objects to respond to key 
  // input. They don't *have* to do this, however - so it's only virtual.
}

Objects::Slideshow::Slideshow( Json::Value data ) :
  Objects::Object( data ), self_lastUpdate(0), self_slidePos(0)
{
  self_spriteGroup = data["sprites"].asString();
  
  if ( data["timeout"] . isInt() )
  {
    // Hangover from old code, we choose to make it 30fps
    self_timeout =  (1.0/30)*data["timeout"].asInt();
  }
  else if ( data["timeout"] . isDouble() )
    self_timeout = data["timeout"] . asDouble();
  else
  {
    // Default to 3 second refresh and error
    self_timeout = 3.0;
    this -> err() << "No timeout specified, defaulting to 3 seconds";
  }

  Json::Value dimensions = data["dimensions"];
  if ( dimensions.isString() )
  {
    //If it is a string, dimensions should say "fullscreen". If it doesn't,
    //we pretend it was meant to and complain
    if (dimensions != "fullscreen")
    {
      this -> err() << "Provided dimensions as string, but does not want"
                    << " fullscreen, interpretting as fullscreen anyway."
                    << endl;
    }

    self_x = -0.5f;
    self_y = -0.5f;
    self_w = 1.0;
    self_h = 1.0;
   
    self_coordType = Objects::NORM;
  }
  else
    // This handles the dimensions, which need to change on update too
    this -> update(); 
}


void Objects::Slideshow::render( double timestamp )
{
  using Graphics::spriteGroup;
  using Graphics::sprites;
  using std::advance;

  // Can't do the initialisation of self_lastUpdate anywhere else
  if ( self_lastUpdate == 0 )
    self_lastUpdate = timestamp;

  // Iterate slides
  if ( self_timeout > 0 and timestamp - self_lastUpdate > self_timeout)
  {
    self_lastUpdate = timestamp;
    self_slidePos++;

    if ( self_slidePos == Graphics::sprites[ self_spriteGroup ] . size() )
      self_slidePos = 0;
  }

  spriteGroup::iterator drawIter = sprites[ self_spriteGroup ].begin();
  advance( drawIter, self_slidePos );
  

  // If there's a sprite at our iterator. (If the group is empty then 
  // begin() is also end() and so not a sprite)
  if ( drawIter != sprites[ self_spriteGroup ] . end() )
  {
    if ( self_coordType == Objects::NORM )
      drawIter -> second -> draw(self_x, self_y, self_w, self_h);

    if ( self_coordType == Objects::NON_NORM )
      drawIter -> second -> draw( (int)self_x, (int)self_y, 
                                         (int)self_w, (int)self_h);
  }
}

void Objects::Slideshow::update()
{
  using Graphics::sprites;
  // Safety, don't calculate dimensions for an empty group
  if ( sprites[ self_spriteGroup ] . size() != 0 )
    // We assume that the sprites in slideshow are all the same size
    this -> autoDimensions(sprites[ self_spriteGroup ].begin() -> second);

  // Protection, to stop std::advance hanging if it invalidates an iterator.
  // Note, if you still get occasional hangs it's possible that this should be
  // >=
  if ( self_slidePos > sprites[ self_spriteGroup ] . size() )
    self_slidePos = 0;
}

Objects::BoincValue::BoincValue(Json::Value data) : 
  Objects::Object( data )
{
  self_prefix = data["prefix"].asString();
  self_type   = data["valueType"].asString();
}

void Objects::BoincValue::render( double timestamp )
{
  //The assumption is that the shared memory is updated regularly in the
  //graphics loop

  stringstream output;
  output << self_prefix;

  if (Share::data == NULL)
  {
    output << "NO SHMEM";
  }
  else
  {
    if (self_type == "username")
      output << Share::data -> init_data . user_name;
    if (self_type == "credit")
      output << Share::data -> init_data . user_total_credit;
  }

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

  // This processes the string data
  this -> update();
}

void Objects::StringDisplay::render( double timestamp )
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

void Objects::StringDisplay::update()
{
  // Make sure repeated calls to this function don't simply add strings
  self_displayStrings.clear();

  Json::Value strings;
  strings = self_data["strings"];

  if (self_data["external"].isBool())
    if (self_data["external"] == true)
    {
      string resource = self_data["resource"].asString();
      string node     = self_data["node"].asString();
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
    if ( lineN == self_maxLines )
    {
      self_displayStrings.push_back(outputStream.str());
      lineN = 0;
      outputStream.str("");
    }

  }
  //Add remaining string
  self_displayStrings.push_back( outputStream.str() );
}

Objects::SpriteDisplay::SpriteDisplay(Json::Value data) :
  Objects::Object( data )
{
  self_spriteName = data["sprite"].asString();

  Json::Value dimensions = data["dimensions"];

  this -> autoDimensions( Graphics::getSprite( self_spriteName ) );

}

void Objects::SpriteDisplay::render( double timestamp )
{
  Sprite* drawSprite = Graphics::getSprite( self_spriteName );

  if ( self_coordType == Objects::NON_NORM )
    drawSprite -> draw( (int)self_x, (int)self_y, (int)self_w, 
                        (int)self_h );

  if ( self_coordType == Objects::NORM )
    drawSprite -> draw( self_x, self_y, self_w, self_h );
}

Objects::Gridshow::Gridshow(Json::Value data) :
  Objects::Object( data ), self_lastUpdate(0), self_slidePos(0)
{
  self_spriteGroup = data["sprites"].asString();

  Json::Value dimensions = data["dimensions"];

  // This calls the automatic dimension functions
  this -> update();

  //Cell settings
  self_cellsWide  = dimensions["cellsWide"].asInt();
  self_numCells   = data["numCells"].asInt();

  if ( data["timeout"] . isInt() )
  {
    // Hangover from old code, we choose to make it 30fps
    self_timeout =  (1.0/30)*data["timeout"].asInt();
  }
  else if ( data["timeout"] . isDouble() )
    self_timeout = data["timeout"] . asDouble();
  else
  {
    // Default to no refresh
    self_timeout = 0.0;
  }


}

void Objects::Gridshow::render( double timestamp )
{
  // Can't set it to current time for first time anywhere else.
  if ( self_lastUpdate == 0 )
    self_lastUpdate = timestamp;

  // This loops the slides. 
  if ( self_timeout > 0 and timestamp - self_lastUpdate > self_timeout)
  {
    self_slidePos += self_numCells;
    size_t groupSize = Graphics::sprites[self_spriteGroup].size();
    if ( self_slidePos  >= groupSize )
      self_slidePos = 0;
    
    self_lastUpdate = timestamp;
  }

  using Graphics::Sprite;
  using Graphics::sprites;
  using Graphics::spriteGroup;
  using std::advance;

  //Drawing loop - start at the current sprite
  spriteGroup::iterator drawIter = sprites[self_spriteGroup].begin();
  advance( drawIter, self_slidePos );
 
  int gridX = 0;
  int gridY = 0;

  //We draw "self_numCells" sprites
  for (int i = 0; i < self_numCells; i++)
  {
    // If the iterator no longer gives us a real sprite, then break
    if ( drawIter == Graphics::sprites[self_spriteGroup] . end() )
      break;

    Sprite* cellSprite = drawIter -> second;

    // No need for error message, as you should already know that it's NULL
    // by previous errors
    if ( cellSprite == NULL )
      continue;
    
    if ( self_coordType == Objects::NON_NORM )
    {
      int cellX = self_x + gridX * self_w;
      int cellY = self_y + gridY * self_h;

      cellSprite -> draw( cellX, cellY, (int)self_w, (int) self_h );
    }
    if ( self_coordType == Objects::NORM )
    {
      double cellX = self_x + gridX * self_w;
      double cellY = self_y + gridY * self_h;

      cellSprite -> draw( cellX, cellY, self_w, self_h );
    }

    //Do appropriate moving of drawing position
    gridX++;
    if (gridX >= self_cellsWide)
    {
      gridX = 0;
      gridY++;
    }

    // Move to next sprite
    drawIter++;
  }

}

void Objects::Gridshow::update()
{
  using Graphics::sprites;
  if ( sprites[ self_spriteGroup ] . size() == 0 )
    this -> err() << "Sprite group is empty, cannot calculate dimensions." 
                  << endl;
  else
    this -> autoDimensions( sprites[ self_spriteGroup ].begin() -> second );

  // Protection, stops std::advance hitting an infinite loop (this happens if
  // std::advance ends up invalidating our iterators)
  // Note, if you still get occasional hangs it's possible that this should be
  // >=
  if ( self_slidePos > sprites[self_spriteGroup].size() )
    self_slidePos = 0;
  
}

Objects::PanSprite::PanSprite( Json::Value data ) :
  Objects::Object( data ), self_panTime(0)
{
  self_sprite = data["sprite"].asString();

  Json::Value dimensions = data["dimensions"];


  self_w = dimensions["w"].asDouble();
  self_h = dimensions["h"].asDouble();

  if (dimensions["displayW"].isNull() == dimensions["displayH"].isNull() )
  {
    this -> err() << "Both/neither of 'displayW' and 'displayH' are set." 
                << endl
                << "Setting displayW to dimensions[w]" << endl;
    dimensions["displayW"] = dimensions["w"].asDouble();
  }

  if ( !dimensions["displayW"].isNull() )
  {
    self_displayDim = dimensions["displayW"].asDouble();
    self_panAxis = HORIZONTAL;
  }
  if ( !dimensions["displayH"].isNull() )
  {
    self_displayDim = dimensions["displayH"].asDouble();
    self_panAxis = VERTICAL;
  }

  self_panDirection = FORWARDS;

  self_panPeriod    = data["period"].asInt();

  if ( data["period"].isNull() )
  {
    this -> err() << "PanSprite period is NULL, but required." << endl
                  << "Setting period to 100 frames" << endl;
    self_panPeriod = 100;
  }

}

void Objects::PanSprite::render( double timestamp )
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

    drawnSprite-> drawArea( self_x, self_y, self_w, self_h,
                            imgX,   imgY,   drawnW, drawnH );
  }

  self_panTime += self_panDirection;
  if ( self_panTime == self_panPeriod || self_panTime == 0 )
    self_panDirection = (Direction) -self_panDirection;
}

