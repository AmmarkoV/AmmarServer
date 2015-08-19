#ifndef VERSION_H
#define VERSION_H

	//Date Version Types
	static const char DATE[] = "19";
	static const char MONTH[] = "08";
	static const char YEAR[] = "2015";
	static const char UBUNTU_VERSION_STYLE[] =  "15.08";
	
	//Software Status
	static const char STATUS[] =  "Alpha";
	static const char STATUS_SHORT[] =  "a";
	
	//Standard Version Type
	static const long MAJOR  = 0;
	static const long MINOR  = 29;
	static const long BUILD  = 299;
	static const long REVISION  = 1497;
	
	//Miscellaneous Version Types
	static const long BUILDS_COUNT  = 317;
	#define RC_FILEVERSION 0,29,299,1497
	#define RC_FILEVERSION_STRING "0, 29, 299, 1497\0"
	static const char FULLVERSION_STRING [] = "0.29.299.1497";
	
	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY  = 64;
	

#endif //VERSION_H
