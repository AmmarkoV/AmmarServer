/** @file asvio.h
* @brief ASVIO ( AmmarSerVer I/O ) bridge : the C side of the asvio Python package
*
* AmmarServer handler threads turn every /asvio/run.json request into a job , the Python side takes jobs with
* asvio_next_job() and answers them with asvio_job_update() / asvio_job_finish() , and /asvio/poll.json
* long-polls a job until it changes. Python never runs on a server thread , see PLAN.md for the protocol.
*
* @author Ammar Qammaz (AmmarkoV)
*/
#ifndef ASVIO_H_INCLUDED
#define ASVIO_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#define ASVIO_JOB_ID_SIZE 33
/** @brief The multipart parser keeps at most MAX_HTTP_POST_BOUNDARY_COUNT ( 32 ) parts , three of them are api/session/data */
#define ASVIO_MAX_FILES 32

/** @brief One uploaded file of a job , content is NOT NUL terminated */
struct asvio_file
{
  char * field;
  char * filename;
  char * content;
  unsigned int length;
};

/**
* @brief What a job asked for. Mirrored by python/asvio/_bridge.py , keep the two in sync.
*        Everything in here stays valid until asvio_job_finish() is called for the job.
*/
struct asvio_job
{
  char id[ASVIO_JOB_ID_SIZE];
  char * api;
  char * session;
  char * data;
  unsigned int dataLength;
  unsigned int fileCount;
  struct asvio_file files[ASVIO_MAX_FILES];
};

/**
* @brief Start serving webroot ( frontend files + asvio_out/ ) plus the /asvio/ routes
* @param Instance name , used for log filenames
* @param IP to bind , e.g. "0.0.0.0"
* @param Port to bind
* @param Web root directory
* @param Largest accepted upload ( whole POST ) in MB
* @param Largest poll response in MB , every poll request allocates and clears this much
* @retval 1=Success,0=Fail
*/
int asvio_start(const char * name,const char * ip,unsigned int port,const char * webroot,unsigned int maxUploadMB,unsigned int maxResponseMB);

/** @retval 1 while the server is running */
int asvio_running();

/** @brief Wake every waiting poll and asvio_next_job() , then stop the server */
void asvio_stop();

/**
* @brief Take the oldest job nobody has taken yet , waiting up to timeoutMS for one
* @retval The job , or 0 on timeout / when stopping
*/
struct asvio_job * asvio_next_job(unsigned int timeoutMS);

/**
* @brief Publish a new state of a job : bumps its seq and wakes everyone polling it
* @param Job id
* @param Status string ( "queued" , "running" )
* @param JSON payload handed to pollers as "data" , 0 keeps the previous one
* @param Payload length
* @retval 1=Success,0=Unknown or finished job
*/
int asvio_job_update(const char * jobID,const char * status,const char * payload,unsigned int payloadLength);

/**
* @brief Like asvio_job_update , but final : status should be "done" , "error" or "cancelled".
*        The job's input ( struct asvio_job ) is freed , and the job is forgotten ASVIO_FINISHED_JOB_LIFETIME seconds later
* @retval 1=Success,0=Unknown or finished job
*/
int asvio_job_finish(const char * jobID,const char * status,const char * payload,unsigned int payloadLength);

/** @retval 1 when a client asked to cancel this job */
int asvio_job_cancelled(const char * jobID);

#ifdef __cplusplus
}
#endif

#endif // ASVIO_H_INCLUDED
