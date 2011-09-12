#ifndef GRAPHICS_H_INC
#define GRAPHICS_H_INC

//JsonCpp
#include "json/json.h"

//Standard
#include <string>
#include <map>

//OpenGL
#include "boinc_gl.h"

//The main namespace
namespace Graphics
{
  void drawText( std::string text, int x, int y, float* colour = NULL );
  void drawText( std::string text, double xFrac, double yFrac, 
                 float* colour = NULL );

  bool loadPng(std::string filename, int& outWidth, int& outHeight, 
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
  
      Sprite(std::string filename);
      
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

  typedef std::map<std::string, Sprite*>     spriteGroup;
  typedef std::map<std::string, spriteGroup> spriteGroupMap;

  //Globals
  extern spriteGroupMap sprites;

  void loadSprites(Json::Value);
  void removeSprites();
  Sprite* getSprite(std::string spriteName);
  Sprite* getSprite(std::string groupName, std::string spriteName);
  
}

#endif //INCLUDE GUARD
