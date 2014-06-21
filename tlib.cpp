#include <zlib.h>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <sys/stat.h>

int fexists(const char* str){
  struct stat buffer ;
  return (stat(str, &buffer )==0 );
}

size_t fsize(const char* fname){
  struct stat st ;
  stat(fname,&st);
  return st.st_size;
}



FILE *tfopen(const char*fname,const char*mode){
  FILE *fp=NULL;
  if(strchr(mode,'r')&&fexists(fname)&&fsize(fname)==0){
    fprintf(stderr,"Problem opening file: %s for reading, doesn't exists or is empty\n",fname);
    exit(0);
  }
  fp=fopen(fname,mode);
  if(fp==NULL){
    fprintf(stderr,"Problem opening file: %s \n",fname);
    exit(0);
  }
  return fp;
}

gzFile tgzopen(const char*fname,const char*mode){
  gzFile fp=Z_NULL;
  if(strchr(mode,'r')&&fexists(fname)&&fsize(fname)==0){
    fprintf(stderr,"Problem opening file: %s for reading, doesn't exists or is empty\n",fname);
    exit(0);
  }
  fp=gzopen(fname,mode);
  if(fp==NULL){
    fprintf(stderr,"Problem opening file: %s \n",fname);
    exit(0);
  }
  return fp;
}
