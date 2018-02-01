#ifndef VERSION_H
#define VERSION_H

	//Date Version Types
	static const char DATE[] = "02";
	static const char MONTH[] = "02";
	static const char YEAR[] = "2018";
	static const char UBUNTU_VERSION_STYLE[] =  "18.02";
	
	//Software Status
	static const char STATUS[] =  "Alpha";
	static const char STATUS_SHORT[] =  "a";
	
	//Standard Version Type
	static const long MAJOR  = 0;
	static const long MINOR  = 33;
	static const long BUILD  = 664;
	static const long REVISION  = 3352;
	
	//Miscellaneous Version Types
	static const long BUILDS_COUNT  = 822;
	#define RC_FILEVERSION 0,33,664,3352
	#define RC_FILEVERSION_STRING "0, 33, 664, 3352\0"
	static const char FULLVERSION_STRING [] = "0.33.664.3352";
	
	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY  = 29;
	

#endif //VERSION_H
