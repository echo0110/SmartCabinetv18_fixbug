#ifndef __HARDWARE_H__
#define __HARDWARE_H__

#ifdef HARDWARE_DEBUG
	#define DM9000_DEBUGF(debug, message) do { \
                               if ( \
                                   ((debug) & DM9000_DBG_ON) && \
                                   ((debug) & DM9000_DBG_TYPES_ON)) { \
                                 LWIP_PLATFORM_DIAG(message); \
                                 if ((debug) & LWIP_DBG_HALT) { \
                                   while(1); \
                                 } \
                               } \
                             } while(0)

#else  /* DM9000_DEBUG */
	#define DM9000_DEBUGF(debug, message) 
#endif /* DM9000_DEBUG */

#endif /* __HARDWARE_H__ */
