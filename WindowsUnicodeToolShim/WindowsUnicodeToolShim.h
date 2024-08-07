#include <string>
#include <vector>
#include <cstdint>
#include <stdio.h>

FILE *fopen_utf8(const char *path, const char *options);
int fseek_int64(FILE *f, int64_t offset, int origin);
int64_t ftell_int64(FILE *f);
int fputs_utf8(const char *str, FILE *f);
int mkdir_utf8(const char *path);
void TerminateDirectoryPath(std::string &path);
void ScanDirectoryForExtension(std::vector<std::string>& outPaths, const char *path, const char *ending, bool recursive);
