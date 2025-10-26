#ifndef STD_STDBOOL_H
#define STD_STDBOOL_H

#define true 1
#define false 0

#ifndef __cplusplus
typedef unsigned char bool;
#endif

#define isfalse(x) ((x) == false)
#define istrue(x) ((x) == true)

#endif //STD_STDBOOL_H