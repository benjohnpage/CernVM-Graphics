//Our stuff
#include "resources.h"
#include "boincShare.h"
#include "networking.h"
#include "errors.h"

//Json
#include "json/json.h"

//Boinc
#include "graphics2.h"

//Std
#include <string>
#include <fstream>
#include <iostream>
#include <map>

using std::string;
using std::ifstream;
using std::map;
using std::endl;

Resources::ResourcesMap Resources::resourcesMap;

Json::Value Resources::getResourceNode( string resourceName, string node )
{
  return Resources::resourcesMap[ resourceName ][ node ];
}

Resources::ResourcesMap Resources::loadResources( Json::Value resources )
{
  using namespace Networking;

  ResourcesMap newResourceMap;

  // Provided are (name, file) pairs, so load them in
  for (Json::ValueIterator itr  = resources.begin(); 
                           itr != resources.end(); 
                           itr++)
  {
    string resourceName = itr.key().asString();
    string netResourceFilename = resources[ resourceName ].asString();

    // Download file
    string localFilename = fileDownloader->getFile(netResourceFilename);
    ifstream resourceFile( localFilename . c_str() );

    // Parse
    Json::Value resource;
    Json::Reader reader;
    bool parsingSuccessful = reader.parse( resourceFile, resource );
    if (!parsingSuccessful)
    {
      Errors::err << "Provided JSON file is invalid JSON." << endl 
           << Errors::fatal;
    }

    // Save in memory
    newResourceMap[ resourceName ] = resource;
  }

  return newResourceMap;
}
