#ifndef RESOURCES_H_INC
#define RESOURCES_H_INC

#include <string>
#include <map>

using std::string;
using std::map;

#include "json/json.h"

namespace Resources
{
  typedef map<string, Json::Value> ResourcesMap;
  extern ResourcesMap resourcesMap;
  Json::Value getResourceNode(string resourceName, string node);
  
  ResourcesMap loadResources(Json::Value resources);
};

#endif //Include guard
