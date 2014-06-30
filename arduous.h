#ifndef _ARDUOUS_H_
#define _ARDUOUS_H_


/* Global constants */
#ifndef MAXTHREADS
#define MAXTHREADS 10           /* Maximum number of threads supported */
#endif


/* Useful macros */
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#define DEBUG 1 /* degree of debugging: high => more output */
#define pdebug(level, fmt, args...) \
    do { if (DEBUG >= level) printf(fmt, ## args); } while (0)


/* Method declarations */
/* ... */


#endif /* _ARDUOUS_H_ */