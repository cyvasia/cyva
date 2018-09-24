/* (c) 2018 CYVA. For details refer to LICENSE */

#pragma once

#include <fc/filesystem.hpp>

#include <cstdlib>


namespace graphene { namespace utilities {
   
   
   class cyva_path_finder {
   private:
      // Constructor may throw exceptions. Which is bad in general, but if it fails program execution must be terminated
      cyva_path_finder();
      cyva_path_finder(const cyva_path_finder&) {}
      
   public:
      static cyva_path_finder& instance() {
         static cyva_path_finder theChoosenOne;
         return theChoosenOne;
      }
      
   public:
      fc::path get_user_home()   const { return _user_home; }
      fc::path get_cyva_home() const { return _cyva_home; }
      fc::path get_cyva_data() const { return _cyva_data; }
      fc::path get_cyva_logs() const { return _cyva_logs; }
      fc::path get_cyva_temp() const { return _cyva_temp; }
      
   private:
      fc::path _user_home;
      fc::path _cyva_home;
      fc::path _cyva_data;
      fc::path _cyva_logs;
      fc::path _cyva_temp;
   };
   
   
} } // graphene::utilities
