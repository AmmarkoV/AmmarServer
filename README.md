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
-  ./run_social will start a small Social chat service on [localhost:8087](http://localhost:8087)
-  ./run_myloader will start a file upload service on [localhost:8085](http://localhost:8085)
-  ./run_mytube will start a youtube like service on [localhost:8084](http://localhost:8084) , you will need to change the [VIDEO_FILES_PATHS](https://github.com/AmmarkoV/AmmarServer/blob/master/src/Services/MyTube/main.c#L40) paths and recompile 
-  ./run_myurl will start a url shortner service (like tinyurl/goo.gl etc ) on [localhost:8082](http://localhost:8082)
-  ./run_habchan will start a small 4chan clone on [localhost:8083](http://localhost:8083)
-  ./run_geoposshare will start a GPS location server on [localhost:8081](http://localhost:8081) to track mobile phones ( see this [Android Application](https://github.com/AmmarkoV/GPSTransmitter)  ) 
-  ./run_myremotedesktop will start a remote desktop session viewable by a browser on [localhost:8080](http://localhost:8080) ( see [video](https://www.youtube.com/watch?v=aqH25ocm-Tc) )
-  ./run_mysearch will start a server that serves a google like front page ( that does no searching though and is only cosmetic ;P ) on [localhost:8080](http://localhost:8080)

-  ./startAmmarServerSuite.sh will start most of these simultaneously ( except remote desktop ) so you can try them out..
-  ./stopAmmarServerSuite.sh will stop them


## Performance
------------------------------------------------------------------

AmmarServer includes an epoll-driven fast path for serving cached static files ( simple GET/HEAD requests
with no query string, Range, ETag/compression negotiation, or auth - see [epollFastPathServer.c](https://github.com/AmmarkoV/AmmarServer/blob/master/src/AmmServerlib/threads/epollFastPathServer.c) ) that serves them
without ever spinning up a worker thread. Everything else ( dynamic pages, POST, uncached files, Range requests )
still goes through the regular thread-per-connection pipeline unchanged. Access/error logging is sharded across
`LOG_SHARD_COUNT` independent files+mutexes ( [server_configuration.h](https://github.com/AmmarkoV/AmmarServer/blob/master/src/AmmServerlib/server_configuration.h) ) rather than one global lock shared by
every request-serving thread - measured to recover most of the throughput a single shared logging mutex was
otherwise costing under concurrent load.

Benchmarked with [wrk](https://github.com/wg/wrk) ( `scripts/benchmark_vs_apache.sh` ) against a locally running
Apache 2.4 + mod_php on the same machine, keep-alive connections, byte-identical static file on both sides
( a 15.9KB PNG ):

| Concurrency | AmmarServer | Apache | AmmarServer lead |
|---|---|---|---|
| 20 connections  | 160,948 req/s | 107,355 req/s | +50% |
| 100 connections | 181,877 req/s | 117,378 req/s | +55% |
| 200 connections | 170,788 req/s | 116,086 req/s | +47% |

AmmarServer also shows noticeably lower and more consistent per-request latency throughout ( sub-millisecond at
low concurrency, under 1ms average even at c=200, vs Apache's 8-9ms average under the same load ). Take this as a
rough, single-machine snapshot rather than a rigorous lab benchmark - re-run `scripts/benchmark_vs_apache.sh` on
your own hardware and content for numbers that mean something for your deployment ( it auto-detects your Apache
docroot, stages a temporary copy of the comparison files there, and removes them again afterward - see the
script's own header comment for options ).

### Compute-bound dynamic content : C callback vs PHP

Static-file serving mostly measures I/O and connection-handling overhead. To see what AmmarServer's model - a
compiled C callback linked directly into the server, vs a script re-interpreted by PHP on every request - is
worth when a page actually has to *do* something, both sides run the same non-trivial, deterministic task : count
the primes below N=100,000 with a Sieve of Eratosthenes and sum them.

-  C : [`prepare_primes_content_callback`](https://github.com/AmmarkoV/AmmarServer/blob/master/src/Services/AmmarServer/main.c) , registered as `/primes.html` , `DIFFERENT_PAGE_FOR_EACH_CLIENT` ( a fresh computation on every request, same as the PHP side )
-  PHP : [`scripts/benchmark_primes.php`](https://github.com/AmmarkoV/AmmarServer/blob/master/scripts/benchmark_primes.php) , the same algorithm, deployed under Apache + mod_php ( `scripts/benchmark_vs_apache.sh` stages/removes it automatically )

Both sides print the prime count and sum they computed ( `primes_below_n=9592 sum_of_primes=454396537` ) so the
two implementations can be diffed for correctness - they match exactly.

| Concurrency | AmmarServer (C) | Apache + mod_php | AmmarServer lead |
|---|---|---|---|
| 20 connections  | 38,176 req/s | 2,273 req/s | ~17x |
| 100 connections | 39,384 req/s | 1,965 req/s | ~20x |
| 200 connections | 41,322 req/s | 1,913 req/s ( timeouts start appearing ) | ~22x |

Apache's prefork MPM hands each PHP request to its own process and re-interprets the script from scratch every
time ; throughput plateaus almost immediately regardless of concurrency, and latency balloons ( 8.8ms average at
c=20 up to 129ms average / 1.99s worst-case at c=200, with socket timeouts starting to appear ) as requests queue
up waiting for a free worker process. AmmarServer's compiled callback keeps scaling with concurrency and stays in
the low-single-digit-millisecond range throughout ( 0.5ms average at c=20, still only 4.9ms average at c=200 ).
Measured standalone with the CLI ( no web server in the loop ), the same algorithm already runs about 9x faster in
C than in PHP ( 0.4ms vs 3.7ms ) - the rest of the gap comes from PHP's per-request interpreter/process overhead on
top of that.

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

