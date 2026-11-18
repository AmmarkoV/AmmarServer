#ifndef BANLIST_H_INCLUDED
#define BANLIST_H_INCLUDED


int executeBanlist(const char * filename);

/**
* @brief Automatic , temporary IP bans shared by every AmmarServer process started from the same directory
*        ( they all read/write BANLIST_SHARED_FILE , cwd relative like the log/ files ). Loopback is never banned.
*        BANLIST_STRIKES_TO_BAN strikes within BANLIST_STRIKE_WINDOW_SECONDS ban an IP for BANLIST_FIRST_BAN_SECONDS ,
*        every repeat ban doubles that up to BANLIST_MAX_BAN_SECONDS , a malicious signal bans for BANLIST_MAX_BAN_SECONDS at once.
*/
int banlist_isIPBanned(const char * ip);

/** @brief Count one strike against ip ( e.g. a failed login ) , @retval 1=ip is now banned , 0=not banned */
int banlist_signalBadBehaviour(const char * ip);

/** @brief Ban ip straight away for BANLIST_MAX_BAN_SECONDS , @retval 1=ip is now banned , 0=exempt/failure */
int banlist_signalMalicious(const char * ip);

/** @brief 1 if resource is something only a vulnerability scanner asks for ( e.g. /wp-login.php , /.env ) , in any directory */
int banlist_isScannerProbe(const char * resource);

#endif // BANLIST_H_INCLUDED
