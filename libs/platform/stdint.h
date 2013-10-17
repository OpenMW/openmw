// Wrapper for MSVC
#ifndef _STDINT_WRAPPER_H
#define _STDINT_WRAPPER_H

#if (_MSC_VER >= 1600)

#include <cstdint>

#else

#include <boost/cstdint.hpp>

// Pull the boost names into the global namespace for convenience
using boost::int32_t;
using boost::uint32_t;
using boost::int64_t;
using boost::uint64_t;

#endif

#endif
