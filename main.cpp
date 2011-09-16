////////////////////////////////////////////////////////////////////////////
// Test4Theory@Home screensaver
//
// Spec:
// - 16x10 window that can scale to fullscreen, keeping that aspect ratio
// - Natural resolution - 960*600
//
// Notes:
// - "GLuint"s appear to be a pointer object of some sort. At least, they're
//   used as handles
// - OpenGL supports no file loading, seemingly, only raw pixel data.
// - PNG is used, so linking against libpng is necessary.
// - BOINC handles the --fullscreen option, this was not obvious. Both the
//   fullscreening AND input closing the window are handled by BOINC here.

//Options
#define WINDOW_TITLE "LHC@Home 2.0"


#include <cstdlib>
#include <string>
#include <fstream>

using std::string;
using std::ifstream;
using std::endl;

//JsonCpp
#include <json/json.h>

//cURl
#include <curl/curl.h>

//BOINC
#include "graphics2.h"
#include "diagnostics.h"
#include "boinc_api.h"
#include "boinc_gl.h" //This handles multiplatform openGL stuff
#include "txf_util.h"

//Our stuff
#include "graphics.h"
#include "objects.h"
#include "boincShare.h"
#include "resources.h"
#include "networking.h"
#include "errors.h"

//Global variables
Share::SharedData* Share::data;

double timeOfUpdate;
double updatePeriod; //Defaults to 0

string forcedConfigFile;
Json::Value appConfig;

void updateConfiguration( CURL* indexHandle )
{
  //Open config file (default name "/index.json")
  string indexFilename;
  
  // Or have we been given a forced config file at run time?
  if (forcedConfigFile != "")
  {
    Errors::dbg << "Using forced config file: " << forcedConfigFile << endl;
    indexFilename = forcedConfigFile;
  }
  else
  {
    indexFilename = "./dispFiles/index.json";
  }

  ifstream jsonFile( indexFilename.c_str() ); 

  if (!jsonFile)
  {
    Errors::err << "Config file " << indexFilename << " not found." << endl
                << "Keeping previous configuration";
    return;
  }


  //Parse config as Json
  Json::Reader reader;
  Json::Value newConfig;
  bool parsingSuccessful = reader.parse(jsonFile, newConfig);
  if (!parsingSuccessful)
  {
    Errors::err << "Provided JSON file" << indexFilename 
                << "is invalid JSON."   << endl
                << "Keeping old configuration" << endl;
    return;
  }

  // Is this a new configuration or simply an update to the current one?
  // If the two configurations are the same then there may be new images
  if (appConfig == newConfig)
  {
    // If the resources also haven't changed then nothing has changed, so
    // lets go check those ( all externalised things are resources ).
    Resources::ResourcesMap newResources;
    newResources = Resources::loadResources(newConfig["resources"]);

    if (newResources != Resources::resourcesMap)
    {
      // Resources have changed, so save the new resource node and go
      // get the new sprites
      Resources::resourcesMap = newResources;

      Graphics::removeSprites();
      Json::Value sprites = newConfig["sprites"];
      Graphics::loadSprites(sprites);

      Objects::updateObjects();
    }

    // Maybe one day a more intelligent solution can be employed as 
    // a lot of file downloading/copying is taking place. This is currently
    // because if you were to check the time of a file served from a VM it
    // would be incorrect due to the pausing/resuming of the VM.
  }
  else
  {
    // It's a new configuration, so remove the old and load the new

    /////////////
    // LOADING //
    /////////////

    //Load the resources (automatic removal upon assignment)
    Json::Value resources = newConfig["resources"];
    Resources::resourcesMap = Resources::loadResources(resources);


    //Find the sprites and load them
    Graphics::removeSprites();
    Json::Value sprites = newConfig["sprites"];
    Graphics::loadSprites(sprites);


    //Load the new objects
    Objects::removeObjects();
    Json::Value objects = newConfig["objects"];
    Objects::loadObjects(objects);

    // General settings/sanity checks
    if ( newConfig["settings"]["refresh"] . isNull() )
    {
      updatePeriod = 0;
    }
    else
    {
      if ( newConfig["settings"]["refresh"].isNumeric() )
        updatePeriod = newConfig["settings"]["refresh"].asDouble(); //Global
      else
      {
        Errors::err << "Nonsense refresh time" << endl 
                    << "Choosing to not refresh" << endl;

        updatePeriod = 0;
      }
    }

    // Accept the new configuration
    appConfig = newConfig;
  }
}

////////////////////////////////////////////////////////////////////////////
//                        WINDOW FUNCTIONS                                //
////////////////////////////////////////////////////////////////////////////

void app_graphics_render(int xs, int ys, double timestamp)
{
  //Boinc Shared Memory

  if ( Share::data == NULL )
    Share::data = (Share::SharedData*) boinc_graphics_get_shmem( "cernvm" );
  else 
    Share::data -> countdown = 5;



  //CURL Downloading 
  Networking::fileDownloader -> process();

  //Update every "updatePeriod" seconds (this also does the initial download
  if (timestamp - timeOfUpdate > updatePeriod)
  {
    // Only update if the updatePeriod is > 0 or it's the first time
    if ( updatePeriod > 0 or ( updatePeriod == 0 and appConfig.isNull() ) )
    {
      using Networking::fileDownloader;
      
      if ( ! fileDownloader->isInQueue("/index.json") )
      {
        using Networking::FileInformation;
        FileInformation indexInfo;
        indexInfo . finishResponse = &updateConfiguration;
        fileDownloader->addFile("/index.json", indexInfo);
      }

      timeOfUpdate = timestamp;
    }
  }

    
  /////////////////////////
  //    Rendering code   //
  /////////////////////////

  if ( Objects::activeView == NULL )
  {
    Errors::err << "The active view is NULL" << endl
                << "Not rendering" << endl;
    return;
  }

  Graphics::begin2D();

  //Black background
  glClearColor(1.0, 1.0, 1.0, 1.f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  //Object handling
  for (size_t i = 0; i < Objects::activeView -> size(); i++)
    Objects::activeView -> at(i) -> render();

  Graphics::end2D();
}

void app_graphics_resize(int width, int height)
{
  glViewport(0, 0, (GLsizei)width, (GLsizei)height);
}

void app_graphics_init()
{

  //Initialises the resources for the application.

  char fontFolder[] = ".";
  txf_load_fonts(fontFolder);

  //Set up the downloader (index downloading done in main loop)
  using Networking::fileDownloader;
  using Networking::FileDownloader;
  string defaultServer = "http://localhost:7859";
  fileDownloader = new FileDownloader(defaultServer);

  //Create the display object that says "not connected to VM"
  //FIXME - externalise these sort of things, so they can be multi-languaged
  Json::Value stringData;
  stringData["type"] = "strings";
  stringData["dimensions"]["x"] = -0.144;
  stringData["dimensions"]["y"] = 0.0;
  stringData["strings"]["Waiting for VM to appear"] = "";

  Objects::viewList.push_back( Objects::View() );
  Objects::activeView = & Objects::viewList[0];
  Objects::activeView -> push_back(new Objects::StringDisplay(stringData));

  Json::Value errorDispData;
  errorDispData["type"] = "errorDisplay";
  errorDispData["dimensions"]["x"] = -0.49;
  errorDispData["dimensions"]["y"] =  0.45;
  Objects::errorView.push_back(new Objects::ErrorDisplay( errorDispData ));

  Json::Value debugDispData;
  debugDispData["type"] = "debugDisplay";
  debugDispData["dimensions"]["x"] = -0.49;
  debugDispData["dimensions"]["y"] =  0.45;
  Objects::debugView.push_back(new Objects::DebugDisplay( debugDispData ));
}


////////////////////////////////////////////////////////////////////////////
//                              USER INPUT                                //
////////////////////////////////////////////////////////////////////////////
void boinc_app_mouse_move(int x, int y, int left, int middle, int right){}

void boinc_app_mouse_button(int x, int y, int which, int is_down){}

void boinc_app_key_press(int key, int)
{
  //BOINC function to handle keyboard input.
  //NOTE it is stated that the arguments are "platform specific", so I have
  //no idea if this will work on anything other than Ubuntu
  if (key==27)
  {
    boinc_close_window_and_quit("Escape Pressed");
  }
  else if ( key == 100 )
    Objects::activeView = &Objects::debugView;
  else if ( key == 101 )
    Objects::activeView = &Objects::errorView;
  else if ( key >= 49 and key <= 58 )
  {
    // viewNumber from 0 - 9, with 1 -> 0 and 0 -> 9 (key -> viewNumber)
    size_t viewNumber = key - 49;

    if ( viewNumber < Objects::viewList.size() )
      Objects::activeView = & Objects::viewList . at ( viewNumber );

  }

  using Objects::keyHandlers;
  using Objects::KeyHandlerList;
  if (keyHandlers.find( key ) != keyHandlers.end() )
  {
    for (KeyHandlerList::iterator handler = keyHandlers[ key ].begin();
         handler != keyHandlers[key] . end(); handler ++)
    {
       handler -> run();
    }
  }
}

void boinc_app_key_release(int, int){}

////////////////////////////////////////////////////////////////////////////
//                             MAIN FUNCTION                              //
////////////////////////////////////////////////////////////////////////////

int main( int argc, char** argv)
{
  if (argc > 1)
  {
    //Horrible argument parsing
    string potentialConfigFile = argv[1];
    if (potentialConfigFile.substr(0,9) == "--config=")
      forcedConfigFile = potentialConfigFile.substr(9);
  }

  boinc_init_graphics_diagnostics(BOINC_DIAG_DEFAULTS);

  boinc_parse_init_data_file();

  boinc_graphics_loop(argc, argv, WINDOW_TITLE);

  boinc_finish_diag();

  return 0;
}

