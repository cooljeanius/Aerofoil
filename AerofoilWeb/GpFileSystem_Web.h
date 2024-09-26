#pragma once

#include "IGpFileSystem.h"
#include "GpFileSystem_Web_Resources.h"

#include "GpCoreDefs.h"

#include <string>
#include <stdio.h>

struct IGpMutex;

class GpFileSystem_Web final : public IGpFileSystem
{
public:
	GpFileSystem_Web();
	~GpFileSystem_Web();
	
	void Init();

	bool FileExists(PortabilityLayer::VirtualDirectory_t virtualDirectory, const char *path) override;
	bool FileLocked(PortabilityLayer::VirtualDirectory_t virtualDirectory, const char *path, bool &exists) override;
	GpIOStream *OpenFileNested(PortabilityLayer::VirtualDirectory_t virtualDirectory, char const* const* subPaths, size_t numSubPaths, bool writeAccess, GpFileCreationDisposition_t createDisposition) override;
	bool DeleteFile(PortabilityLayer::VirtualDirectory_t virtualDirectory, const char *path, bool &existed) GP_ASYNCIFY_PARANOID_OVERRIDE;
	IGpDirectoryCursor *ScanDirectoryNested(PortabilityLayer::VirtualDirectory_t virtualDirectory, char const* const* paths, size_t numPaths) override;

	bool ValidateFilePath(const char *path, size_t pathLen) const override;
	bool ValidateFilePathUnicodeChar(uint32_t ch) const override;

	void SetDelayCallback(DelayCallback_t delayCallback) override;

	static void MarkFSStateDirty();
	static void SyncDownloadFile(const std::string &filePath, const std::string &prettyName);
	static void FlushFS();

	static GpFileSystem_Web *GetInstance();

private:
	struct ScanDirectoryNestedContext
	{
		GpFileSystem_Web *m_this;

		IGpDirectoryCursor *m_returnValue;
		PortabilityLayer::VirtualDirectory_t m_virtualDirectory;
		char const *const *m_paths;
		size_t m_numPaths;
	};

	static void ScanDirectoryNestedThunk(void *context);
	IGpDirectoryCursor *ScanDirectoryNestedInternal(PortabilityLayer::VirtualDirectory_t virtualDirectory, char const* const* paths, size_t numPaths);

	IGpDirectoryCursor *ScanDirectory(PortabilityLayer::VirtualDirectory_t virtualDirectory, char const* const* paths, size_t numPaths);
	
	static const GpFileSystem_Web_Resources::FileCatalog *GetCatalogForVirtualDirectory(PortabilityLayer::VirtualDirectory_t virtualDirectory);

	static IGpDirectoryCursor *ScanCatalog(const GpFileSystem_Web_Resources::FileCatalog &catalog);

	bool ResolvePath(PortabilityLayer::VirtualDirectory_t virtualDirectory, char const* const* paths, size_t numPaths, bool trailingSlash, std::string &resolution, bool &outIsIDB);

	DelayCallback_t m_delayCallback;

	std::string m_prefsPath;
	std::string m_basePath;
	std::string m_exportPath;
	static bool ms_fsStateDirty;

	static GpFileSystem_Web ms_instance;
};
