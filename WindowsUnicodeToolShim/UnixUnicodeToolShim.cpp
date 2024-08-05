#include "WindowsUnicodeToolShim.h"

#include <cstdio>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

#include <dirent.h>
#include <sys/stat.h>

// Linux and macOS (and probably other non-Windows systems in general) do not
// have separate file system or command line APIs for different text encodings.
// On these systems UTF-8 is commonly the system encoding and, if so, argv will
// typically be UTF-8 encoded, though it is the calling process's responsibility
// to ensure this, as argv is passed verbatim as a sequence of byte strings.
//
// Filesystems on Unix-like systems are typically encoding-unaware, in which
// case the actual encoding used should match the system encoding, or else
// manual intervention is required.
//
// On macOS, HFS+ and APFS filenames are encoded as UTF-16 and UTF-8
// respectively, and the C filesystem API accepts UTF-8 strings. There are some
// gotchas relating to Unicode normalization, where HFS+ enforces a specific
// form of normalization at the filesystem layer but APFS does not, and using
// the C API to access the filesystem may avoid automatic normalization that
// higher-level macOS API functions may perform.
//
// In summary, text encoding is still a hairy problem on every computer system,
// though on the major non-Windows systems, assuming UTF-8 encoding is
// reasonable. For now, this header simply maps the WindowsUnicodeToolShim
// functions to their regular C API counterparts.
int toolMain(int argc, const char **argv);
int main(int argc, const char **argv) { return toolMain(argc, argv); }

FILE *fopen_utf8(const char *path, const char *options) { return fopen(path, options); }
int fputs_utf8(const char *str, FILE *f) { return fputs(str, f); }
int mkdir_utf8(const char *path) { return mkdir(path, 0777); }

void TerminateDirectoryPath(std::string &path)
{
	const size_t len = path.length();

	if (len == 0 || path[len - 1] != '/')
		path.push_back('/');
}

void ScanDirectoryForExtension
(
	std::vector<std::string> &outPaths,
	const char *path,
	const char *ending,
	bool recursive
) {
	DIR *dir = opendir(path);
	if (!dir)
	{
		return;
	}

	size_t endingLen = strlen(ending);

	dirent *ent;
	while ((ent = readdir(dir)))
	{
		if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
			continue;
		else if (recursive && ent->d_type == DT_DIR)
		{
			std::string tmpPath(path);
			tmpPath.append("/");
			tmpPath.append(ent->d_name);
			ScanDirectoryForExtension(
				outPaths,
				tmpPath.c_str(),
				ending,
				recursive
			);
		}
		else
		{
			size_t nameLen = strlen(ent->d_name);

			if (endingLen <= nameLen && memcmp
				(
					ent->d_name + nameLen - endingLen,
					ending,
					endingLen
				) == 0
			) {
				std::string tmpPath(path);
				tmpPath.append("/");
				tmpPath.append(ent->d_name);
				outPaths.push_back(std::move(tmpPath));
			}
		}
	}
	closedir(dir);
}
