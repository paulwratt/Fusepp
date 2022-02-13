// Hackaday RSS filesystem class implementation

#include "HaDFS.h"

#include <iostream>
#include <stdio.h>  // using popen
#include <string>
#include <array>

// include in one .cpp file
#include "Fuse-impl.h"

using namespace std;

static const string root_path = "/";

// Maximum number of topics
#define MAXTOPIC ((unsigned long)128)
static unsigned topics=0;  // number of topics we've seen

// Record for RSS item
struct a_topic
{
  string title;  // title
  string url;    // URL
  string content;  // content
};

// Where the topics live
array<a_topic,MAXTOPIC> topic;


// The curl line to read our feed
static char cmd[]="curl https://hackaday.com/feed/ 2>/dev/null | egrep '(<title>)|(<link>)'";

// Trim RSS lines to kill leading spaces and the last </...> part
// We are assuming the format will not have multiple things on one line
// Although that isn't a great assumption
static string trimrss(const string &str, const string &white1 = " \t\r\n", const string &white2 = "<")
{
  size_t first, last;
  first=str.find_first_not_of(white1);
  if (first==string::npos) return "";
  last=str.find_last_of(white2);
  if (last==string::npos) last=str.length()-1; else last--;  // untested if there is no <...> at the end
  return str.substr(first,last-first+1);
}

// Find a path and return a file descriptor (can't be zero so we add 1 to it)
int HaDFS::pathfind(const char *path)
{
  string s=path;
  if (s.find_first_of('/')!=string::npos)
    {
      // no subdirectories
      return -1;
    }
  for (unsigned int i=0;i<topics;i++)
    if (s==topic[i].title) return i+1;
  return -1;
}



// Read the content or any URL for that matter
static string readurl(string &url)
{
  FILE *fp;
  char buf[1024];  // working buffer for reading strings
    string result, curl="2>/dev/null curl '";
    curl+=url;
  curl+="'";
  if (!(fp=popen(curl.c_str(),"r"))) return "";
  while (fgets(buf,sizeof(buf),fp))
    {
      result+=buf;
      result+="\n";
    }
  pclose(fp);
  return result;
}

// User initialization--read the feed (note that init is reserved by the FUSE library)
int HaDFS::userinit(void)
{

  FILE *fp;
  char buf[1024];  // working buffer for reading strings
  if (!(fp=popen(cmd,"r"))) return 1;  // open pipe
  while (fgets(buf,sizeof(buf),fp))
    {
      string line=buf;
      line=trimrss(line);  // trim off extra stuff
      if (line.substr(0,7)=="<title>")    // identify line type and process
	{
	  topic[topics].title=line.substr(7);
	  topic[topics].title+=".html";

	}
      else if (line.substr(0,6)=="<link>")
	{
	  topic[topics].url=line.substr(6);
	  topics++;
	  if (topics==MAXTOPIC) break;   // quietly truncate a long feed
	}
    }
  pclose(fp);
  return 0;
}


// Fuse wants to stat our files
int HaDFS::getattr(const char *path, struct stat *stbuf, struct fuse_file_info *)
{
        int res = 0,n;
	memset(stbuf, 0, sizeof(struct stat));
	if (path == root_path)
	  {
	    stbuf->st_mode = S_IFDIR | 0755;   // our root directory
	    stbuf->st_nlink = 2;
	  }
	else if ((n=pathfind(path+1))>0)   // find path (skip leading /)
	  {
	    stbuf->st_mode = S_IFREG | 0444;  // read only
	    stbuf->st_nlink = 1;   // no links
	    if (topic[n-1].content.empty())  // if empty, go ahead and fill content cache
	      {
		topic[n-1].content=readurl(topic[n-1].url);  
	      }
	    stbuf->st_size = topic[n-1].content.length();  // now we know the length
	  }
	else
	  {
	  res = -ENOENT;  // not found
	  }
	return res;
}

// Fuse wants to read a directory
int HaDFS::readdir(const char *path, void *buf, fuse_fill_dir_t filler,
			               off_t, struct fuse_file_info *, enum fuse_readdir_flags)
{
  if (path != root_path)  // only root is valid
    return -ENOENT;
  // we always have . and ..
  filler(buf, ".", NULL, 0, FUSE_FILL_DIR_PLUS);
  filler(buf, "..", NULL, 0, FUSE_FILL_DIR_PLUS);
  // plus all of our "files"
  for (unsigned int i=0;i<topics;i++)
    filler(buf, topic[i].title.c_str(), NULL, 0, FUSE_FILL_DIR_PLUS);
  return 0;
}


// Fuse wants to open a file
int HaDFS::open(const char *path, struct fuse_file_info *fi)
{
       string fn=path+1;  // skip leading /
       int n;
       if (fn.find_first_of('/')!=string::npos) return -ENOENT;  // no sub dirs
       if ((n=pathfind(fn.c_str()))==-1)    // find fd
	 return -ENOENT;

       if ((fi->flags & 3) != O_RDONLY)  // must be read only
	 return -EACCES;
// fi->fh is file handle
       fi->fh=n;
       if (topic[n-1].content.empty())  // if we have no content in cache, try to read now
	 {
	   topic[n-1].content=readurl(topic[n-1].url);
	 }
	return 0;
}


// Fuse wants to read something
int HaDFS::read(const char *path, char *buf, size_t size, off_t offset,
		              struct fuse_file_info *fi)
{
	size_t len;
	len = topic[fi->fh-1].content.length();  // we get the length
	if ((size_t)offset < len)  // if the offset isn't past the end...
	  {
	    if (offset + size > len)
	      size = len - offset;
	    memcpy(buf, topic[fi->fh-1].content.c_str() + offset, size);  // get the data over to the buffer
	  } else
	  size = 0;  // no data here
	return size;
}











