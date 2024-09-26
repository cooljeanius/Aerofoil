#pragma once

#if __cplusplus >= 199711L
#define GP_IS_CPP11 1
#else
#define GP_IS_CPP11 0
#endif

#if GP_IS_CPP11
#define GP_DELETED = delete
#else
#ifndef nullptr
#define nullptr 0
#endif

#ifndef override
#define override
#endif

#ifndef final
#define final
#endif

#define GP_DELETED
#endif
