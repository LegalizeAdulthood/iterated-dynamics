#include "compiler.h"

int main(int argc, char *argv[])
{
    return hc::compiler(argc, argv).process();
}
