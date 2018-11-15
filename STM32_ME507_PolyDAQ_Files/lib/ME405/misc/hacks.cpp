//*************************************************************************************
/** \file hacks.cpp
 *  @brief   Source code for extras to make the GCC compiler work on our 
 *           microcontrollers.
 *  @details This file contains a set of enhancements to the GCC system for our 
 *           embedded processors which support some of the more helpful C++ features 
 *           that aren't present or working well in the normal GCC distribution 
 *           (though I have no idea why). Some information used here came from an 
 *           AVR-Freaks posting at: 
 *       \li http://www.avrfreaks.net/index.php?name=PNphpBB2&file=viewtopic&p=410870
 * 
 *           Other stuff is added to streamline the connection between the ME405 
 *           library and FreeRTOS. Of particular importance are FreeRTOS versions of 
 *           @c new and @c delete.
 *
 *  Revisions
 *    \li 04-12-2008 JRR Original file, material from source above
 *    \li 09-30-2012 JRR Added code to make memory allocation work with FreeRTOS
 *    \li 08-05-2014 JRR Ported to allow use with GCC for STM32's as well as AVR's
 *
 *  License:
 *    This file is copyright 2014 by JR Ridgely and released under the Lesser GNU 
 *    Public License, version 2. It intended for educational use only, but its use
 *    is not limited thereto. */
/*    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
 *    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
 *    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
 *    ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE 
 *    LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUEN-
 *    TIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS 
 *    OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
 *    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, 
 *    OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 *    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. */
//*************************************************************************************

#include "hacks.h"
#include "FreeRTOS.h"


//-------------------------------------------------------------------------------------
/** @brief   FreeRTOS version of the C++ memory allocation operator.
 *  @details This operator maps @c new to the FreeRTOS memory allocation function, 
 *           which is configurable and thread safe, because the regular @c malloc()
 *           shouldn't be used in a FreeRTOS program. 
 *  @param   size The number of bytes which are to be allocated
 *  @return  A pointer to the memory area which has just been allocated
 */

void* operator new (size_t size)
{
	return pvPortMalloc (size);
}


//-------------------------------------------------------------------------------------
/** @brief   FreeRTOS version of the C++ memory deallocation operator.
 *  @details This operator maps @c delete to the FreeRTOS memory deallocation
 *           function, which is configurable and thread safe. Note that memory 
 *           deallocation is only possible when using some (not all) versions of 
 *           the memory allocator in FreeRTOS; for example, @c heap_1.c doesn't 
 *           implement deallocation at all. 
 *  @param   ptr A pointer to the object whose contents are to be deleted
 */

void operator delete (void *ptr)
{
	if (ptr)
	{
		vPortFree (ptr);
	}
}


//-------------------------------------------------------------------------------------
/** @brief   FreeRTOS version of the C++ array memory allocation operator.
 *  @details This operator maps the array version of @c new to the FreeRTOS memory
 *           allocation function, which is configurable and thread safe, because 
 *           the regular @c malloc() shouldn't be used in a FreeRTOS program. 
 *  @param   size The number of bytes which are to be allocated
 *  @return  A pointer to the memory area which has just been allocated
 */

void* operator new[] (size_t size)
{
	return pvPortMalloc (size);
}


//-------------------------------------------------------------------------------------
/** @brief   FreeRTOS version of the C++ array memory deallocation operator.
 *  @details This operator maps the array version of @c delete to the FreeRTOS 
 *           memory deallocation function, which is configurable and thread safe. 
 *           Note that memory deallocation is only possible when using some (not 
 *           all) versions of the memory allocator in FreeRTOS; for example, 
 *           @c heap_1.c doesn't allow deallocation at all. 
 *  @param   ptr A pointer to the memory area whose contents are to be deleted
 */

void operator delete[] (void *ptr)
{
	if (ptr)
	{
		vPortFree (ptr);
	}
}


#ifdef __ARMEL__

	extern "C"
	{
		/// @cond NO_DOXY The following line isn't to be documented by Doxygen
		// size_t _sbrk _PARAMS ((int));
		/// @endcond

		//-----------------------------------------------------------------------------
		/** @brief   Increase program heap space. 
		 *  @details This function is used by @c malloc() and related functions, 
		 *           including the @c new operator, to increase heap space for dynamic
		 *           memory allocation. It would normally be supplied by the operating
		 *           system of a PC or similar computer, but some code only has an RTOS
		 *           which needs this function to help with memory management. Some
		 *           other code libraries supply @c _sbrk, for example AVR-libc.
		 *  @param   incr The size of the memory increment to add to the heap space
		 *  @return  Where the heap used to end, or (-1) if we've run out of memory
		 */

		size_t _sbrk (int incr)
		{
			extern char _ebss;                  // Defined by the linker
			static char *heap_end = 0;          // Tracks end of heap space
			char *prev_heap_end;                // Stores temporary value

			if (heap_end == 0)                  // 
			{
				heap_end = &_ebss;
			}

			prev_heap_end = heap_end;
			heap_end += incr;

			return ((size_t)prev_heap_end);
		}
	}
#endif // __ARMEL__


//-------------------------------------------------------------------------------------
/** @brief   Utility function for virtual methods.
 *  @details This function is used to help make templates and virtual methods work. 
 *           The author isn't sure exactly how it works, but the Interwebs seem to 
 *           have half a clue. Information about this and related functions can be 
 *           found at
 *     http://www.avrfreaks.net/index.php?name=PNphpBB2&file=printview&t=57466&start=0
 *  @param   g A pointer to some random thing
 *  @return  Something that's pointed to by g
 */

extern "C"
{
	int __cxa_guard_acquire (__guard *g)
	{
		return !*(char *)(g);
	}
}


//-------------------------------------------------------------------------------------
/** @brief   Utility function for virtual methods.
 *  @details This function is used to help make templates and virtual methods work. 
 *           The author isn't sure exactly how it works, but the Interwebs seem to 
 *           have half a clue. Information about this and related functions can be 
 *           found at
 *     http://www.avrfreaks.net/index.php?name=PNphpBB2&file=printview&t=57466&start=0
 *  @param   g A pointer to some random thing
 */

extern "C"
{
	void __cxa_guard_release (__guard *g)
	{
		*(char *)g = 1;
	}
}


//-------------------------------------------------------------------------------------
/** @brief   Utility function for virtual methods.
 *  @details This function is used to help make templates and virtual methods work. 
 *           The author isn't sure exactly how it works, but the Interwebs seem to 
 *           have half a clue. Information about this and related functions can be 
 *           found at
 *     http://www.avrfreaks.net/index.php?name=PNphpBB2&file=printview&t=57466&start=0
 *  @param   g A pointer to some random thing
 */

extern "C"
{
	void __cxa_guard_abort (__guard *g)
	{
		(void)g;                            // Shuts up a dumb compiler warning
	}
}


//-------------------------------------------------------------------------------------
/** @brief   Stub function for virtual methods, maybe.
 *  @details This function seems to be used to help make pure virtual methods work. 
 *           The author isn't sure exactly how it works (OK, clueless), but it might 
 *           be a function that fills in for the body of a pure virtual method, 
 *           \e i.e. a method which doesn't have any body in the regular source files. 
 *           Or maybe it's a secret note from space aliens, who knows. Information 
 *           about this and related functions can be found at
 *     http://www.avrfreaks.net/index.php?name=PNphpBB2&file=printview&t=57466&start=0
 */

extern "C"
{    
	void __cxa_pure_virtual (void)
	{
	}
}
