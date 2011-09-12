#ifndef VMNET_H_INC
#define VMNET_H_INC

#include <cstdio>
#include <string>
#include <map>

#include <curl/curl.h>


namespace Networking
{
  typedef void (*responseFunc)( CURL* ); 

  struct FileInformation
  {
    std::string filePath;
    responseFunc finishResponse;
    FILE* filep;
    time_t modTime;
    FileInformation();
  };

  typedef std::map< CURL*, FileInformation > FileInfoMap ;

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
      std::string self_defaultServerAddress;
      int    self_numActive;
      CURLM* self_multiHandle;

      FileInfoMap self_fileInfos;
    public:
      FileDownloader( std::string defaultServerAddress );
      void   process       ();
      void   addFile       ( std::string filename, FileInformation fileInfo);
      long   getFileAge    ( std::string filename );
      std::string getFile       ( std::string filename );
      std::string pathFromString( std::string path );
      bool   isInQueue( std::string filePath );
  };

  extern FileDownloader* fileDownloader;

  time_t fileModifyTime( std::string localPath );
};

#endif //Include guard
