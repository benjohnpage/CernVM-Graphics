#ifndef BOINC_SHARE_H
#define BOINC_SHARE_H
namespace Share
{
  struct SharedData
  {
    float credit;
    string username;
    string sharedFolder;
  };
  extern SharedData* data;
};

#endif //include guard
