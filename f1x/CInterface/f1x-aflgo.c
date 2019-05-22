/* this file shows an example that invoking f1x using the c interface.
 * Those c interfaces show be invoked inside aflgo
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "F1X.h"
#include "SearchEngine.h"
#include<unistd.h>
#include<sys/types.h>
#include<sys/wait.h>
#include <dirent.h>

void executeMetaProgram(struct C_SearchEngine * engine, char* path){
    DIR *dp;
    struct dirent *ep;
    dp = opendir (path);
    int i = 0;
    long num_plausiblePatch = -1;
    if (dp != NULL)
    {
      while (ep = readdir (dp)){
        i++;
        //given one test, c_fuzzPatch will return the current partition size.
        char full_path [80];
        sprintf(full_path, "%s%s", path, ep->d_name);

        //given one test, c_fuzzPatch will return the current partition size.
        num_plausiblePatch = c_fuzzPatch(engine, full_path);

        if(i%100 == 0)
            printf("the current(%d) number of plausible patches is: %ld\n", i, num_plausiblePatch);
        if(num_plausiblePatch<=0)
          break;
      }
      printf("the current number of plausible patches is: %ld\n", num_plausiblePatch);
      (void) closedir (dp);
    }
    else
      perror ("Couldn't open the directory");
}

int main(int argc, char* argv[]){

  struct C_SearchEngine * engine;
  //run original f1x
  c_repair_main(argc, argv, &engine);

  if(engine != NULL){
    //get all the locations of plausible patches
    char * locs; int length;
    c_getPatchLoc(engine, &length, &locs);
    //char location[4096]="";
    //for(int i=0; i< length; i++){
      //encode location to one string("loc1", "loc2", "loc3"), which will be used in aflgo
      //sprintf(location, "%s\"%d\",", location, locs[i]);
    //}
    printf("locations of plausible patches are : %s\n", locs);
    printf("the working directory is : %s\n", c_getWorkingDir(engine));

    //invoke aflgo to generate new test
/*    pid_t id = fork();
    if(id == 0)
    {
      char *argv[] = { "executeAFLGO", location, c_getWorkingDir(engine), NULL };
      execvp("executeAFLGO", argv);
      printf("Children Done!!!\n");
      exit(0);
    } else {
      waitpid(id, NULL, 0);
      executeMetaProgram(engine, "/out2/ef709ce2/crashes/");
      executeMetaProgram(engine, "/out2/ef709ce2/interestedTest/");
      printf("DONE\n");
    }
*/ 
  }

  return 0;
}
