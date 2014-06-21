#include <cstdio> //used for printf
#include <ctype.h>
#include <cstring> //used for strtok strcmp
#include <map>
#include <utility> //used for make_pair
#include <cstdlib> //used atoi
#include <zlib.h>
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

size_t LENS = 1000000; //buffer length
//comparison operator used when comparing char*
struct cmp_char {
  bool operator()(const char *first,const char* second) const {
    int tmp = std::strcmp(first, second);
    return tmp<0;
  }
};


typedef std::map <char*,int,cmp_char> aMap;


void printMap(const aMap &m,FILE *fp){
  for(aMap::const_iterator it = m.begin(); it != m.end(); ++it){
    char *l = it->first;
    int val = it->second;
    fprintf(fp,"%s\t%d\n",l,val) ;
  }
}


void ttolower(char *str){
  while(*str){
    *str=tolower(*str);
    str++;
  }
}



aMap build_map(const char *filename,const char *delims,int casecmp){
  gzFile gz = tgzopen(filename,"r");
  char *buf = new char[LENS];

  
  aMap ret;
  while(gzgets(gz,buf,LENS)){
    char *key = strtok(buf,delims);
    while(key!=NULL){
      if(casecmp)
	ttolower(key);
      ret.insert(std::make_pair(strdup(key),1));
      key =strtok(NULL,delims);
    }
  }
  //  fprintf(stderr,"num items to grep for: %lu\n",ret.size());
  delete [] buf;
  gzclose(gz);
  return ret;
  
}

char* whiler(char *buf,int LENS,FILE *fp,gzFile gz){
  if(gz==Z_NULL)
    return fgets(buf,LENS,fp);
  else
    return gzgets(gz,buf,LENS);
      
  
}

char escape(const char* str){
  //  fprintf(stderr,"\'%s\'strlen:%zu\n",str,strlen(str));
  if(0==strcmp(str,"\\t"))
    return '\t';
  else if(strlen(str)==1)
    return str[0];
  
  fprintf(stderr,"Only single character delimiters allowed:%s\n",str);
  exit(0);
  return '\0';//never here
}

int main(int argc, char *argv[]){
  
  if(argc==1){
    fprintf(stderr,"usage: grepper [OPTION] -k keyfile datafile.gz\n");
    fprintf(stderr,"usage: gunzip -c datafile.gz | grepper [OPTION] -k keyfile\n");
    fprintf(stderr,"options:\n\t-c [int]: which column to use for grepping (1 indexed)\n");
    fprintf(stderr,"\t-d [char] delimitor for the datafile\n");
    fprintf(stderr,"\t-w search for whole words (similar to grep -w option)\n");
    fprintf(stderr,"\t-v complement grep (similar to grep -v option)\n");
    fprintf(stderr,"\t-i ignore case (similar to grep -i option)\n");
    return 0;
  }

  unsigned c=2;
  char d = '\t';
 
  gzFile gz= Z_NULL;
  int v =0;
  aMap asso;//used for keys
  int w=0;
  int i = 0;
  char *mapfile = NULL;
  while(*(++argv)){
    if((*argv)[0]=='-'){
      !strcasecmp(*argv,"-c")?c=atoi(*++argv):0; 
      !strcasecmp(*argv,"-d")?d=escape(*++argv):0; 
      !strcasecmp(*argv,"-k")?mapfile=*++argv:0;
      !strcasecmp(*argv,"-w")?w=1:0; 
      !strcasecmp(*argv,"-v")?v=1:0; 
      !strcasecmp(*argv,"-i")?i=1:0; 
    }else{
      gz = tgzopen(*argv,"r");
    }
  }
  if(!mapfile){
    fprintf(stderr,"supply a file with keys to grep for \'./grepper -k filename file\n");
  }
  fprintf(stderr,"-c %d -d \'%c\' -w %d -v %d -i %d gz:%p\n",c,d,w,v,i,gz);
  asso = build_map(mapfile,"\n\r\t ",i);
  fprintf(stderr,"\t-> Number of keys: in keys file: %zu\n",asso.size());
  if(asso.size()==0)
    return 0;
  //printMap(asso,stderr);

  char *buffer = new char[LENS];
  char *original = new char[LENS];

  int inter=0;
  while(whiler(buffer,LENS,stdin,gz)){
    if(1&&!(inter++ %1000))
      fprintf(stderr,"\r[%d] buf:%s",inter,buffer);
    strcpy(original,buffer);
    char *p1,*p2;
    buffer[strlen(buffer)-1] = '\0';
    unsigned at=0;
    p1=p2=buffer;
    char *toks = p1;
    //tokenize by allowing empty tokens
    while((p2=strchr(p1,d))){
      *p2='\0';
      if(++at==c)
	break;
      toks = p2+1;
      p1 = p2+1;
    }
    if(i)
      ttolower(toks);
    if(w){
      aMap::iterator it = asso.find(toks);
      int hit = (it!=asso.end());
      if((hit==1&&v==0)||(hit==0&&v==1))
	fprintf(stdout,"%s",original);
    }else{
      int hit =0;
      for(aMap::iterator it=asso.begin();it!=asso.end();++it){
	//	fprintf(stderr,"it:%s\n",it->first);
	if(strstr(toks,it->first)) {
	  //	  fprintf(stderr,"substr\n");
	  if(v==0){
	    fprintf(stdout,"%s",original);
	    break;
	  }
	}else
	  hit++;

	  
      }
      //   fprintf(stderr,"hit:%d v:%d\n",hit,v);
      if(v==1&&hit==(int)asso.size())
	fprintf(stdout,"%s",original); 
	

    }
    
  }
  
  for(aMap::iterator it=asso.begin();it!=asso.end();++it)
    free(it->first);
  delete [] buffer;
  delete [] original;


  if(gz!=Z_NULL)
    gzclose(gz);
  return 0;
}
