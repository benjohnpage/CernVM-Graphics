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

namespace Objects
{
  void loadObjects(Json::Value objects);
  void updateObjects();
  void removeObjects();

  enum CoordType { NORM, NON_NORM };

  class Object
  {
    public:
      Object(Json::Value data);
      virtual void update();
      virtual void render() = 0;
    protected:
      Json::Value self_data;
      double      self_x;
      double      self_y;
      CoordType   self_coordType;

      double    self_w;
      double    self_h;
      void      autoDimensions(Graphics::Sprite* sprite);
  };

  class BoincValue: public Object
  {
    public:
      BoincValue(Json::Value data);
      void render();
    private:
      std::string self_type;
      std::string self_prefix;
  };

  class StringDisplay: public Object
  {
    public:
      StringDisplay(Json::Value data);
      void update();
      void render();
    private:
      std::vector<std::string> self_displayStrings;
      std::string         self_delimiter;
      int            self_maxLines;
      int            self_lineWidth;
  };

  class SpriteDisplay: public Object
  {
    public:
      SpriteDisplay(Json::Value data);
      void render();
    private:
      std::string self_spriteName;
  };

  class Slideshow: public Object
  {
    public:
      Slideshow(Json::Value data);
      void update();
      void render();
    private:
      int self_time;
      int self_timeout;
      
      std::string self_spriteGroup;
      size_t self_slidePos;
  };

  class Gridshow: public Object
  {
    public:
      Gridshow(Json::Value data);
      void update();
      void render();
    private:
      int self_cellsWide;
      int self_numCells;

      int self_time;
      int self_timeout;
      
      std::string self_spriteGroup;
      size_t self_slidePos;
  };

  class PanSprite : public  Object
  {
    public: 
      PanSprite(Json::Value data);
      void render();
    private:
      std::string self_sprite;

      int self_panTime;
      int self_panPeriod;

      enum Axis { VERTICAL, HORIZONTAL };
      Axis self_panAxis;

      //This enables maths trickery
      enum Direction { FORWARDS = 1, BACKWARDS = -1 };
      Direction self_panDirection;

      double self_displayDim;
  };

  typedef std::vector< Objects::Object* > View;
  typedef std::vector< Objects::View >    ViewList;
  extern ViewList viewList;
  extern View*    activeView;
};

#endif
