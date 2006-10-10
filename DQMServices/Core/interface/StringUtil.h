#ifndef StringUtil_h
#define StringUtil_h

#include <string>
#include <vector>
#include <sys/types.h>
#include <regex.h>


// Class with string utilities; Use to convert strings from a unix-like format
// with wildcards (eg. C1/C2/*, C1/C2/h?, etc) to a format that the internal DQM
// logic understands: <directory pathname>:<h1>,<h2>,...
class StringUtil
{
 public:
  StringUtil(void){}
  ~StringUtil(void){}

 private:
  // if a search patten contains a wildcard (*, ?), the regex library 
  // needs a preceding "." if it is to behave like a unix "?" or "*" wildcard;
  // this function replaces "*" by ".*" and "?" by ".?"
  static std::string addDots2WildCards(const std::string & s);
  // find all "<subs>" in string <s>, replace by "<repl>"
  static std::string replace_string(const std::string& s, 
				    const std::string& subs, 
				    const std::string & repl);

  /* get parent directory. 
     Examples: 
     (a) A/B/C --> A/B
     (b) A/B/  --> A/B (because last slash implies subdirectories of A/B)
     (c) C     --> ROOT_PATHNAME  (".": top folder)
  */
  static std::string getParentDirectory(const std::string & pathname);


  // called when encountering errors in monitoring object unpacking
  static void errorObjUnp(const std::vector<std::string> & desc);

 protected:

  // keep maximum pathname till first wildcard (?, *); put rest in chopped_part
  // Examples: 
  // "a/b/c/*"    -->  "a/b/c/", chopped_part = "*"
  // "a/b/c*"     -->  "a/b/c" , chopped_part = "*"
  // "a/b/c/d?/*" -->  "a/b/c/d", chopped_part = "?/*"
  static std::string getMaxPathname(const std::string & search_string, 
			     std::string & chopped_part);

  // print out error message
  static void nullError(const char * name);
  
  // yes if we have a match (<pattern> can include unix-like wildcards "*", "?")
  static bool matchit(const std::string & s, const std::string & pattern);
  // return <pathname>/<filename>
  // (or just <filename> if <pathname> corresponds to top folder)
  static std::string getUnixName(const std::string & pathname, 
				 const std::string & filename);
  // true if pathname corresponds to top folder
  static bool isTopFolder(const std::string & pathname);

  // Using fullpath as input (e.g. A/B/test.txt) 
  // extract path (e.g. A/B) and filename (e.g. test.txt)
  static void unpack(const std::string & fullpath, 
		     std::string & path, std::string & filename);

  /* true if <subdir> is a subfolder of <parent_dir> (or same)
     eg. c0/c1/c2 is a subdirectory of c0/c1, but
     c0/c1_1 is not */
  static bool isSubdirectory(const std::string & parentdir_fullpath, 
		      const std::string & subdir_fullpath);

  // similar to isSubdirectory, with the exception that <subscription> is 
  // of the form: <directory pathname>:<h1>,<h2>,...
  static bool belongs2folder(const std::string & folder, 
		      const std::string & subscription);

  // structure for unpacking "directory" format: <dir_path>:<obj1>,<obj2>,...
  // if directory appears empty, all owned objects are implied;
  // a tag could be specified at end: <dir_path>:<obj1>,<obj2>,...:<tag>
  struct DirFormat_{
    std::string dir_path;  // full directory pathname
    std::vector<std::string> contents; // vector of monitoring objects
    unsigned int tag; // tag (optional, it may no appear ==> 0)
    DirFormat_(){tag = 0;}
  };
  typedef struct DirFormat_ DirFormat;

  // unpack directory format (name); expected format: see DirFormat definition
  // return success flag
  static bool unpackDirFormat(const std::string & name, DirFormat & dir);
  // unpack input string into vector<string> by using "separator"
  static void unpackString(const char* in, const char* separator, 
			   std::vector<std::string> & put_here);

  // unpack QReport (with name, value) into ME_name, qtest_name, status, message;
  // return success flag; Expected format of QReport is a TNamed variable with
  // (a) name in the form: <ME_name>.<QTest_name>
  // (b) title (value) in the form: st.<status>.<the message here>
  // (where <status> is defined in Core/interface/QTestStatus.h)
  static bool unpackQReport(std::string name, std::string value, 
			    std::string & ME_name, 
			    std::string & qtest_name,
			    int & status, std::string & message);

  // true if string includes any wildcards ("*" or "?")
  static bool hasWildCards(const std::string & pathname);

  // chop off last slash in pathname (if any); eg. "A/B/" ==> "A/B"
  static void chopLastSlash(std::string & pathname)
  {
    unsigned int size = pathname.size();
    if(size == 0)return;
    
    if(pathname[size-1] == '/')
      pathname.erase(size-1);
  }

  // get range as defined by search_string; sometimes parent directory must also
  // be included (otherwise set it to one-beyond-end)
  // Examples:
  //  (a) "A/B/C*" --> all pathnames starting with A/B/C and parent directory A/B 
  //  (not clear if C is part of directory name or file inside A/B directory)
  //  (b) "A/B/*" --> all pathnames starting with A/B/ and parent directory A/B
  //  (c) "C*"    --> all pathnames starting with C and top directory
  //  (d) "*"     --> all pathnames (top directory already included)
  template <class T>
    void getSubRange(const std::string & search_string, const T & t, 
		     typename T::const_iterator & start, 
		     typename T::const_iterator & end, 
		     typename T::const_iterator & parent_dir) const
  {
    start = end = parent_dir = t.end();
    if(search_string.empty())return;
    
    std::string chopped_part;
    std::string path = getMaxPathname(search_string, chopped_part);
    
    // initialize start as if full range should be included
    start = t.begin();

    unsigned length = path.size();
    // non-empty string
    if(length)
      {
	start = t.lower_bound(path);

	// if search-string contains wildcards
	// will set end to one-position-beyond the expected range
	if(!chopped_part.empty())
	  {
	    char last_character = path[length-1];
	    // replace last character in path by next alphanumeric character
	    std::string end_path = path.substr(0, length-1) 
	      + char(int(last_character) + 1);
	    end = t.lower_bound(end_path);
	  }
	else
	  // otherwise range is limited to single directory
	  {
	    end = start;
	    if(end != t.end())++end;
	  }
      }

    bool full_range = (start == t.begin() && end == t.end());
    bool chopped_slashes = (chopped_part.find("/") != std::string::npos);
    // will consider parent directory if 
    // (a) we don't already include the full range, and
    // (b) search_string does not imply we don't want parent directory,
    // e.g. "A/B/D?/*"
    if(!full_range && !chopped_slashes)
      {
	parent_dir = t.find(getParentDirectory(path));
      }
    
  }

  
};

#endif // define StringUtil_h
