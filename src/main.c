#include <stdio.h>
#include <intrinsic.h>

int main(void)
{
    printf("ZX Framework ready\n");
    while (1) intrinsic_halt();
    return 0;
}
