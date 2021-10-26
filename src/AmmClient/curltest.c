/***************************************************************************
 *                                  _   _ ____  _
 *  Project                     ___| | | |  _ \| |
 *                             / __| | | | |_) | |
 *                            | (__| |_| |  _ <| |___
 *                             \___|\___/|_| \_\_____|
 *
 * Copyright (C) 1998 - 2020, Daniel Stenberg, <daniel@haxx.se>, et al.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution. The terms
 * are also available at https://curl.se/docs/copyright.html.
 *
 * You may opt to use, copy, modify, merge, publish, distribute and/or sell
 * copies of the Software, and permit persons to whom the Software is
 * furnished to do so, under the terms of the COPYING file.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ***************************************************************************/
/* <DESC>
 * HTTP Multipart formpost with file upload and two additional parts.
 * </DESC>
 */
/* Example code that uploads a file name 'foo' to a remote script that accepts
 * "HTML form based" (as described in RFC1738) uploads using HTTP POST.
 *
 * The imaginary form we'll fill in looks like:
 *
 * <form method="post" enctype="multipart/form-data" action="examplepost.cgi">
 * Enter file: <input type="file" name="sendfile" size="40">
 * Enter file name: <input type="text" name="filename" size="30">
 * <input type="submit" value="send" name="submit">
 * </form>
 *
 */


//Basic image streamer..
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <curl/curl.h>



char * readFileToMemory(const char * filename,unsigned int *length)
{
    *length = 0;
    FILE * pFile = fopen ( filename, "rb" );

    if (pFile==0)
    {
        fprintf(stderr, "AmmServer_ReadFileToMemory failed\n");
        fprintf(stderr, "Could not read file %s \n",filename);
        return 0;
    }

    // obtain file size:
    fseek (pFile, 0, SEEK_END);
    unsigned long lSize = ftell (pFile);
    rewind (pFile);

    // allocate memory to contain the whole file:
    unsigned long bufferSize = sizeof(char)*(lSize+1);
    char * buffer = (char*) malloc (bufferSize);
    if (buffer == 0 )
    {
        fprintf(stderr,"Could not allocate enough memory for file %s \n",filename);
        fclose(pFile);
        return 0;
    }

    // copy the file into the buffer:
    size_t result = fread (buffer,1,lSize,pFile);
    if (result != lSize)
    {
        free(buffer);
        fprintf(stderr,"Could not read the whole file onto memory %s \n",filename);
        fclose(pFile);
        return 0;
    }

    /* the whole file is now loaded in the memory buffer. */

    // terminate
    fclose (pFile);

    buffer[lSize]=0; //Null Terminate Buffer!
    *length = (unsigned int) lSize;
    return buffer;
}



int main(int argc, char *argv[])
{
    CURL *curl;
    CURLcode res;

    curl_mime *form = NULL;
    curl_mimepart *field = NULL;
    struct curl_slist *headerlist = NULL;
    static const char buf[] = "Expect:";

    curl_global_init(CURL_GLOBAL_ALL);

    curl = curl_easy_init();
    if(curl)
    {
        /* enable TCP keep-alive for this transfer */
        curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);

        /* keep-alive idle time to 120 seconds */
        curl_easy_setopt(curl, CURLOPT_TCP_KEEPIDLE, 120L);

        /* interval time between keep-alive probes: 60 seconds */
        curl_easy_setopt(curl, CURLOPT_TCP_KEEPINTVL, 60L);


        /* Create the form */
        form = curl_mime_init(curl);

        /* Fill in the file upload field */
        field = curl_mime_addpart(form);
        curl_mime_name(field, "fileToUpload");
        curl_mime_filename(field, "test.jpg");


        //curl_mime_filedata(field, "test.jpg");
        unsigned int ourJPEGLength = 0;
        char * ourJPEGData = readFileToMemory("test.jpg",&ourJPEGLength);

        curl_mime_data(field,ourJPEGData,ourJPEGLength);

        /* Fill in the submit field too, even if this is rarely needed */
        field = curl_mime_addpart(form);
        curl_mime_name(field, "submit");
        curl_mime_data(field, "send", CURL_ZERO_TERMINATED);


        /* initialize custom header list (stating that Expect: 100-continue is not wanted */
        headerlist = curl_slist_append(headerlist, buf);
        /* what URL that receives this POST */
        //curl_easy_setopt(curl, CURLOPT_URL, "http://ammar.gr/stream/upload.php");
        curl_easy_setopt(curl, CURLOPT_URL, "127.0.0.1:8080");

        if((argc == 2) && (!strcmp(argv[1], "noexpectheader")))
            /* only disable 100-continue header if explicitly requested */
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist);
        curl_easy_setopt(curl, CURLOPT_MIMEPOST, form);

        /* Perform the request, res will get the return code */
        res = curl_easy_perform(curl);
        /* Check for errors */
        if(res != CURLE_OK)
            fprintf(stderr, "curl_easy_perform() failed: %s\n",
                    curl_easy_strerror(res));

        /* always cleanup */
        curl_easy_cleanup(curl);


        /* then cleanup the form */
        curl_mime_free(form);
        /* free slist */
        curl_slist_free_all(headerlist);
    }
    return 0;
}
