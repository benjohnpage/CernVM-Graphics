////////////////////////////////////////////////////////////////////////////
// Sprites.cpp:
//
// Contains all of the classes/functions declared in graphics.h that are to
// do with the Sprite class
////////////////////////////////////////////////////////////////////////////

//Ours
#include "graphics.h"
#include "boincShare.h"
#include "networking.h"
#include "resources.h"

//JsonCPP
#include "json/json.h"

//BOINC
#include "boinc_gl.h"
#include "graphics2.h"

//Standard
#include <map>
#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <stdexcept>

using std::map;
using std::stringstream;
using std::out_of_range;

Graphics::spriteGroupMap Graphics::sprites;

bool isPowerOfTwo(int x){ return  (x != 0) && ((x & (x-1)) == 0); }

//Functioned sprite access 
//This is to add checking protection and if we change the standard it's only
//in one place

Graphics::Sprite* Graphics::getSprite(string groupName, string spriteName)
{
  Graphics::spriteGroupMap::iterator groupItr = 
                                          Graphics::sprites.find(groupName);
  if (groupItr == Graphics::sprites.end())
  {
    fprintf(stderr, "The requested group \"%s\" does not exist", 
            groupName.c_str());
    throw out_of_range("Unknown group"); 
  }

  Graphics::spriteGroup::iterator spriteItr = groupItr 
                                             -> second . find(spriteName);
  if (spriteItr == groupItr -> second.end())
  {
    fprintf(stderr, 
            "The requested sprite \"%s\", in group \"%s\" does not exist", 
            spriteName.c_str(), groupName.c_str());
    throw out_of_range("Unknown sprite in group"); 
  }
  return spriteItr->second;
}

Graphics::Sprite* Graphics::getSprite(string spriteName)
{
  return Graphics::getSprite("__main__", spriteName);
}

////////////////////////
//Sprite Class Methods//
////////////////////////

Graphics::Sprite::Sprite()
{
  self_texture = 0;
  self_textureWidth = 0;
  self_textureHeight = 0;
  self_imageWidth = 0;
  self_imageHeight = 0;
  self_textureHasAlpha = false;
  self_textureTarget = GL_TEXTURE_2D;
}

Graphics::Sprite::Sprite(string filename)
{
  fprintf(stderr, "Creating sprite from %s \n", filename.c_str() );
  int width, height;
  bool hasAlpha;
  GLubyte* texturePointer;

  //Load the image
  bool successfulLoad = false;
  string errorMessage = "";
  size_t fileExtensionPos = filename.find_last_of(".") + 1;
  string fileExtension = filename.substr(fileExtensionPos); 

  if (fileExtension == "png")
    successfulLoad = Graphics::loadPng(filename, width, height, hasAlpha,
                                       &texturePointer);
  else 
    errorMessage = "File type \"" + fileExtension + "\" not supported.\n";


  if (!successfulLoad)
  {
    errorMessage += "Unable to load texture file.\n";
    errorMessage += "Filename: " + filename + "\n";
    throw errorMessage.c_str();
  }

  //Set up normal parameters
  self_imageWidth    = width;
  self_imageHeight   = height;
  self_textureHasAlpha = hasAlpha;

  //OpenGL Version dependancy "fixes"
  //Get the version
  stringstream versionStream;
  float openGLVersion;
  versionStream << glGetString(GL_VERSION);
  versionStream >> openGLVersion;
  
  //NPOTS textures are only supported by extension before openGL 2
  if (openGLVersion < 2 and (!isPowerOfTwo(width) or !isPowerOfTwo(height)))
  {
    self_textureTarget = GL_TEXTURE_RECTANGLE_ARB;
    self_textureWidth  = width;
    self_textureHeight = height;
  }
  else
  {
    //Otherwise we use:
    self_textureTarget = GL_TEXTURE_2D;
    //Which uses normalised coordinates:
    self_textureWidth  = 1.0;
    self_textureHeight = 1.0;
  }

  ///
  /// Begin texture setup
  ///

  //Create OpenGL texture and set it to the current one
  glGenTextures(1, &self_texture);
  glBindTexture(self_textureTarget, self_texture);

  //This sets up the openGL texture (most work being done in glTexImage2D)
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  GLint internalFormat = hasAlpha ? 4 : 3;
  GLenum imageFormat    = hasAlpha ? GL_RGBA : GL_RGB;

  glTexImage2D(self_textureTarget, 0, internalFormat, self_imageWidth, 
               self_imageHeight, 0, imageFormat, GL_UNSIGNED_BYTE, 
               texturePointer);

  //Clamp any out of bounds requests to the texture
  glTexParameterf(self_textureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameterf(self_textureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP);

  //Use linear interpolation for texture scaling
  glTexParameterf(self_textureTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameterf(self_textureTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

  // OpenGL has made it's own copy of the image data, so we're free to get
  // rid of it.
  free( texturePointer );
}

Graphics::Sprite::~Sprite()
{
  glDeleteTextures(1, &self_texture);
}

void Graphics::Sprite::draw( double xFrac, double yFrac, double wFrac,
                             double hFrac )
{
  // Double corodinates are treated as fractional coordinates of the window
  // NOTE - origin is middle of window.

  // Get window dimensions
  int windowW, windowH; 
  Graphics::drawableWindow( windowW, windowH );

  int x = xFrac * windowW;
  int w = wFrac * windowW;
  int y = yFrac * windowH;
  int h = hFrac * windowH;

  // Now draw as integer coordinates
  this -> draw( x, y, w, h );
}

void Graphics::Sprite::draw(int x, int y, int width, int height)
{
  // Provided parameters are integer coordinates representing on
  // screen pixel coordinates

  // If no width/height stated, assume natural dimensions
  if (width  == 0) width  = self_imageWidth;
  if (height == 0) height = self_imageHeight;

  this -> blit( x, y, width, height, 0.0, 0.0, 1.0, 1.0 );
}

void Graphics::Sprite::drawArea(int xScr, int yScr, int wScr, int hScr, 
                                int xImg, int yImg, int wImg, int hImg)
{
  // We first convert the image coordinates to fractions of the image size
  // as  this is what we feed to glTexCoord2f

  double xImgFrac = xImg / self_imageWidth;
  double yImgFrac = yImg / self_imageHeight;
  double wImgFrac = wImg / self_imageWidth;
  double hImgFrac = hImg / self_imageHeight;

  this -> blit( xScr, yScr, wScr, hScr, xImgFrac, yImgFrac, wImgFrac, 
                hImgFrac );
}

void Graphics::Sprite::drawArea( double xFrac, double yFrac, 
                                 double wFrac, double hFrac, 
                                 double xImgFrac, double yImgFrac, 
                                 double wImgFrac, double hImgFrac )
{
  // Double corodinates are treated as fractional coordinates of the window
  // NOTE - origin is middle of window.

  // Get window dimensions
  int windowW, windowH; 
  Graphics::drawableWindow( windowW, windowH );

  int x = xFrac * windowW;
  int w = wFrac * windowW;
  int y = yFrac * windowH;
  int h = hFrac * windowH;

  // Now draw as integer coordinates
  this -> blit( x, y, w, h, xImgFrac, yImgFrac, wImgFrac, hImgFrac );
}

void Graphics::Sprite::blit( int xScr, int yScr, int wScr, int hScr,
                              double xTex, double yTex, double wTex,
                                                        double hTex )
{
  glBindTexture(self_textureTarget, self_texture);

  //Make sure that we're in the right setup
  glEnable(self_textureTarget);
  glShadeModel(GL_FLAT); 

  //Texture quad environment
  glBegin(GL_QUADS);

    glColor4f(1.0, 1.0, 1.0, 1.0);

    glTexCoord2f( xTex * self_textureWidth,  
                  yTex * self_textureHeight );
    glVertex2f(xScr, yScr);

    
    glTexCoord2f( xTex * self_textureWidth, 
                  (yTex + hTex) * self_textureHeight ); 
    glVertex2f(xScr, yScr + hScr);


    glTexCoord2f( (xTex + wTex) * self_textureWidth,
                  (yTex + hTex) * self_textureHeight );
    glVertex2f(xScr + wScr, yScr + hScr);


    glTexCoord2f( (xTex + wTex) * self_textureWidth,
                  yTex * self_textureHeight );
    glVertex2f(xScr + wScr, yScr);

  glEnd();

  glDisable(self_textureTarget);
}

///////////////////
// Sprite Loader //
///////////////////

void Graphics::loadSprites(Json::Value sprites)
{
  if (sprites["external"].isBool())
    if (sprites["external"] == true)
    {
      string resource = sprites["resource"].asString();
      string node = sprites["node"].asString();
      sprites = Resources::getResourceNode(resource, node);
    }
  
  //Loads a series of sprite groups.
  for (Json::ValueIterator itr = sprites.begin(); 
       itr != sprites.end();
       itr++)
  {
    //Get and iterate over the group
    string groupName = itr.key().asString();
    Json::Value group = sprites[groupName];
    for (size_t i = 0; i < group.size(); i++)
    {
      //Get the sprite info
      //(Currently just file and possibly name - room for future expansion)
      Json::Value spriteData = group[i];
      string offsiteFilename;
      string internalName;

      //Accepted type 1 - just a string of the filename
      if (spriteData.isString())
      {
        offsiteFilename = spriteData.asString();

        //Internal naming for nameless sprites is __NUM__ where NUM depends
        //on the order of loading
        stringstream spriteNameStream;
        spriteNameStream << "__" << i << "__";
        internalName = spriteNameStream.str();
      }
      //Accepted type 2 - object containing more data
      else if(spriteData.isObject())
      {
        offsiteFilename = spriteData["file"].asString();
        internalName    = spriteData["name"].asString();
      }

      //Get the file from offsite
      //NOTE - synchronous call, potential for blocking
      using namespace Networking;
      string localFilename = fileDownloader->getFile( offsiteFilename );

      //Try to create a new sprite
      //NOTE - Sprites are loaded with no thought to their lifetime
      Graphics::Sprite* newSprite;
      try
      {
        newSprite = new Graphics::Sprite( localFilename );
      }
      catch (const char* msg)
      {
        fprintf(stderr, "%s \n", msg);
        boinc_close_window_and_quit("Aborted.");
      }
      
      //Add sprite to group it's respective group with appropriate name
      Graphics::sprites[groupName][internalName] = newSprite;
      }
  }
}

void Graphics::removeSprites()
{
  using Graphics::sprites;
  using Graphics::spriteGroupMap;
  using Graphics::spriteGroup;

  //Iterate over groups
  for (spriteGroupMap::iterator groupItr = sprites.begin();
       groupItr != sprites.end();
       groupItr++)
  {
    //Iterate over sprites in group
    for(spriteGroup::iterator spriteItr = groupItr -> second . begin();
        spriteItr != groupItr -> second . end();
        spriteItr++)
    {
      delete spriteItr -> second;
      spriteItr -> second = NULL; //Unnecessary but good practice
    }
    //Clear the group
    groupItr -> second . clear();
  }

  //Clear the group map
  sprites.clear();
}
