#ifndef RESOURCES_H_INC
#define RESOURCES_H_INC

#include <string>
#include <map>

#include "json/json.h"

namespace Resources
{
  typedef std::map<std::string, Json::Value> ResourcesMap;
  extern ResourcesMap resourcesMap;
  Json::Value getResourceNode(std::string resourceName, std::string node);
  
  ResourcesMap loadResources(Json::Value resources);
};

#endif //Include guard
