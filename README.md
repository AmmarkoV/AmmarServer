![AmmarServer](https://raw.githubusercontent.com/AmmarkoV/AmmarServer/master/doc/ammarserverbanner.png)

# AmmarServer
## A lightweight extendable barebones HTTP server for linux

Please see the wiki for more info on whats going on in this repository : )
https://github.com/AmmarkoV/AmmarServer/wiki

This repository is also tracked via OpenHub
https://www.openhub.net/p/AmmarServer

One of the most basic philosophies behind this is to try to add as much functionality possible in a reusable way *WITHOUT* overly increasing loc and dependencies.. The biggest recent improvements have been actually trying to merge common functionality and reducing loc. 


## Building
------------------------------------------------------------------ 

The projects build dependencies are the gcc compiler , pthreads , cmake and pretty basic things 
so if you issue sudo apt-get install cmake build-essential  ( assuming a Debian/Ubuntu based system ) you should be able to compile it without problems..

Compression is supported , so you might want to also apt-get install liblzma-dev if you [ENABLE_COMPRESSION at server_configuration.h](https://github.com/AmmarkoV/AmmarServer/blob/master/src/AmmServerlib/server_configuration.h#L163)
MyURL Service needs libjpeg in order to [serve captchas](https://github.com/AmmarkoV/AmmarServer/tree/master/src/AmmCaptcha) , so to add it sudo apt-get install libjpeg-dev

To perform a compilation you just need to issue 
"mkdir build && cd build &&  cmake .. && make" from the root directory

This should try to compile all of the project files which you can then run using the scripts listed in the list below 

To update your version of the project you can use the provided script that updates directly from github
It will remove any changes you have made to any of the files in the repository   
./update_from_git from the root directory 

## Running
------------------------------------------------------------------

After building the server you can use one of the provided scripts in the root directory

-  ./run_ammarserver will start a basic file server ( serving public_html/ files ) on [localhost:8080](http://localhost:8080)
-  ./run_myblog will start a small Wordpress like blog service on [localhost:8086](http://localhost:8086)
-  ./run_myloader will start a file upload service on [localhost:8085](http://localhost:8085)
-  ./run_mytube will start a youtube like service on [localhost:8084](http://localhost:8084) , you will need to change the [VIDEO_FILES_PATHS](https://github.com/AmmarkoV/AmmarServer/blob/master/src/Services/MyTube/main.c#L40) paths and recompile 
-  ./run_myurl will start a url shortner service (like tinyurl/goo.gl etc ) on [localhost:8082](http://localhost:8082)
-  ./run_habchan will start a small 4chan clone on [localhost:8083](http://localhost:8083)
-  ./run_geoposshare will start a GPS location server on [localhost:8081](http://localhost:8081) to track mobile phones ( see this [Android Application](https://github.com/AmmarkoV/GPSTransmitter)  ) 
-  ./run_myremotedesktop will start a remote desktop session viewable by a browser on [localhost:8080](http://localhost:8080) ( see [video](https://www.youtube.com/watch?v=aqH25ocm-Tc) )
-  ./run_mysearch will start a server that serves a google like front page ( that does no searching though and is only cosmetic ;P ) on [localhost:8080](http://localhost:8080)

-  ./startAmmarServerSuite.sh will start most of these simultaneously ( except remote desktop ) so you can try them out..
-  ./stopAmmarServerSuite.sh will stop them


## Installing
------------------------------------------------------------------

Install scripts are provided ( ./install.sh and ./uninstall.sh ) but at the moment they only provide the "vanilla" file static server. Until sufficient testing has been done it is not advisable to use this in a production enviornment.. 


## Features
------------------------------------------------------------------

This Repository contains :

-  The AmmarServer library with which you can build your own highly optimized 
and fully customizable web service..

-  A template for starting to build your own webserver which is very well documented
   https://github.com/AmmarkoV/AmmarServer/blob/master/src/Services/SimpleTemplate/main.c
   You can run it by issuing ./run_simpleTemplate from root directory 

-  A simple (but extensible) webserver demo showcasing some dynamic pages
   https://github.com/AmmarkoV/AmmarServer/blob/master/src/Services/main.c
   You can run it by issuing ./run_ammarserver from root directory 

-  A URL Shortner service like tinyurl , bitly , goo.gl etc..
   https://github.com/AmmarkoV/AmmarServer/tree/master/src/Services/MyURL/
   You can run it by issuing ./run_myurl from root directory 

-  A File Uploader service based on MyLoader
   https://github.com/AmmarkoV/AmmarServer/tree/master/src/Services/MyLoader/
   You can run it by issuing ./run_myloader from root directory 

-  A Web Service that allows location sharing ..
   https://github.com/AmmarkoV/AmmarServer/tree/master/src/Services/GeoPosShare/
   You can run it by issuing ./run_geoposshare from root directory 

-  A Remote Desktop Sharing Web Service that allows controlling your linux desktop through a web browser  ..
   https://github.com/AmmarkoV/AmmarServer/tree/master/src/Services/MyRemoteDesktop/
   You can run it by issuing ./run_myremotedesktop from root directory 

-  A Youtube like clone ..
   https://github.com/AmmarkoV/AmmarServer/tree/master/src/Services/MyTube/
   You can run it by issuing ./run_mytube from root directory 

-  A Blog engine that can use Wordpress skins..
   https://github.com/AmmarkoV/AmmarServer/tree/master/src/Services/MyBlog/
   You can run it by issuing ./run_myblog from root directory 

-  A 4chan like  engine for hosting a small image board , currently under construction..
   https://github.com/AmmarkoV/AmmarServer/tree/master/src/Services/HabChan/
   You can run it by issuing ./run_habchan from root directory 

------------------------------------------------------------------


AmmarServer is also an integral part , providing network connectivity and used in the following projects :
-  [V4L2ToHTTP](https://github.com/AmmarkoV/V4L2ToHTTP/)
-  [RoboVision](https://github.com/AmmarkoV/RoboVision/)
-  [FlashySlideshows](https://github.com/AmmarkoV/FlashySlideshows/)
-  [RGBDAcquisition](https://github.com/AmmarkoV/RGBDAcquisition)

This project has also been successfully deployed as a means to control embedded platforms like Robots
-  For [Hobbit EU Project](https://www.youtube.com/watch?v=41_8ktacxt8) , http://hobbit.acin.tuwien.ac.at/
-  For [GuarddoG Robot](https://www.youtube.com/watch?v=61GGKFbzG7I) , https://github.com/AmmarkoV/RoboVision/blob/master/Documentation/GuarDDoG_RoboVision.pdf
-  For [Softbank NAO Robot Realtime Bridge](https://www.youtube.com/watch?v=axfKwyVTRuA)

