#ifndef VERSION_H
#define VERSION_H

	//Date Version Types
	static const char DATE[] = "18";
	static const char MONTH[] = "01";
	static const char YEAR[] = "2019";
	static const char UBUNTU_VERSION_STYLE[] =  "19.01";
	
	//Software Status
	static const char STATUS[] =  "Alpha";
	static const char STATUS_SHORT[] =  "a";
	
	//Standard Version Type
	static const long MAJOR  = 0;
	static const long MINOR  = 36;
	static const long BUILD  = 960;
	static const long REVISION  = 4939;
	
	//Miscellaneous Version Types
	static const long BUILDS_COUNT  = 1166;
	#define RC_FILEVERSION 0,36,960,4939
	#define RC_FILEVERSION_STRING "0, 36, 960, 4939\0"
	static const char FULLVERSION_STRING [] = "0.36.960.4939";
	
	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY  = 25;
	

#endif //VERSION_H
