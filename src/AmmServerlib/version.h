#ifndef VERSION_H
#define VERSION_H

	//Date Version Types
	static const char DATE[] = "28";
	static const char MONTH[] = "10";
	static const char YEAR[] = "2021";
	static const char UBUNTU_VERSION_STYLE[] =  "21.10";
	
	//Software Status
	static const char STATUS[] =  "Alpha";
	static const char STATUS_SHORT[] =  "a";
	
	//Standard Version Type
	static const long MAJOR  = 0;
	static const long MINOR  = 36;
	static const long BUILD  = 961;
	static const long REVISION  = 4949;
	
	//Miscellaneous Version Types
	static const long BUILDS_COUNT  = 1166;
	#define RC_FILEVERSION 0,36,961,4949
	#define RC_FILEVERSION_STRING "0, 36, 961, 4949\0"
	static const char FULLVERSION_STRING [] = "0.36.961.4949";
	
	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY  = 26;
	

#endif //VERSION_H
