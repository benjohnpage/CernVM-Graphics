#ifndef VMNET_H_INC
#define VMNET_H_INC

#include <cstdio>
#include <string>
#include <map>
using std::string;
using std::map;

#include <curl/curl.h>


namespace Networking
{
  typedef void (*responseFunc)( CURL* ); 

  struct FileInformation
  {
    string filePath;
    responseFunc finishResponse;
    FILE* filep;
    time_t modTime;
    FileInformation();
  };

  typedef map< CURL*, FileInformation > FileInfoMap ;

  //The class for file downloading
  //
  //process() processes the asynchronous downloads - goes in the main loop
  //addFile() is for initialising an asynchronous download.
  //getFile() makes synchronous downloads - this will block until finished
  //Once the download is finished then the argument finishFunction() 
  //will be called.
  class FileDownloader
  {
    private:
      string self_defaultServerAddress;
      int    self_numActive;
      CURLM* self_multiHandle;

      FileInfoMap self_fileInfos;
    public:
      FileDownloader( string defaultServerAddress );
      void   process       ();
      void   addFile       ( string filename, FileInformation fileInfo);
      long   getFileAge    ( string filename );
      string getFile       ( string filename );
      string pathFromString( string path );
      bool   isInQueue( string filePath );
  };

  extern FileDownloader* fileDownloader;

  time_t fileModifyTime( string localPath );
};

#endif //Include guard
