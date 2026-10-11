#include "Backend.hpp"

std::string Backend::GetTempDir() {
    char tempPath[MAX_PATH]{};
    DWORD len = GetTempPathA(MAX_PATH, tempPath);
    if (len == 0) return "C:\\Temp";
    std::string dir(tempPath);
    if (!dir.empty() && dir.back() == '\\') dir.pop_back();
    return dir;
}

std::string Backend::GetTempPath(const std::string& filename) {
    return GetTempDir() + "\\" + filename;
}

bool Backend::FileExists(const std::string& path) {
    return GetFileAttributesA(path.c_str()) != INVALID_FILE_ATTRIBUTES;
}

static bool FileHasContent(const std::string& path, DWORD minBytes = 4) {
    HANDLE h = CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ,
        nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) return false;
    DWORD size = GetFileSize(h, nullptr);
    CloseHandle(h);
    return size >= minBytes;
}

static std::string ReadWholeFile(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f.is_open()) return "";
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

bool Backend::CreateDirectoryTree(const std::string& path) {
    if (SHCreateDirectoryExA(nullptr, path.c_str(), nullptr) == ERROR_SUCCESS)
        return true;
    if (GetFileAttributesA(path.c_str()) != INVALID_FILE_ATTRIBUTES)
        return true;
    return false;
}

bool Backend::DownloadFile(const std::string& url,
    const std::string& localPath,
    bool               force) {
    if (!force && FileExists(localPath)) {
        char buf[512];
        sprintf_s(buf, "OBLIVION: skip (exists) %s\n", localPath.c_str());
        OutputDebugStringA(buf);
        return true;
    }

    DeleteFileA(localPath.c_str());

    HRESULT hr = URLDownloadToFileA(nullptr, url.c_str(), localPath.c_str(), 0, nullptr);
    if (FAILED(hr)) {
        char buf[768];
        sprintf_s(buf, "OBLIVION: FAIL dl url=%s hr=0x%08X\n", url.c_str(), (unsigned)hr);
        OutputDebugStringA(buf);
        return false;
    }

    if (!FileHasContent(localPath, 4)) {
        char buf[768];
        sprintf_s(buf, "OBLIVION: FAIL empty url=%s\n", url.c_str());
        OutputDebugStringA(buf);
        return false;
    }

    std::string head = ReadWholeFile(localPath);
    if (!head.empty()) {
        std::string lower = head.substr(0, (std::min)(head.size(), (size_t)256));
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        if (lower.find("<!doctype html") != std::string::npos ||
            lower.find("<html") != std::string::npos) {
            char buf[768];
            sprintf_s(buf, "OBLIVION: FAIL html url=%s\n", url.c_str());
            OutputDebugStringA(buf);
            DeleteFileA(localPath.c_str());
            return false;
        }
    }

    char buf[768];
    sprintf_s(buf, "OBLIVION: OK   dl -> %s (%zu bytes)\n",
        localPath.c_str(), head.size());
    OutputDebugStringA(buf);
    return true;
}

bool Backend::EnsureDirectories() {
    bool ok = true;
    ok &= CreateDirectoryTree(CONFIG_DIR);
    ok &= CreateDirectoryTree(SOUNDS_DIR);
    return ok;
}

bool Backend::EnsureClickSound() {
    CreateDirectoryTree(SOUNDS_DIR);
    return DownloadFile(URL_CLICK_SOUND, SOUND_PATH, false);
}

bool Backend::RefreshInfo() {
    bool ok1 = DownloadFile(URL_STATUS, GetTempPath("Status.txt"), true);
    bool ok2 = DownloadFile(URL_INFO, GetTempPath("Information.txt"), true);
    bool ok3 = DownloadFile(URL_CHANGELOGS, GetTempPath("Changelogs.txt"), true);

    char buf[256];
    sprintf_s(buf, "OBLIVION: RefreshInfo Status=%d Info=%d Change=%d\n",
        (int)ok1, (int)ok2, (int)ok3);
    OutputDebugStringA(buf);
    return ok1 && ok2 && ok3;
}

bool Backend::EnsureOblivionExe() {
    return DownloadFile(URL_OBLIVION_EXE, GetTempPath("Oblivion.exe"), true);
}

static std::string Trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    size_t end = s.find_last_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    return s.substr(start, end - start + 1);
}

LoaderInfo Backend::ParseInfo() {
    LoaderInfo info;

    std::ifstream sf(GetTempPath("Status.txt"));
    if (sf.is_open()) {
        std::string line;
        while (std::getline(sf, line)) {
            line = Trim(line);
            if (line.empty() || line[0] == '#') continue;
            auto eq = line.find('=');
            if (eq == std::string::npos) continue;
            std::string key = Trim(line.substr(0, eq));
            std::string val = Trim(line.substr(eq + 1));
            if (key == "version") info.version = val;
            else if (key == "allow_load") {
                std::transform(val.begin(), val.end(), val.begin(), ::tolower);
                info.allowLoad = (val == "true" || val == "1" || val == "yes");
            }
        }
        info.valid = true;
    }

    std::ifstream inf(GetTempPath("Information.txt"));
    if (inf.is_open()) {
        std::string line;
        while (std::getline(inf, line)) {
            line = Trim(line);
            if (line.empty() || line[0] == '#') continue;
            auto eq = line.find('=');
            if (eq == std::string::npos) continue;
            std::string key = Trim(line.substr(0, eq));
            std::string val = Trim(line.substr(eq + 1));
            if (key == "status")       info.status = val;
            else if (key == "updated") info.updated = val;
            else if (key == "version") info.version = val;
        }
    }

    std::ifstream cf(GetTempPath("Changelogs.txt"));
    if (cf.is_open()) {
        std::stringstream ss;
        ss << cf.rdbuf();
        info.changelogs = ss.str();
    }
    return info;
}

std::string Backend::GetRemoteVersion() {
    std::ifstream file(GetTempPath("Status.txt"));
    if (!file.is_open()) return "";
    std::string line;
    while (std::getline(file, line)) {
        line = Trim(line);
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key = Trim(line.substr(0, eq));
        std::string val = Trim(line.substr(eq + 1));
        if (key == "version") return val;
    }
    return "";
}

bool Backend::IsVersionUpToDate() {
    std::string remote = GetRemoteVersion();
    if (remote.empty()) return false;
    return remote == LOCAL_VERSION;
}

static std::wstring ToWideStr(const std::string& src) {
    if (src.empty()) return L"";
    int wlen = MultiByteToWideChar(CP_UTF8, 0, src.c_str(), (int)src.size(), nullptr, 0);
    std::wstring out(wlen, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, src.c_str(), (int)src.size(), &out[0], wlen);
    return out;
}

bool Backend::LaunchOblivionExe() {
    std::string exePath = GetTempPath("Oblivion.exe");
    if (!FileExists(exePath)) {
        char buf[MAX_PATH + 64];
        sprintf_s(buf, "OBLIVION: Oblivion.exe not found at %s\n", exePath.c_str());
        OutputDebugStringA(buf);
        return false;
    }

    DWORD fileSize = 0;
    {
        HANDLE h = CreateFileA(exePath.c_str(), GENERIC_READ, FILE_SHARE_READ,
            nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (h != INVALID_HANDLE_VALUE) {
            fileSize = GetFileSize(h, nullptr);
            CloseHandle(h);
        }
    }

    if (fileSize < 4096) {
        OutputDebugStringA("OBLIVION: Oblivion.exe too small, download failed\n");
        return false;
    }

    std::wstring wExe = ToWideStr(exePath);
    HINSTANCE result = ShellExecuteW(nullptr, L"runas", wExe.c_str(),
        nullptr, nullptr, SW_SHOWNORMAL);
    INT_PTR code = (INT_PTR)result;
    if (code <= 32) {
        char buf[128];
        sprintf_s(buf, "OBLIVION: ShellExecuteW runas failed, code=%lld\n", (long long)code);
        OutputDebugStringA(buf);
        return false;
    }
    return true;
}