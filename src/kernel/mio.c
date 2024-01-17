#include <phinix/mio.h>

u8 mem_in_byte(u32 addr)
{
    return *((volatile u8 *)addr);
}   

u16 mem_in_word(u32 addr)
{
    return *((volatile u16 *)addr);
}  

u32 mem_in_dword(u32 addr)
{
    return *((volatile u32 *)addr);
} 


void mem_out_byte(u32 addr, u8 value)
{
    *((volatile u8 *)addr) = value;
}   

void mem_out_word(u32 addr, u16 value)
{
    *((volatile u16 *)addr) = value;
}  

void mem_out_dword(u32 addr, u32 value)
{
    *((volatile u32 *)addr) = value;
} 
