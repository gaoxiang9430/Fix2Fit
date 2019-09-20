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

int main(int argc, char* argv[]){

  struct C_SearchEngine * engine;
  //run original f1x
  c_repair_main(argc, argv, &engine);

  if(engine != NULL){
    //get all the locations of plausible patches
    char * locs; int length;
    c_getPatchLoc(engine, &length, &locs);

    FILE * fp;
    fp = fopen("location.txt", "w");
    if (fp == NULL){
      exit(1);
    }
 
    char* end_locs;
    char* end_loc;
    char *loc = strtok_r (locs, "#", &end_locs);
    while(loc != NULL){
      char *bl = strtok_r (loc, " ", &end_loc);
      bl = strtok_r(NULL," ", &end_loc); //the second value is the line number
      fprintf(fp, "%s ", bl);
      loc = strtok_r(NULL,"#", &end_locs); //the second value is the line number
    }

    fprintf(fp, "\n");
    fclose(fp);
  }
  return 0;
}
