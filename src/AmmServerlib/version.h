#ifndef VERSION_H
#define VERSION_H

	//Date Version Types
	static const char DATE[] = "23";
	static const char MONTH[] = "02";
	static const char YEAR[] = "2018";
	static const char UBUNTU_VERSION_STYLE[] =  "18.02";
	
	//Software Status
	static const char STATUS[] =  "Alpha";
	static const char STATUS_SHORT[] =  "a";
	
	//Standard Version Type
	static const long MAJOR  = 0;
	static const long MINOR  = 34;
	static const long BUILD  = 736;
	static const long REVISION  = 3744;
	
	//Miscellaneous Version Types
	static const long BUILDS_COUNT  = 894;
	#define RC_FILEVERSION 0,34,736,3744
	#define RC_FILEVERSION_STRING "0, 34, 736, 3744\0"
	static const char FULLVERSION_STRING [] = "0.34.736.3744";
	
	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY  = 1;
	

#endif //VERSION_H
