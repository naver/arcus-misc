/*
 * Copyright 2012-2014 NAVER Corp.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>

int main()
{
   DIR            *dir_info;
   struct dirent  *dir_entry;
   int             fname_len;

   //mkdir( "test_A"     , 0755); 
   //mkdir( "test_B"     , 0755);

   dir_info = opendir( "./data");
   if (NULL != dir_info)
   {
      while( dir_entry   = readdir( dir_info)) 
      {
         if (dir_entry->d_type == DT_REG) {  
             fname_len = strlen(dir_entry->d_name);
             if (strncmp(&dir_entry->d_name[0], "squall_data:", 12) == 0 &&
                 strncmp(&dir_entry->d_name[fname_len-5], "00001", 5) == 0)
         printf( "d_ino=%lu, d_off=%d, d_reclen=%d, d_type=%d, d_name=%s(%d)\n",
                 dir_entry->d_ino, dir_entry->d_off, dir_entry->d_reclen, dir_entry->d_type,
                 dir_entry->d_name, fname_len);
         }
      }
      closedir( dir_info);
   }
}

