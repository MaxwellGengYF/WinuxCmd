#include <iostream>
#ifndef WINUXCMD_BIN_DIR
#define WINUXCMD_BIN_DIR L"."
#endif
int main() {
    std::wcout << L"WINUXCMD_BIN_DIR: " << WINUXCMD_BIN_DIR << std::endl;
    return 0;
}
