//
// Created by koncord on 23.01.16.
//

#ifndef PLATFORM_HPP
#define PLATFORM_HPP

#if _MSC_VER
#ifdef _M_X86
#define ARCH_X86
#endif
#endif

#if __GNUC__
#ifdef __i386__
#define ARCH_X86
#endif
#endif

#endif //PLATFORM_HPP
