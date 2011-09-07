#ifndef GRAPHICS_H_INC
#define GRAPHICS_H_INC

//JsonCpp
#include "json/json.h"

//Standard
#include <string>
#include <map>

using std::string;
using std::map;

//OpenGL
#include "boinc_gl.h"

//The main namespace
namespace Graphics
{
  void drawText( string text, int x, int y, float* colour = NULL );
  void drawText( string text, double xFrac, double yFrac, 
                 float* colour = NULL );

  bool loadPng(string filename, int& outWidth, int& outHeight, 
               bool& outHasAlpha, GLubyte** outData);
  
  void drawableWindow( int& windowW, int& windowH );
  double screenAspect();

  void begin2D();
  void end2D();

  class Sprite
  {
    private:
      GLuint self_texture;
      GLenum self_textureTarget;

      // texture width and height, depend on whether or not this is using
      // normalised or non-normalised coordinates
      int  self_textureWidth;
      int  self_textureHeight;

    public:
      // original image width and height, in pixels
      int  self_imageWidth;
      int  self_imageHeight;

      bool self_textureHasAlpha;
  
      Sprite();
      ~Sprite();
  
      Sprite(string filename);
      
      void blit( int    xScr, int    yScr, int    wScr, int    hScr,
                 double xTex, double yTex, double wTex, double hTex );
      void draw(int x, int y, int width, int height);
      void draw(double xFrac, double yFrac, double wFrac, double hFrac);
      void drawArea(int xScr, int yScr, int wScr, int hScr,
                    int xImg, int yImg, int wImg, int hImg);
      void drawArea(double xScr, double yScr, double wScr, double hScr,
                    double xImg, double yImg, double wImg, double hImg);

      double aspectRatio(); //Width to height
  };

  typedef map<string, Sprite*>     spriteGroup;
  typedef map<string, spriteGroup> spriteGroupMap;

  //Globals
  extern spriteGroupMap sprites;

  void loadSprites(Json::Value);
  void removeSprites();
  Sprite* getSprite(string spriteName);
  Sprite* getSprite(string groupName, string spriteName);
  
}

#endif //INCLUDE GUARD
