/* (c) 2018 CYVA. For details refer to LICENSE */

#include <graphene/utilities/dirhelper.hpp>
#if defined( _MSC_VER )
#include <ShlObj.h>
#else
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#endif
#include <cstdlib>
#include <boost/filesystem.hpp>

using namespace graphene;
using namespace utilities;


cyva_path_finder::cyva_path_finder() {
#if defined( _MSC_VER )
   PWSTR path = NULL;
   HRESULT hr = SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, NULL, &path);
   char home_dir[MAX_PATH];
   memset(home_dir, 0, sizeof(home_dir));
   if (SUCCEEDED(hr)) {
      wcstombs(home_dir, path, MAX_PATH - 1);
      CoTaskMemFree(path);
   }
#else
   struct passwd *pw = getpwuid(getuid());
   const char *home_dir = pw->pw_dir;
#endif
   _user_home = home_dir;
   
   const char* cyva_home = getenv("CYVA_HOME");
   if (cyva_home == NULL) {
      _cyva_home = _user_home / ".cyva";
   } else {
      _cyva_home = cyva_home;
   }
   
   const char* cyva_logs = getenv("CYVA_LOGS");
   if (cyva_logs == NULL) {
      _cyva_logs = _cyva_home / "logs";
   } else {
      _cyva_logs = _cyva_logs;
   }
   
   const char* cyva_temp = getenv("CYVA_TEMP");
   if (cyva_temp == NULL) {
      _cyva_temp = _cyva_home / "temp";
   } else {
      _cyva_temp = cyva_temp;
   }
   
   const char* cyva_data = getenv("CYVA_DATA");
   if (cyva_data == NULL) {
      _cyva_data = _cyva_home / "data";
   } else {
      _cyva_data = cyva_data;
   }
   
   create_directories(_cyva_home);
   create_directories(_cyva_data);
   create_directories(_cyva_logs);
   create_directories(_cyva_temp);
}
