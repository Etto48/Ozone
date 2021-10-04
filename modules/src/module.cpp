#include <ozone.h>

extern "C" int main();
extern "C" void fini();
extern "C" void wrapper()
{
    uint64_t ret = main();
    fini();
    ozone::user::exit(ret);
}