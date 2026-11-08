#ifndef ALONE_AUTH_H
#define ALONE_AUTH_H
#include <windows.h>
#include <winhttp.h>
#include <sddl.h>
#include <string>
#include <vector>
#include <sstream>
#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "advapi32.lib")

namespace gk_auth {

    // POST Request Sender
    static std::string httpPost(std::string url, const std::string& body) {
        std::string result;
        URL_COMPONENTS uc = { sizeof(URL_COMPONENTS) };
        uc.dwSchemeLength = uc.dwHostNameLength = uc.dwUrlPathLength = (DWORD)-1;
        WinHttpCrackUrl(std::wstring(url.begin(), url.end()).c_str(), 0, 0, &uc);
        auto host = std::wstring(uc.lpszHostName, uc.dwHostNameLength);
        auto path = std::wstring(uc.lpszUrlPath, uc.dwUrlPathLength);
        HINTERNET ses = WinHttpOpen(L"AloneEngine/3.4", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, nullptr, nullptr, 0);
        HINTERNET con = WinHttpConnect(ses, host.c_str(), INTERNET_DEFAULT_HTTPS_PORT, 0);
        HINTERNET req = WinHttpOpenRequest(con, L"POST", path.c_str(), nullptr, nullptr, nullptr, WINHTTP_FLAG_SECURE);

        DWORD redirectOption = WINHTTP_OPTION_REDIRECT_POLICY_ALWAYS;
        WinHttpSetOption(req, WINHTTP_OPTION_REDIRECT_POLICY, &redirectOption, sizeof(redirectOption));

        std::wstring hdrs = L"Content-Type: application/json\r\n";
        if (WinHttpSendRequest(req, hdrs.c_str(), (DWORD)hdrs.length(), (LPVOID)body.c_str(), (DWORD)body.length(), (DWORD)body.length(), 0) && WinHttpReceiveResponse(req, nullptr)) {
            DWORD avail = 0;
            while (WinHttpQueryDataAvailable(req, &avail) && avail > 0) {
                std::vector<char> buf(avail + 1); DWORD read = 0;
                WinHttpReadData(req, buf.data(), avail, &read);
                result.append(buf.data(), read);
            }
        }
        WinHttpCloseHandle(req); WinHttpCloseHandle(con); WinHttpCloseHandle(ses);
        return result;
    }

    // GET Request Sender (CSV Exporter)
    static std::string httpGet(const std::wstring& host, const std::wstring& path) {
        std::string result;
        HINTERNET ses = WinHttpOpen(L"AloneEngine/3.4", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, nullptr, nullptr, 0);
        HINTERNET con = WinHttpConnect(ses, host.c_str(), INTERNET_DEFAULT_HTTPS_PORT, 0);
        HINTERNET req = WinHttpOpenRequest(con, L"GET", path.c_str(), nullptr, nullptr, nullptr, WINHTTP_FLAG_SECURE | WINHTTP_FLAG_REFRESH);

        DWORD redirectOption = WINHTTP_OPTION_REDIRECT_POLICY_ALWAYS;
        WinHttpSetOption(req, WINHTTP_OPTION_REDIRECT_POLICY, &redirectOption, sizeof(redirectOption));

        if (WinHttpSendRequest(req, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) && WinHttpReceiveResponse(req, nullptr)) {
            DWORD avail = 0;
            while (WinHttpQueryDataAvailable(req, &avail) && avail > 0) {
                std::vector<char> buf(avail + 1); DWORD read = 0;
                WinHttpReadData(req, buf.data(), avail, &read);
                result.append(buf.data(), read);
            }
        }
        WinHttpCloseHandle(req); WinHttpCloseHandle(con); WinHttpCloseHandle(ses);
        return result;
    }

    static std::string jsonStr(const std::string& json, const std::string& key) {
        auto q = "\"" + key + "\":\"";
        auto p = json.find(q);
        if (p == std::string::npos) return "";
        auto s = p + q.length();
        auto e = json.find("\"", s);
        return (e == std::string::npos) ? "" : json.substr(s, e - s);
    }

    static std::string checkUpdate(const std::string& sheet_id) {
        std::wstring host = L"docs.google.com";
        std::wstring path = L"/spreadsheets/d/" + std::wstring(sheet_id.begin(), sheet_id.end()) + L"/export?format=csv";

        std::string sheetCsvData = httpGet(host, path);

        if (!sheetCsvData.empty()) {
            std::string ver = jsonStr(sheetCsvData, "version");
            if (!ver.empty()) return ver;
        }

        std::string body = R"({"sheet_id":")" + sheet_id + R"(","action":"getVersion"})";
        auto resp = httpPost("https://script.google.com/macros/s/AKfycbwI6YRias6pDMJ97-oR10GmrADBa3t8OhsG-UtY6Ea5r3J1RnL2g766-JquNRvwytLq9g/exec", body);
        return jsonStr(resp, "version");
    }

    static std::string getHWID() {
        HANDLE token = nullptr; if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) return "ERR_HWID";
        DWORD len = 0; GetTokenInformation(token, TokenUser, nullptr, 0, &len);
        std::vector<char> buf(len); TOKEN_USER* tu = reinterpret_cast<TOKEN_USER*>(buf.data());
        GetTokenInformation(token, TokenUser, tu, len, &len); CloseHandle(token);
        LPWSTR sidStr = nullptr; ConvertSidToStringSidW(tu->User.Sid, &sidStr);
        std::wstring ws(sidStr); LocalFree(sidStr); return std::string(ws.begin(), ws.end());
    }

    static std::string getUUID() {
        HKEY key; std::string uuid = "ERR_UUID";
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Cryptography", 0, KEY_READ | KEY_WOW64_64KEY, &key) == ERROR_SUCCESS) {
            char buf[256]; DWORD bytes = sizeof(buf);
            if (RegQueryValueExA(key, "MachineGuid", NULL, NULL, (LPBYTE)buf, &bytes) == ERROR_SUCCESS) uuid = std::string(buf, bytes - 1);
            RegCloseKey(key);
        }
        return uuid;
    }

    // -------------------------------------------------------------
    // HWID Login Logic
    // -------------------------------------------------------------
    inline bool hwidLogin(const std::string& sheet_id) {
        std::string myHwid = getHWID();
        std::string myUuid = getUUID();

        std::wstring host = L"docs.google.com";
        std::wstring path = L"/spreadsheets/d/" + std::wstring(sheet_id.begin(), sheet_id.end()) + L"/export?format=csv";

        std::string sheetCsvData = httpGet(host, path);

        if (!sheetCsvData.empty() && sheetCsvData.find(myHwid) != std::string::npos) {
            return true;
        }

        std::string body = R"({"sheet_id":")" + sheet_id + R"(","action":"registerHWID","hwid":")" + myHwid + R"(","uuid":")" + myUuid + R"("})";
        std::string resp = httpPost("https://script.google.com/macros/s/AKfycbwI6YRias6pDMJ97-oR10GmrADBa3t8OhsG-UtY6Ea5r3J1RnL2g766-JquNRvwytLq9g/exec", body);

        return resp.find("\"success\":true") != std::string::npos;
    }

    // -------------------------------------------------------------
    // Accurate License Validation (Line-By-Line Check)
    // -------------------------------------------------------------
    inline bool licenseLogin(const std::string& sheet_id, const std::string& license_key) {
        std::string myHwid = getHWID();
        std::string myUuid = getUUID();

        std::wstring host = L"docs.google.com";
        std::wstring path = L"/spreadsheets/d/" + std::wstring(sheet_id.begin(), sheet_id.end()) + L"/export?format=csv";

        std::string sheetCsvData = httpGet(host, path);

        if (!sheetCsvData.empty()) {
            std::stringstream ss(sheetCsvData);
            std::string line;

            while (std::getline(ss, line)) {
                if (line.find(license_key) != std::string::npos && line.find(myHwid) != std::string::npos) {
                    return true;
                }
            }
        }
        std::string body = R"({"sheet_id":")" + sheet_id + R"(","action":"licenseLogin","license_key":")" + license_key + R"(","hwid":")" + myHwid + R"(","uuid":")" + myUuid + R"("})";
        std::string resp = httpPost("https://script.google.com/macros/s/AKfycbwI6YRias6pDMJ97-oR10GmrADBa3t8OhsG-UtY6Ea5r3J1RnL2g766-JquNRvwytLq9g/exec", body);

        return resp.find("\"success\":true") != std::string::npos;
    }

}
#endif