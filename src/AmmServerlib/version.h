#ifndef VERSION_H
#define VERSION_H

	//Date Version Types
	static const char DATE[] = "24";
	static const char MONTH[] = "02";
	static const char YEAR[] = "2015";
	static const char UBUNTU_VERSION_STYLE[] =  "15.02";
	
	//Software Status
	static const char STATUS[] =  "Alpha";
	static const char STATUS_SHORT[] =  "a";
	
	//Standard Version Type
	static const long MAJOR  = 0;
	static const long MINOR  = 29;
	static const long BUILD  = 254;
	static const long REVISION  = 1275;
	
	//Miscellaneous Version Types
	static const long BUILDS_COUNT  = 255;
	#define RC_FILEVERSION 0,29,254,1275
	#define RC_FILEVERSION_STRING "0, 29, 254, 1275\0"
	static const char FULLVERSION_STRING [] = "0.29.254.1275";
	
	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY  = 19;
	

#endif //VERSION_H
