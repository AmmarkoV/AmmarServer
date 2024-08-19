#ifndef VERSION_H
#define VERSION_H

	//Date Version Types
	static const char DATE[] = "16";
	static const char MONTH[] = "08";
	static const char YEAR[] = "2024";
	static const char UBUNTU_VERSION_STYLE[] =  "24.08";
	
	//Software Status
	static const char STATUS[] =  "Alpha";
	static const char STATUS_SHORT[] =  "a";
	
	//Standard Version Type
	static const long MAJOR  = 0;
	static const long MINOR  = 36;
	static const long BUILD  = 962;
	static const long REVISION  = 4954;
	
	//Miscellaneous Version Types
	static const long BUILDS_COUNT  = 1167;
	#define RC_FILEVERSION 0,36,962,4954
	#define RC_FILEVERSION_STRING "0, 36, 962, 4954\0"
	static const char FULLVERSION_STRING [] = "0.36.962.4954";
	
	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY  = 27;
	

#endif //VERSION_H
