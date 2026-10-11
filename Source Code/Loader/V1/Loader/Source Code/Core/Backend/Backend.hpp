#pragma once
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <Windows.h>
#include <string>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <urlmon.h>
#include <shlobj.h>
#include <shellapi.h>

#pragma comment(lib, "urlmon.lib")
#pragma comment(lib, "shell32.lib")

struct LoaderInfo {
    std::string status = "Unknown";
    std::string updated = "Unknown";
    std::string version = "Unknown";
    std::string changelogs = "";
    bool        allowLoad = false;
    bool        valid = false;
};

class Backend {
public:
    static constexpr const char* BASE_DIR = "C:\\Oblivion";
    static constexpr const char* CONFIG_DIR = "C:\\Oblivion\\Configurations";
    static constexpr const char* SOUNDS_DIR = "C:\\Oblivion\\Resources\\Sounds";
    static constexpr const char* SOUND_PATH = "C:\\Oblivion\\Resources\\Sounds\\Click.wav";

    static constexpr const char* URL_STATUS =
        "https://raw.githubusercontent.com/sn0fla/Oblivion/main/Backends/Loader/Status.txt";
    static constexpr const char* URL_INFO =
        "https://raw.githubusercontent.com/sn0fla/Oblivion/main/Backends/Loader/Information.txt";
    static constexpr const char* URL_CHANGELOGS =
        "https://raw.githubusercontent.com/sn0fla/Oblivion/main/Backends/Loader/Changelogs.txt";
    static constexpr const char* URL_OBLIVION_EXE =
        "https://raw.githubusercontent.com/sn0fla/Oblivion/main/Backends/Loader/Oblivion.exe";
    static constexpr const char* URL_CLICK_SOUND =
        "https://raw.githubusercontent.com/sn0fla/Oblivion/main/Backends/Resources/Sounds/Click.wav";

    static constexpr const char* LOCAL_VERSION = "0.0";

    static std::string GetTempDir();
    static std::string GetTempPath(const std::string& filename);
    static bool        FileExists(const std::string& path);
    static bool        CreateDirectoryTree(const std::string& path);

    static bool DownloadFile(const std::string& url,
        const std::string& localPath,
        bool               force);

    static bool EnsureDirectories();
    static bool EnsureClickSound();
    static bool RefreshInfo();
    static bool EnsureOblivionExe();
    static LoaderInfo ParseInfo();
    static std::string GetRemoteVersion();
    static bool IsVersionUpToDate();

    static bool LaunchOblivionExe();
};