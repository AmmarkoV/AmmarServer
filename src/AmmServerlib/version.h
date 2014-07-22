#ifndef VERSION_H
#define VERSION_H

	//Date Version Types
	static const char DATE[] = "22";
	static const char MONTH[] = "07";
	static const char YEAR[] = "2014";
	static const char UBUNTU_VERSION_STYLE[] =  "14.07";
	
	//Software Status
	static const char STATUS[] =  "Alpha";
	static const char STATUS_SHORT[] =  "a";
	
	//Standard Version Type
	static const long MAJOR  = 0;
	static const long MINOR  = 27;
	static const long BUILD  = 127;
	static const long REVISION  = 556;
	
	//Miscellaneous Version Types
	static const long BUILDS_COUNT  = 109;
	#define RC_FILEVERSION 0,27,127,556
	#define RC_FILEVERSION_STRING "0, 27, 127, 556\0"
	static const char FULLVERSION_STRING [] = "0.27.127.556";
	
	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY  = 92;
	

#endif //VERSION_H
