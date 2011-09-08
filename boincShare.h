#include "boinc_api.h"

#ifndef BOINC_SHARE_H
#define BOINC_SHARE_H
namespace Share
{
  struct SharedData
  {
    double update_time;
    double fraction_done;
    double cpu_time;
    BOINC_STATUS status;
    APP_INIT_DATA init_data;
    int countdown;
        // graphics app sets this to 5 repeatedly,
        // main program decrements it once/sec.
        // If it's zero, don't bother updating shmem
  };

  extern SharedData* data;
};

#endif //include guard
