#pragma once

#include <cstdint>

#ifdef __cplusplus
extern "C" {
#endif

int32_t LoadFileToMem(const char *relativefilepath, char **fileOut, uint32_t *sizeOut);
uint32_t load_loader_elf_from_sd(unsigned char *baseAddress, const char *relativePath);

#ifdef __cplusplus
}
#endif
