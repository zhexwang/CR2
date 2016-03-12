#pragma once

#include "type.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#define COLOR_RED "\033[01;31m"
#define COLOR_GREEN "\033[32m"
#define COLOR_HIGH_GREEN "\033[01;32m"
#define COLOR_YELLOW "\033[33m"
#define COLOR_BLUE "\033[01;36m"
#define COLOR_END "\033[0m"

#define PRINT(format, ...) do{ fprintf(stderr, format, ##__VA_ARGS__);}while(0)

#define INFO(format, ...) do{ fprintf(stderr,COLOR_HIGH_GREEN format COLOR_END, ##__VA_ARGS__);}while(0)
#define BLUE(format, ...) do{ fprintf(stderr,COLOR_BLUE format COLOR_END, ##__VA_ARGS__);}while(0)
#define YELLOW(format, ...) do{ fprintf(stderr,COLOR_YELLOW format COLOR_END, ##__VA_ARGS__);}while(0)
#define ERR(format, ...) do{ fprintf(stderr,COLOR_RED format COLOR_END, ##__VA_ARGS__);}while(0)

#define FATAL(cond, format, ...) do{if(cond) {ERR("FATAL: %s:%-4d "format, __FILE__, __LINE__, ## __VA_ARGS__); abort();}}while(0)
#define PERROR(cond, format) do{if(!(cond)){perror(format); abort();}}while(0)

#ifdef _DEBUG
#define ASSERT(cond)       do{if(!(cond)) {ERR("ASSERT failed: %s:%-4d ", __FILE__, __LINE__); abort();}}while(0)
#define ASSERTM(cond, format, ...) do{if(!(cond)) {ERR("ASSERT failed: %s:%-4d "format, __FILE__, __LINE__, ## __VA_ARGS__); abort();}}while(0)
#define NOT_IMPLEMENTED(who)        do{ERR("%s() in %s:%-4d is not implemented by %s\n", __FUNCTION__, __FILE__, __LINE__, #who); abort();} while (0)
#else
#define ASSERT(cond) 
#define ASSERTM(cond, format, ...) 
#define NOT_IMPLEMENTED(who)       
#endif

/*********debug********/
//#define TRACE_DEBUG
//#define LAST_RBBL_DEBUG

/*********opt**********/
#define USE_CALLER_SAVED_DESTROY_OPT
#define USE_JMPIN_REG_DESTORY_OPT
#define USE_JMPIN_MEM_INDEX_DESTORY_OPT
#define USE_MAIN_SWITCH_CASE_COPY_OPT
#define USE_CLOSE_CLEAN_CC_OPT
#define USE_TRAMP_RECORD_OPT

//bits define
#define BITS_ARE_SET_ANY(value, bits)	   ( ((value)&(bits)) != 0 )
#define BITS_ARE_SET_ALL(value, bits)	   ( ((value)&(bits)) == (bits) )
#define BITS_ARE_SET(value, bits)	   BITS_ARE_SET_ALL(value, bits)
#define BITS_ARE_CLEAR_ANY(value, bits)	   ( ((value)&(bits)) != (bits) )
#define BITS_ARE_CLEAR_ALL(value, bits)	   ( ((value)&(bits)) == 0 )
#define BITS_ARE_CLEAR(value, bits)	   BITS_ARE_CLEAR_ALL(value, bits)
#define BITS_SET(value, bits)		  ( (value) |= (bits) )
#define BITS_CLEAR(value, bits)		  ( (value) &= ~(bits) )
#define HAS_OTHER_BITS(int, bits)	   (((int)&(~(bits))) != 0)

//page define
#define X86_PAGE_SIZE_ZERO_BITS_NUM (12)
#define X86_PAGE_SIZE (1<<X86_PAGE_SIZE_ZERO_BITS_NUM)	 // 4096
#define X86_PAGE_MASK (~(X86_PAGE_SIZE - 1))
#define X86_PAGE_OFFSET(addr) ( addr & (X86_PAGE_SIZE - 1) )
#define X86_PAGE_IS_ALIGN(addr)	( ((addr)&(X86_PAGE_SIZE-1)) == 0 )
#define X86_PAGE_ALIGN_CEIL(addr)  ( ((addr)+X86_PAGE_SIZE-1) & X86_PAGE_MASK )
#define X86_PAGE_ALIGN_FLOOR(addr) ( (addr) & X86_PAGE_MASK )

//to string define
#define btoa(x) ((x)?"true":"false")
#define TO_STRING_INTERNAL(x) #x
#define TO_STRING(x) TO_STRING_INTERNAL(x)

