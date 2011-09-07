#include "graphics.h"

#include <cstdio>
#include <sstream>

using std::stringstream;

//LibPNG
#include <png.h>

//BOINC
#include "boinc_api.h"
#include "filesys.h"
#include "boinc_gl.h"
#include "txf_util.h"


//Simple helper functions
double Graphics::screenAspect()
{
  // Accepts an original width height aspect ratio (assumedly of an image)
  // and converts it to a screen fraction one.
  return 16.0/10.0;
}

void Graphics::drawableWindow( int& width, int& height )
{
  //get Window Dimensions
  int viewPort[4];
  glGetIntegerv(GL_VIEWPORT, viewPort);

  width  = viewPort[2];
  height = viewPort[3];

  // Fix to screen aspect ratio
  
  // Calculate the 16:10 height if width is true
  int aspectHeight = width / Graphics::screenAspect();

  //if this is > that the actual height then height takes presidence
  if (aspectHeight > height)
  {
    width = height * Graphics::screenAspect();
  }
  else
  {
    height = width / Graphics::screenAspect();
  }
}
void Graphics::drawText( string text, double xFrac, double yFrac, 
                         float * colour )
{
  // Double coordinates interpretted as fractional window coordinates
  
  // Get window dimensions
  int windowW, windowH; 
  Graphics::drawableWindow(windowW, windowH);

  int x = xFrac * windowW;
  int y = yFrac * windowH;

  Graphics::drawText( text, x, y, colour );
}

void Graphics::drawText(string text, int x, int y, float * colour)
{
  //Default to black
  float black[] = {0.0, 0.0, 0.0, 1.0};
  if (colour == NULL) colour = black;

  int windowW, windowH;
  Graphics::drawableWindow( windowW, windowH );

  int lineHeightConstant = 25*1.4; //These two work, so guess inv proportion
  double textScaleConstant = 540*1.4; //Same as above

  double textScale = textScaleConstant / windowH;
  int lineHeight = lineHeightConstant / textScale; 

  char buffer[256];
  stringstream textStream(text);

  //This loop is required because txf_render_string doesn't deal with \n
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  while (textStream.getline(buffer, 256))
  {
    txf_render_string(0.1, x, y, 0, textScale, colour, 0, buffer);
    y -= lineHeight;
  }
  glDisable(GL_BLEND);
}

//PNG Loading
bool Graphics::loadPng(string filename, int& outWidth, int& outHeight, 
                       bool& outHasAlpha, GLubyte** outData)
{
  //Simple PNG loading function that puts data straight into the arguments,
  //whilst reordering the pixel data into a format that openGL understands.
  //(That is it stops it from putting the image upside down)

  //We begin with a mass of initialisation
  png_structp imageStruct;
  png_infop   imageInfo;


  string resolvedFile;
  boinc_resolve_filename_s(filename.c_str(), resolvedFile);
  FILE * imageFile = boinc_fopen(resolvedFile.c_str(), "rb");
  if (imageFile == NULL)
    return false;

  imageStruct = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL,
                                       NULL);
  if (imageStruct == NULL)
  {
    fclose(imageFile);
    return false;
  }

  imageInfo = png_create_info_struct(imageStruct);
  if (imageInfo == NULL)
  {
    fclose(imageFile);
    png_destroy_read_struct(&imageStruct, png_infopp_NULL, png_infopp_NULL);
    return false;
  }

  //Error handling
  if (setjmp(png_jmpbuf(imageStruct)))
  {
    png_destroy_read_struct(&imageStruct, &imageInfo, png_infopp_NULL);
    fclose(imageFile);
    return false;
  }

  //Initialisation is now finished, time to start the IO

  png_init_io(imageStruct, imageFile);
  int signatureRead = 0; //Something to do with the PNG signature
  png_set_sig_bytes(imageStruct, signatureRead);

  //Read the PNG into imageStruct!
  png_read_png(imageStruct, imageInfo, PNG_TRANSFORM_STRIP_16 | 
               PNG_TRANSFORM_PACKING | PNG_TRANSFORM_EXPAND, 
               png_voidp_NULL);
  //Passing back info as arguments

  outWidth  = imageInfo->width;
  outHeight = imageInfo->height;

  if (imageInfo->color_type == PNG_COLOR_TYPE_RGBA)
    outHasAlpha = true;
  else if (imageInfo->color_type == PNG_COLOR_TYPE_RGB)
    outHasAlpha = false;
  else
  {
    fprintf(stderr, "Unsupported colour type in PNG loading. Load fail");
    png_destroy_read_struct(&imageStruct, &imageInfo, NULL);
    fclose(imageFile);
    return false;
  }

  //Load the pixel data! Finally!
  unsigned int bytesPerRow = png_get_rowbytes(imageStruct, imageInfo);
  *outData = (unsigned char *) malloc( bytesPerRow * outHeight );

  png_bytepp rowPointers = png_get_rows(imageStruct, imageInfo);

  for(int i = 0; i < outHeight; i++)
  {
    memcpy( *outData + (bytesPerRow * (outHeight - 1 - i)), rowPointers[i],
             bytesPerRow);
  }

  //Clean up
  png_destroy_read_struct(&imageStruct, &imageInfo, png_infopp_NULL);
  fclose(imageFile);
  //And go home
  return true;
}


//2D environment functions
//(I do not know enough at time of writing to explain what these do, but the//do work)

void Graphics::begin2D()
{
  //Get window dimensions
  int viewPort[4]; 
  glGetIntegerv(GL_VIEWPORT, viewPort);

  //Set it up so we affect the projection matrix (but can get it back later)
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();

  //Use an orthogonal projection with a cartesian coordinate system
  //based upon the window dimensions
  int left   = -viewPort[2]/2;
  int right  =  viewPort[2]/2;
  int bottom = -viewPort[3]/2;
  int top    =  viewPort[3]/2;
  glOrtho(left, right, bottom, top, -1, 1);

  //We require the modelview matrix to be the identity
  //NOTE - There is no pushing of the matrix here, I do not know why, and
  //may have missed one out.
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
}

void Graphics::end2D()
{
  //Reclaim the old projection and model matrices
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
}
