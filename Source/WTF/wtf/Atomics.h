/*
 * Copyright (C) 2007, 2008, 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2007 Justin Haygood (jhaygood@reaktix.com)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer. 
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution. 
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * Note: The implementations of InterlockedIncrement and InterlockedDecrement are based
 * on atomic_increment and atomic_exchange_and_add from the Boost C++ Library. The license
 * is virtually identical to the Apple license above but is included here for completeness.
 *
 * Boost Software License - Version 1.0 - August 17th, 2003
 * 
 * Permission is hereby granted, free of charge, to any person or organization
 * obtaining a copy of the software and accompanying documentation covered by
 * this license (the "Software") to use, reproduce, display, distribute,
 * execute, and transmit the Software, and to prepare derivative works of the
 * Software, and to permit third-parties to whom the Software is furnished to
 * do so, all subject to the following:
 * 
 * The copyright notices in the Software and this entire statement, including
 * the above license grant, this restriction and the following disclaimer,
 * must be included in all copies of the Software, in whole or in part, and
 * all derivative works of the Software, unless such copies or derivative
 * works are solely in the form of machine-executable object code generated by
 * a source language processor.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
 * SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
 * FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifndef Atomics_h
#define Atomics_h

#include <wtf/Platform.h>
#include <wtf/StdLibExtras.h>
#include <wtf/UnusedParam.h>

#if OS(WINDOWS)
#include <windows.h>
#elif OS(DARWIN)
#include <libkern/OSAtomic.h>
#elif OS(QNX)
#include <atomic.h>
#elif OS(ANDROID)
#include <sys/atomics.h>
#elif COMPILER(GCC)
#if ((__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ >= 2))) && !defined(__LSB_VERSION__)
#include <ext/atomicity.h>
#else
#include <bits/atomicity.h>
#endif
#endif

namespace WTF {

#if OS(WINDOWS)
#define WTF_USE_LOCKFREE_THREADSAFEREFCOUNTED 1

#if COMPILER(MINGW) || COMPILER(MSVC7_OR_LOWER) || OS(WINCE)
inline int atomicIncrement(int* addend) { return InterlockedIncrement(reinterpret_cast<long*>(addend)); }
inline int atomicDecrement(int* addend) { return InterlockedDecrement(reinterpret_cast<long*>(addend)); }
#else
inline int atomicIncrement(int volatile* addend) { return InterlockedIncrement(reinterpret_cast<long volatile*>(addend)); }
inline int atomicDecrement(int volatile* addend) { return InterlockedDecrement(reinterpret_cast<long volatile*>(addend)); }
#endif

#elif OS(DARWIN)
#define WTF_USE_LOCKFREE_THREADSAFEREFCOUNTED 1

inline int atomicIncrement(int volatile* addend) { return OSAtomicIncrement32Barrier(const_cast<int*>(addend)); }
inline int atomicDecrement(int volatile* addend) { return OSAtomicDecrement32Barrier(const_cast<int*>(addend)); }

#elif OS(QNX)
#define WTF_USE_LOCKFREE_THREADSAFEREFCOUNTED 1

// Note, atomic_{add, sub}_value() return the previous value of addend's content.
inline int atomicIncrement(int volatile* addend) { return static_cast<int>(atomic_add_value(reinterpret_cast<unsigned volatile*>(addend), 1)) + 1; }
inline int atomicDecrement(int volatile* addend) { return static_cast<int>(atomic_sub_value(reinterpret_cast<unsigned volatile*>(addend), 1)) - 1; }

#elif OS(ANDROID)
#define WTF_USE_LOCKFREE_THREADSAFEREFCOUNTED 1

// Note, __atomic_{inc, dec}() return the previous value of addend's content.
inline int atomicIncrement(int volatile* addend) { return __atomic_inc(addend) + 1; }
inline int atomicDecrement(int volatile* addend) { return __atomic_dec(addend) - 1; }

#elif COMPILER(GCC) && !CPU(SPARC64) // sizeof(_Atomic_word) != sizeof(int) on sparc64 gcc
#define WTF_USE_LOCKFREE_THREADSAFEREFCOUNTED 1

inline int atomicIncrement(int volatile* addend) { return __gnu_cxx::__exchange_and_add(addend, 1) + 1; }
inline int atomicDecrement(int volatile* addend) { return __gnu_cxx::__exchange_and_add(addend, -1) - 1; }

#endif

#if OS(WINDOWS)
inline bool weakCompareAndSwap(volatile unsigned* location, unsigned expected, unsigned newValue)
{
#if OS(WINCE)
    return InterlockedCompareExchange(reinterpret_cast<LONG*>(const_cast<unsigned*>(location)), static_cast<LONG>(newValue), static_cast<LONG>(expected)) == static_cast<LONG>(expected);
#else
    return InterlockedCompareExchange(reinterpret_cast<LONG volatile*>(location), static_cast<LONG>(newValue), static_cast<LONG>(expected)) == static_cast<LONG>(expected);
#endif
}

inline bool weakCompareAndSwap(void*volatile* location, void* expected, void* newValue)
{
    return InterlockedCompareExchangePointer(location, newValue, expected) == expected;
}
#else // OS(WINDOWS) --> not windows
#if COMPILER(GCC) && !COMPILER(CLANG) // Work around a gcc bug 
inline bool weakCompareAndSwap(volatile unsigned* location, unsigned expected, unsigned newValue) 
#else
inline bool weakCompareAndSwap(unsigned* location, unsigned expected, unsigned newValue)
#endif
{
#if ENABLE(COMPARE_AND_SWAP)
    bool result;
#if CPU(X86) || CPU(X86_64)
    asm volatile(
        "lock; cmpxchgl %3, %2\n\t"
        "sete %1"
        : "+a"(expected), "=q"(result), "+m"(*location)
        : "r"(newValue)
        : "memory"
        );
#elif CPU(ARM_THUMB2)
    unsigned tmp;
    asm volatile(
        "movw %1, #1\n\t"
        "ldrex %2, %0\n\t"
        "cmp %3, %2\n\t"
        "bne.n 0f\n\t"
        "strex %1, %4, %0\n\t"
        "0:"
        : "+Q"(*location), "=&r"(result), "=&r"(tmp)
        : "r"(expected), "r"(newValue)
        : "memory");
    result = !result;
#else
#error "Bad architecture for compare and swap."
#endif
    return result;
#else
    UNUSED_PARAM(location);
    UNUSED_PARAM(expected);
    UNUSED_PARAM(newValue);
    CRASH();
    return 0;
#endif
}

inline bool weakCompareAndSwap(void*volatile* location, void* expected, void* newValue)
{
#if ENABLE(COMPARE_AND_SWAP)
#if CPU(X86_64)
    bool result;
    asm volatile(
        "lock; cmpxchgq %3, %2\n\t"
        "sete %1"
        : "+a"(expected), "=q"(result), "+m"(*location)
        : "r"(newValue)
        : "memory"
        );
    return result;
#else
    return weakCompareAndSwap(bitwise_cast<unsigned*>(location), bitwise_cast<unsigned>(expected), bitwise_cast<unsigned>(newValue));
#endif
#else // ENABLE(COMPARE_AND_SWAP)
    UNUSED_PARAM(location);
    UNUSED_PARAM(expected);
    UNUSED_PARAM(newValue);
    CRASH();
    return 0;
#endif // ENABLE(COMPARE_AND_SWAP)
}
#endif // OS(WINDOWS) (end of the not-windows case)

inline bool weakCompareAndSwapUIntPtr(volatile uintptr_t* location, uintptr_t expected, uintptr_t newValue)
{
    return weakCompareAndSwap(reinterpret_cast<void*volatile*>(location), reinterpret_cast<void*>(expected), reinterpret_cast<void*>(newValue));
}

#if CPU(ARM_THUMB2)

inline void memoryBarrierAfterLock()
{
    asm volatile("dmb" ::: "memory");
}

inline void memoryBarrierBeforeUnlock()
{
    asm volatile("dmb" ::: "memory");
}

#else

inline void memoryBarrierAfterLock() { }
inline void memoryBarrierBeforeUnlock() { }

#endif

} // namespace WTF

#if USE(LOCKFREE_THREADSAFEREFCOUNTED)
using WTF::atomicDecrement;
using WTF::atomicIncrement;
#endif

#endif // Atomics_h