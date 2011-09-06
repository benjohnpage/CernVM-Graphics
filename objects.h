#ifndef OBJECTS_H_INC
#define OBJECTS_H_INC

//Ours
#include "graphics.h"
//JsonCpp
#include <json/json.h>

//Standard
#include <string>
#include <vector>
#include <map>

using std::string;
using std::vector;
using std::map;

namespace Objects
{
  void loadObjects(Json::Value objects);
  void removeObjects();

  enum CoordType { NORM, NON_NORM };

  class Object
  {
    public:
      Object(Json::Value data);
      virtual void render() = 0;
    protected:
      Json::Value self_data;
      double      self_x;
      double      self_y;
      CoordType   self_coordType;
  };

  class BoincValue: public Object
  {
    public:
      BoincValue(Json::Value data);
      void render();
    private:
      string self_type;
      string self_prefix;
  };

  class StringDisplay: public Object
  {
    public:
      StringDisplay(Json::Value data);
      void render();
    private:
      vector<string> self_displayStrings;
      string         self_delimiter;
      int            self_maxLines;
      int            self_lineWidth;
  };

  class SpriteDisplay: public Object
  {
    public:
      SpriteDisplay(Json::Value data);
      void render();
    private:
      double    self_w;
      double    self_h;
      string self_spriteName;
  };

  class Slideshow: public Object
  {
    public:
      Slideshow(Json::Value data);
      void render();
    private:
      double  self_w;
      double  self_h;

      int self_time;
      int self_timeout;
      
      string self_spriteGroup;
      Graphics::spriteGroup::iterator self_spriteIter;
  };

  class Gridshow: public Object
  {
    public:
      Gridshow(Json::Value data);
      void render();
    private:
      double self_cellWidth;
      double self_cellHeight;
      int self_cellsWide;
      int self_numCells;

      int self_time;
      int self_timeout;
      
      string self_spriteGroup;
      Graphics::spriteGroup::iterator self_spriteIter;
  };

  class PanSprite : public  Object
  {
    public: 
      PanSprite(Json::Value data);
      void render();
    private:
      string self_sprite;
      double self_w;
      double self_h;

      int self_panTime;
      int self_panPeriod;

      enum Axis { VERTICAL, HORIZONTAL };
      Axis self_panAxis;

      //This enables maths trickery
      enum Direction { FORWARDS = 1, BACKWARDS = -1 };
      Direction self_panDirection;

      double self_displayDim;
  };

  typedef vector< Objects::Object* >    View;
  typedef vector< Objects::View > ViewList;
  extern ViewList viewList;
  extern View*    activeView;
};

#endif
