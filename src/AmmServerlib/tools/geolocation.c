#include "geolocation.h"
#include <stdio.h>
//http://www.ipdeny.com/ipblocks/
// wget http://www.ipdeny.com/ipblocks/data/countries/all-zones.tar.gz && tar xvzf all-zones.tar.gz
// wget http://www.ipdeny.com/ipv6/ipaddresses/blocks/ipv6-all-zones.tar.gz && tar xvzf ipv6-all-zones.tar.gz



char * getIPCountry(const char * ip)
{
 fprintf(stderr,"getIPCountry(%s) not implemented \n",ip);
 return 0;
}
