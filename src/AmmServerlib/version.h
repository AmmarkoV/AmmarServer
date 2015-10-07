#ifndef VERSION_H
#define VERSION_H

	//Date Version Types
	static const char DATE[] = "07";
	static const char MONTH[] = "10";
	static const char YEAR[] = "2015";
	static const char UBUNTU_VERSION_STYLE[] =  "15.10";
	
	//Software Status
	static const char STATUS[] =  "Alpha";
	static const char STATUS_SHORT[] =  "a";
	
	//Standard Version Type
	static const long MAJOR  = 0;
	static const long MINOR  = 31;
	static const long BUILD  = 444;
	static const long REVISION  = 2222;
	
	//Miscellaneous Version Types
	static const long BUILDS_COUNT  = 492;
	#define RC_FILEVERSION 0,31,444,2222
	#define RC_FILEVERSION_STRING "0, 31, 444, 2222\0"
	static const char FULLVERSION_STRING [] = "0.31.444.2222";
	
	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY  = 9;
	

#endif //VERSION_H
