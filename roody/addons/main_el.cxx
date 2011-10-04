//
// main.cxx
//
// $Id$
//

#include <cstdlib>
#include <iostream>
#include <string>
#include <getopt.h>
using std::cout;
using std::endl;
using std::cerr;

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>

#ifdef OS_UNIX
#include <unistd.h>
#endif

#ifdef OS_WINNT
#include <process.h>
#endif 

#include "Roody.h"
#include "TApplication.h"
#include "rootsys.h"

//******** Command line options *************
static char* pname = NULL;
static const char* optStr = "M:R:p:c:h?";
static const struct option opts[] = {
  {"mhost", required_argument, NULL, 'M'},
  {"rhost", required_argument, NULL, 'R'},
  {"port",  required_argument, NULL, 'p'},
  {"config",required_argument, NULL, 'c'},
};

void usage()
{
  cout << "Usage:\n  " << pname 
       << " [options] [file1.root] [file2.root] .. [filen.hbook]" << endl
       << endl
       << "Options:" << endl
       << "  --mhost" << endl
       << "  -M <mhost>    -- Connect to MIDAS server <mhost>" << endl
       << "  --rhost" << endl
       << "  -R <rhost>    -- Connect to ROOT server <rhost>" << endl
       << "  --port" << endl
       << "  -p <port>     -- Connect to port <port>" << endl
       << "  --config" << endl
       << "  -c <xmlfile>  -- Using config file in XML format" << endl << endl
       << "  [files.root]  -- Load .root files" << endl
       << "  [files.hbook] -- Load .hbook files" << endl << endl;
}

//******** Main procedure *******************
int main(int argc, char *argv[])
{
  setbuf(stdout, 0);
  setbuf(stderr, 0);
  pname = argv[0];

#ifndef OS_WINNT
  signal(SIGBUS,  SIG_DFL);
  signal(SIGSEGV, SIG_DFL);
  signal(SIGPIPE, SIG_IGN);

  char const *env = getenv("ROOTSYS");  // enforce compiled-in ROOTSYS settings
  if (!env || strcmp(env,rootsys))  {
      struct stat st;
      int status = stat(rootsys,&st);
      if (status != 0)	{
	cerr << "main.cxx: Cannot use compiled-in ROOTSYS=\'"
	     << rootsys << "\': stat() error "
	     << errno << " (" << strerror(errno) << ")\n";
      } else if (st.st_mode&S_IFDIR == 0) {
	cerr << "main.cxx: Cannot use compiled-in ROOTSYS=\'"
	     << rootsys << "\', st_mode is 0"
	     << st.st_mode << ", probably not a directory\n";
      } else {
	cerr << "main.cxx: Re-executing \'" << argv[0]
	     << "\' with compiled-in ROOTSYS=\'" << rootsys << "\'\n";
	setenv( "ROOTSYS", rootsys, 1);
	execvp( argv[0], argv ); // does not return
	cerr << "execv(" << argv[0] << ") error " << errno
	     << " (" << strerror(errno) << ")\n";
	exit(1);
      }

      printf("main.cxx: Continuing with ROOTSYS=\'%s\' from local environment\n", env);
  }
#endif

  char** filenames = NULL;
  char*  hostname  = NULL;
  char*  nhostname = NULL;
  char*  xmlname   = NULL;
  int port = 0;
  int fileCount = 0;
  int longIdx = 0;

  int opt;
  std::string temp;
  do {
    opt = getopt_long(argc, argv, optStr, opts, &longIdx);
    switch(opt) {
    case 'M': hostname = optarg; break;
    case 'R': nhostname = optarg; break;
    case 'p': port = atoi(optarg); break;
    case 'c':
      temp = optarg;
      if(temp.find(".xml") == temp.npos) temp += ".xml";
      strcpy(xmlname, temp.c_str());
      break;
    case 'h':
    case '?':
    case 0:  usage();  return 0;
    break;
    default: break;
    }
  } while( opt != -1 );
  fileCount = argc - optind;
  if(fileCount > 0)
    filenames = argv + optind;

  TApplication* app = new TApplication( "App", 0, NULL );
  Roody* roody = new Roody();

  if (xmlname)
    roody->RestoreFile(xmlname);
  else
    roody->RestoreFile(NULL);

  if (hostname)
    roody->ConnectServer(hostname);

  if (nhostname)
    roody->ConnectNetDirectory(nhostname);

  for (int i=0; i<fileCount; i++)
    roody->OpenFile(filenames[i]);

  app->Run(kTRUE);
  return 0;
}

// end of file
