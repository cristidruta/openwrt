#ifndef _ADAPT_LOG_H_
#define _ADAPT_LOG_H_


#ifdef __cplusplus   
extern "C" {        
#endif



#define LOG_EMERG        0      /* system is unusable */
#define LOG_ALERT        1      /* action must be taken immediately */
#define LOG_CRIT         2      /* critical conditions */
#define LOG_ERR          3      /* error conditions */
#define LOG_WARNING      4      /* warning conditions */
#define LOG_NOTICE       5      /* normal but signification condition */
#define LOG_INFO         6      /* informational */
#define LOG_DEBUG        7      /* debug-level messages */



#define MAX_LOG_LINE_LENGTH      1024
#define GATEWAY_ADAPT   "[gateway-adapt]"

typedef enum
{
    LOG_LEVEL_ERR    = 3, /**< Message at error level. */
    LOG_LEVEL_DEBUG  = 7  /**< Message at debug level. */
} LOG_LEVEL;

extern void log_log(LOG_LEVEL log_level, const char *module, const char *func, unsigned int line_num, const char *p_fmt, ... );

#define adapt_error(args...)  log_log(LOG_LEVEL_ERR, GATEWAY_ADAPT, __FUNCTION__, __LINE__, args)
#define adapt_debug(args...)  log_log(LOG_LEVEL_DEBUG, GATEWAY_ADAPT, __FUNCTION__, __LINE__, args)


#ifdef __cplusplus
}
#endif

#endif /* _ADAPT_LOG_H_ */
