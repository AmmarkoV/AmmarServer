#ifndef VERSION_H
#define VERSION_H

	//Date Version Types
	static const char DATE[] = "03";
	static const char MONTH[] = "09";
	static const char YEAR[] = "2015";
	static const char UBUNTU_VERSION_STYLE[] =  "15.09";
	
	//Software Status
	static const char STATUS[] =  "Alpha";
	static const char STATUS_SHORT[] =  "a";
	
	//Standard Version Type
	static const long MAJOR  = 0;
	static const long MINOR  = 29;
	static const long BUILD  = 308;
	static const long REVISION  = 1536;
	
	//Miscellaneous Version Types
	static const long BUILDS_COUNT  = 327;
	#define RC_FILEVERSION 0,29,308,1536
	#define RC_FILEVERSION_STRING "0, 29, 308, 1536\0"
	static const char FULLVERSION_STRING [] = "0.29.308.1536";
	
	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY  = 73;
	

#endif //VERSION_H
