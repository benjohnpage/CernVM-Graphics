//boinc
#include "filesys.h"

//cURL
#include <curl/curl.h>

//Ours
#include "networking.h"
#include "errors.h"

// Standard
#include <string>
#include <iostream>
using std::string;
using std::endl;

Networking::FileDownloader* Networking::fileDownloader;

#ifdef __unix__
#include <sys/stat.h>
#include <sys/types.h>

time_t Networking::fileModifyTime( string localPath )
{
  struct stat fileInfo;
  stat(localPath.c_str(), &fileInfo);
  return fileInfo.st_mtime;
}
#endif

#ifdef _WIN32
#include <windows.h>
//TODO File age on windows
#endif


Networking::FileInformation::FileInformation() :
  filep(NULL), modTime(-1) {}

string Networking::FileDownloader::pathFromString(string path)
{
  // The path can be of two formats:
  // 1) Prepended with a "/", this causes the FileDownloader to use the
  // default server provided in its constructor
  // 2) Everything else - this will be interpretted as a URL

  // Type 1 - default server
  if (path.at(0) == '/')
    return self_defaultServerAddress + path;

  else
    return path;
}

string Networking::FileDownloader::getFile(string filename)
{
  //This is a synchronous call to curl to get a file, that is the call will
  //block until the file is downloaded - do NOT use it to download a lot.

  string newFilename; 

  CURL* downloadHandle = curl_easy_init();
  if (downloadHandle)
  {
    newFilename = "./dispFiles/" + filename;
    FILE* newFile = fopen(newFilename.c_str(), "w");

    string offsitePath = this->pathFromString(filename);
    curl_easy_setopt(downloadHandle, CURLOPT_URL, offsitePath.c_str());
    curl_easy_setopt(downloadHandle, CURLOPT_WRITEDATA, newFile);
    CURLcode result = curl_easy_perform(downloadHandle);

    if (result != CURLE_OK) 
    {
      Errors::err << "CURL Error in getting file \"" << offsitePath << "\" - "
                  << curl_easy_strerror(result) << endl;
    }
    else
    {
      Errors::dbg << "Successful file download of " << offsitePath << endl;
    }

    fclose(newFile);
  }

  curl_easy_cleanup(downloadHandle);

  return newFilename;
}

Networking::FileDownloader::FileDownloader(string defaultServerAddress) :
  self_numActive(0), self_multiHandle(NULL)
{
  self_multiHandle = curl_multi_init();
  self_defaultServerAddress = defaultServerAddress;
  boinc_mkdir("./dispFiles");
}

long Networking::FileDownloader::getFileAge( string filename )
{
  string onlineFilePath = this -> pathFromString( filename );

  CURL* easyHandle = curl_easy_init();
  curl_easy_setopt( easyHandle, CURLOPT_URL, onlineFilePath.c_str());
  curl_easy_setopt( easyHandle, CURLOPT_NOBODY, 1);
  curl_easy_setopt( easyHandle, CURLOPT_FILETIME, 1);

  CURLcode result =  curl_easy_perform( easyHandle );
  if (result == CURLE_OK)
  {
    long age = -1;
    curl_easy_getinfo(easyHandle, CURLINFO_FILETIME, &age);

    if (age == -1)
      Errors::err << "ERROR - the server does not support file ages." 
                  << endl;

    return age;
  }
  else
  {
    //An error occured, file probably inaccessible
    return -1;
  }
}

void Networking::FileDownloader::addFile(string filename,  
                                       Networking::FileInformation fileInfo)
{
  //XXX
  //The current implimentation stores the information against the
  //easy_handle in a map. I've deemed this a bad (but currently unavoidable)
  //choice. This is because we technically lose track of the easy_handle,
  //only being able to get back at it through the multi_handle. So the
  //only thing we can guarantee is that there will be an easy handle passed
  //back at completion, but the pointer value (which we are using as the map
  //key) may well change - hence a bad decision.
  //HOWEVER, I've hopefully written the abstraction in such a way that this
  //doesn't affect the "user", so changing this implimentation should not be
  //too much of an issue in future.

  string onlineFilePath = this->pathFromString(filename);
  fileInfo . filePath = onlineFilePath;

  //Allow the user to choose a different place to put the file if necessary
  if (fileInfo . filep == NULL)
  {  
    string localPath = "./dispFiles/" + filename;
    fileInfo . filep = fopen( localPath.c_str(), "w" );
  }

  //Make and setup the easyHandle
  CURL* easyHandle = curl_easy_init();
  curl_easy_setopt(easyHandle, CURLOPT_URL, onlineFilePath.c_str());
  curl_easy_setopt(easyHandle, CURLOPT_WRITEDATA, fileInfo . filep);

  curl_easy_setopt( easyHandle, CURLOPT_FILETIME, 1);
  if (fileInfo.modTime != -1)
  {
    curl_easy_setopt( easyHandle, CURLOPT_TIMEVALUE, fileInfo.modTime);
    curl_easy_setopt( easyHandle, CURLOPT_TIMECONDITION,
                      CURL_TIMECOND_IFMODSINCE);
  }

  //Save the information
  self_fileInfos[easyHandle] = fileInfo;
  
  //Add the easy to the multi!
  curl_multi_add_handle(self_multiHandle, easyHandle);

  self_numActive++;
}

bool Networking::FileDownloader::isInQueue( string filename )
{
  // Fileinfo structs store by online filename, so turn filename into an
  // online path.

  string onlinePath = this -> pathFromString( filename );

  for ( FileInfoMap::iterator itr  = self_fileInfos.begin();
                              itr != self_fileInfos.end();
                              itr ++ )
  {
    if (itr -> second . filePath == onlinePath)
      return true;
  }

  return false;
}

void Networking::FileDownloader::process()
{

  curl_multi_perform(self_multiHandle, &self_numActive);

  int numMessages;
  do 
  {
    CURLMsg *message = curl_multi_info_read(self_multiHandle, &numMessages);
    if (message != NULL) 
    {
      //Readability variables
      CURLcode result  = message -> data.result;
      CURL* easyHandle = message -> easy_handle;

      if (result == CURLE_OK)
      {
        Networking::FileInformation completedFile = 
                                                 self_fileInfos[easyHandle];
        //Report the success
        Errors::dbg << "Successful download of file \"" 
                    << completedFile.filePath
                    << "\"" << endl;

        //Close file and run the finish function (if it exists)
        fclose(completedFile . filep);

        if (completedFile . finishResponse != NULL)
        {
          (*(completedFile . finishResponse))( easyHandle );
        }

        //Kill the fileInfo, and clean up the easyHandles
        self_fileInfos.erase(easyHandle);
        curl_multi_remove_handle(self_multiHandle, easyHandle);
        curl_easy_cleanup(easyHandle);
      }
      else
      {
        //If we couldn't connect, retry the handle as we really should be
        //able to.
        curl_multi_remove_handle(self_multiHandle, easyHandle);
        curl_multi_add_handle   (self_multiHandle, easyHandle);
        self_numActive++;

        // If it isn't a "couldn't connect" then relay an error message
        // Couldn't connect is special as this will happen with no VM present
        if (result  != CURLE_COULDNT_CONNECT)
        {
          Networking::FileInformation completedFile = 
                                                 self_fileInfos[easyHandle];
          string errorDescription = curl_easy_strerror(result);
  
          Errors::err << "Unhandled error downloading \"" 
                      << completedFile.filePath 
                      << "\" - " << errorDescription << endl;
        }
      }
    }
  }
  while (numMessages != 0);
}

