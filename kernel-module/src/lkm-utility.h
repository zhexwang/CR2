#ifndef __LKM_UTILITY_H_
#define __LKM_UTILITY_H_
#include <linux/module.h>

#define PRINTK(format, ...) do{ printk(KERN_ERR format, ##__VA_ARGS__);}while(0)
#define ASSERT(cond)       do{if(!(cond)) {PRINTK("ASSERT failed: %s:%-4d ", __FILE__, __LINE__); abort();}}while(0)
#define ASSERTM(cond, format, ...) do{if(!(cond)) {PRINTK("ASSERT failed: %s:%-4d "format, __FILE__, __LINE__, ## __VA_ARGS__); abort();}}while(0)

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


#endif
