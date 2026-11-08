#pragma once
#include <Windows.h>
#include <vector>
#include <TlHelp32.h>
#include <winternl.h>
#include <mutex>
#include <future>
#include <chrono>
#include <atlbase.h>
#include <random>
#define WIN32_LEAN_AND_MEAN
#pragma comment(lib, "ntdll.lib")

#include <windows.h>
#include <sddl.h>
#include <winhttp.h>
#include <iostream>
#include <string>
#include <random>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>
#include <codecvt>
#pragma comment(lib, "winhttp.lib")
#include <sddl.h>
#include <winhttp.h>
#include <iostream>
#include <string>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>
#include <codecvt>
#include <windows.h>
#include <winhttp.h>
#include <string>
#include "imgui_internal.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#include <tchar.h>
#include <dwmapi.h>

#include <iostream>
#include <Windows.h>
#include <string>
#include <unordered_set>

std::vector<uintptr_t> storedCamera;
std::vector<uintptr_t> storedSpeed;
std::vector<uintptr_t> storedLanding;
std::vector<uintptr_t> storedSniper;

int selection = 0;
int memMode = 0;
int sliderSniper = 39;

std::vector<DWORD_PTR> resultForAll;
std::mutex resultMutex;

int dragKey, speedKey, landingKey, cameraKey, sniperKey;

namespace var
{
    bool sniper, chams_menu, streamer_esp, streamer_proof, _aimbot, delay, sniperAim, glitch, norecoil, reload, changer, steam, quick, bypassLoading, bypassLobby, recoil, wall, body, sniperLoc, ammoLoc, scope, female, speed, bLoading, bLobby, camera, landing, guest, fastMem, func = false;
}

namespace stats
{
    int sniper, chams_menu, streamer_esp, streamer_proof, _aimbot, delay, sniperAim, glitch, norecoil, reload, changer, steam, quick, bypassLoading, bypassLobby, recoil, wall, body, sniperLoc, ammoLoc, scope, female, speed, bLoading, bLobby, camera, landing, guest, fastMem, func = 0;
}


// ---------------------- 3-SECOND COMPACT TOP-LEFT NOTIFICATION SYSTEM ----------------------
enum NotificationType { Notify_Success, Notify_Warning, Notify_Error };

struct Notification {
    std::string message;
    NotificationType type;
    float timer;
    float max_time = 3.0f;
};

static std::vector<Notification> notifications;

void AddNotification(const std::string& msg, NotificationType type) {
    notifications.push_back({ msg, type, 3.0f });
}

uintptr_t SAddress = 0x0L;
#ifdef _WIN64
uintptr_t EAddress = 0x00007fffffffffff;
#else
uintptr_t EAddress = 0x7fffffff;
#endif

extern "C" NTSTATUS NTAPI ZwReadVirtualMemory(HANDLE ProcessHandle, PVOID BaseAddress, PVOID Buffer, SIZE_T BufferSize, PSIZE_T NumberOfBytesRead);
bool fastMem = false;

DWORD ProcessId = 0;
HANDLE ProcessHandle = nullptr;
std::mutex mtx;

struct MEMORY_REGION {
    uintptr_t dwBaseAddr;
    uintptr_t dwMemorySize;

    MEMORY_REGION(uintptr_t base = 0, uintptr_t size = 0)
        : dwBaseAddr(base), dwMemorySize(size) {}
};

bool Attach(const char* procname = nullptr) {
    const char* emulators[] = { "HD-PIayer.exe", "HD-Player.exe", "MEmuHeadless.exe" };

    if (!procname) {
        for (const char* emulator : emulators) {
            if (Attach(emulator)) {
                return true;
            }
        }
        return false;
    }

    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    PROCESSENTRY32 pe = { sizeof(PROCESSENTRY32) };
    if (Process32First(hSnap, &pe)) {
        do {
            if (_tcsicmp(pe.szExeFile, procname) == 0) {
                ProcessId = pe.th32ProcessID;
                ProcessHandle = OpenProcess(PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION, TRUE, ProcessId);
                CloseHandle(hSnap);
                return ProcessHandle != nullptr;
            }
        } while (Process32Next(hSnap, &pe));
    }
    CloseHandle(hSnap);
    return false;
}

template<int N>
struct XorStr {
    char buf[N];
    constexpr XorStr(const char(&s)[N], char k) : buf{} {
        for (int i = 0; i < N; ++i) buf[i] = s[i] ^ k;
    }
    const char* decrypt(char k) const {
        static char out[N];
        for (int i = 0; i < N; ++i) out[i] = buf[i] ^ k;
        return out;
    }
};
#define XS(s, k) (XorStr<sizeof(s)>(s, k).decrypt(k))

namespace NtResolve {
    static constexpr DWORD djb2_ct(const char* s) noexcept {
        DWORD h = 5381;
        while (*s) h = ((h << 5) + h) + (DWORD)*s++;
        return h;
    }

    inline FARPROC resolve_by_hash(HMODULE hMod, DWORD targetHash) {
        auto base = (BYTE*)hMod;
        auto dos = (PIMAGE_DOS_HEADER)base;
        auto nt = (PIMAGE_NT_HEADERS)(base + dos->e_lfanew);
        auto& exp = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
        if (!exp.VirtualAddress) return nullptr;
        auto dir = (PIMAGE_EXPORT_DIRECTORY)(base + exp.VirtualAddress);
        auto names = (DWORD*)(base + dir->AddressOfNames);
        auto funcs = (DWORD*)(base + dir->AddressOfFunctions);
        auto ords = (WORD*)(base + dir->AddressOfNameOrdinals);
        for (DWORD i = 0; i < dir->NumberOfNames; ++i) {
            const char* name = (const char*)(base + names[i]);
            DWORD rth = 5381;
            for (const char* p = name; *p; ++p) rth = ((rth << 5) + rth) + (DWORD)*p;
            if (rth == targetHash)
                return (FARPROC)(base + funcs[ords[i]]);
        }
        return nullptr;
    }
}

namespace HandleSteal {
    inline HANDLE steal_process_handle(DWORD targetPid, DWORD desiredAccess) {
        typedef NTSTATUS(NTAPI* pNtQuerySystemInformation)(ULONG, PVOID, ULONG, PULONG);
        typedef NTSTATUS(NTAPI* pNtDuplicateObject)(HANDLE, HANDLE, HANDLE, PHANDLE, ACCESS_MASK, ULONG, ULONG);

        HMODULE hNt = GetModuleHandleA("ntdll.dll");
        auto _NtQSI = (pNtQuerySystemInformation)GetProcAddress(hNt, "NtQuerySystemInformation");
        auto _NtDO = (pNtDuplicateObject)GetProcAddress(hNt, "NtDuplicateObject");
        if (!_NtQSI || !_NtDO) return nullptr;

        const ULONG SystemHandleInformation = 16;
        ULONG bufLen = 0x10000;
        PVOID buf = VirtualAlloc(NULL, bufLen, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

        while (_NtQSI(SystemHandleInformation, buf, bufLen, &bufLen) == (NTSTATUS)0xC0000004L) {
            VirtualFree(buf, 0, MEM_RELEASE);
            bufLen *= 2;
            buf = VirtualAlloc(NULL, bufLen, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        }

        struct SYSTEM_HANDLE_ENTRY {
            ULONG OwnerPid;
            BYTE ObjectType;
            BYTE HandleFlags;
            USHORT HandleValue;
            PVOID ObjectPointer;
            ACCESS_MASK AccessMask;
        };

        struct SYSTEM_HANDLE_INFO {
            ULONG Count;
            SYSTEM_HANDLE_ENTRY Handles[1];
        };

        auto* handleInfo = (SYSTEM_HANDLE_INFO*)buf;
        HANDLE stolenHandle = nullptr;

        for (ULONG i = 0; i < handleInfo->Count; ++i) {
            const auto& entry = handleInfo->Handles[i];
            if (entry.OwnerPid == GetCurrentProcessId()) continue;

            HANDLE hOwner = OpenProcess(PROCESS_DUP_HANDLE, FALSE, entry.OwnerPid);
            if (!hOwner) continue;

            HANDLE hDup = nullptr;
            NTSTATUS status = _NtDO(hOwner, (HANDLE)(ULONG_PTR)entry.HandleValue, GetCurrentProcess(), &hDup, 0, FALSE, 2);
            CloseHandle(hOwner);

            if (status == 0 && hDup) {
                if (GetProcessId(hDup) == targetPid) {
                    HANDLE hFinal = nullptr;
                    DuplicateHandle(GetCurrentProcess(), hDup, GetCurrentProcess(), &hFinal, desiredAccess, FALSE, 0);
                    CloseHandle(hDup);
                    stolenHandle = hFinal;
                    break;
                }
                CloseHandle(hDup);
            }
        }

        VirtualFree(buf, 0, MEM_RELEASE);
        if (!stolenHandle) {
            stolenHandle = OpenProcess(desiredAccess, FALSE, targetPid);
        }
        return stolenHandle;
    }
}

inline HMODULE get_ntdll() {
    static HMODULE h = nullptr;
    if (!h) h = GetModuleHandleA(XS("ntdll.dll", 0x11));
    return h;
}

int MemfindNew(BYTE* buffer, SIZE_T bufSize, BYTE* pat, SIZE_T patLen) {
    if (bufSize < patLen) return -1;
    SIZE_T anchor = 0;
    while (anchor < patLen && pat[anchor] == '?') ++anchor;
    if (anchor >= patLen) return 0;

    BYTE firstByte = pat[anchor];
    __m128i needle = _mm_set1_epi8((char)firstByte);
    SIZE_T limit = bufSize - patLen;

    SIZE_T i = 0;
    for (; i + 15 + anchor <= limit; i += 16) {
        __m128i chunk = _mm_loadu_si128((__m128i*)(buffer + i + anchor));
        int mask = _mm_movemask_epi8(_mm_cmpeq_epi8(chunk, needle));
        while (mask) {
            unsigned long bit;
            _BitScanForward(&bit, mask);
            SIZE_T pos = i + bit;
            if (pos <= limit) {
                SIZE_T j;
                for (j = 0; j < patLen; ++j)
                    if (pat[j] != '?' && buffer[pos + j] != pat[j]) break;
                if (j == patLen) return (int)pos;
            }
            mask &= mask - 1;
        }
    }
    for (; i <= limit; ++i) {
        SIZE_T j;
        for (j = 0; j < patLen; ++j)
            if (pat[j] != '?' && buffer[i + j] != pat[j]) break;
        if (j == patLen) return (int)i;
    }
    return -1;
}

namespace CachedApi {
    typedef NTSTATUS(NTAPI* pNtQueryVirtualMemory)(HANDLE, PVOID, int, PVOID, SIZE_T, PSIZE_T);
    typedef NTSTATUS(NTAPI* pNtReadVirtualMemory)(HANDLE, PVOID, PVOID, SIZE_T, PSIZE_T);
    typedef NTSTATUS(NTAPI* pNtWriteVirtualMemory)(HANDLE, PVOID, PVOID, SIZE_T, PSIZE_T);

    inline pNtQueryVirtualMemory getNtQVM() {
        static auto fn = (pNtQueryVirtualMemory)NtResolve::resolve_by_hash(get_ntdll(), NtResolve::djb2_ct("NtQueryVirtualMemory"));
        return fn;
    }
    inline pNtReadVirtualMemory getNtRVM() {
        static auto fn = (pNtReadVirtualMemory)NtResolve::resolve_by_hash(get_ntdll(), NtResolve::djb2_ct("NtReadVirtualMemory"));
        return fn;
    }
    inline pNtWriteVirtualMemory getNtWVM() {
        static auto fn = (pNtWriteVirtualMemory)NtResolve::resolve_by_hash(get_ntdll(), NtResolve::djb2_ct("NtWriteVirtualMemory"));
        return fn;
    }
}

DWORD_PTR GetProcessId(const char* procname) {
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap == INVALID_HANDLE_VALUE) return 0;

    PROCESSENTRY32W pe = { sizeof(pe) };
    std::wstring targetW(procname, procname + strlen(procname));
    if (Process32FirstW(hSnap, &pe)) {
        do {
            if (_wcsicmp(pe.szExeFile, targetW.c_str()) == 0) {
                CloseHandle(hSnap);
                return pe.th32ProcessID;
            }
        } while (Process32NextW(hSnap, &pe));
    }
    CloseHandle(hSnap);
    return 0;
}

bool FindPatternNew(const char* procname, DWORD_PTR StartRange, DWORD_PTR EndRange, BYTE* SearchBytes, SIZE_T SearchSize, std::vector<DWORD_PTR>& AddressRet) {
    DWORD_PTR pid = GetProcessId(procname);
    if (pid == 0) return false;

    auto _NtQVM = CachedApi::getNtQVM();
    auto _NtRVM = CachedApi::getNtRVM();
    if (!_NtQVM || !_NtRVM) return false;

    HANDLE hProcess = HandleSteal::steal_process_handle((DWORD)pid, PROCESS_QUERY_INFORMATION | PROCESS_VM_READ);
    if (!hProcess) return false;

    std::vector<MEMORY_REGION> regions;
    MEMORY_BASIC_INFORMATION mbi;

    for (DWORD_PTR addr = StartRange; addr < EndRange; addr += mbi.RegionSize) {
        SIZE_T retLen = 0;
        NTSTATUS status = _NtQVM(hProcess, (PVOID)addr, 0, &mbi, sizeof(mbi), &retLen);
        if (status != 0) {
            mbi.RegionSize = 0x1000;
            continue;
        }
        if (mbi.State == MEM_COMMIT && !(mbi.Protect & PAGE_NOACCESS) && !(mbi.Protect & PAGE_GUARD) && (mbi.Protect & (PAGE_READONLY | PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE))) {
            regions.push_back(MEMORY_REGION((DWORD_PTR)mbi.BaseAddress, mbi.RegionSize));
        }
    }

    if (regions.empty()) { CloseHandle(hProcess); return true; }

    std::sort(regions.begin(), regions.end(), [](const MEMORY_REGION& a, const MEMORY_REGION& b) {
        return a.dwBaseAddr < b.dwBaseAddr;
        });
    std::vector<MEMORY_REGION> merged;
    merged.push_back(regions[0]);
    for (size_t i = 1; i < regions.size(); ++i) {
        auto& last = merged.back();
        if (regions[i].dwBaseAddr == last.dwBaseAddr + last.dwMemorySize) {
            last.dwMemorySize += regions[i].dwMemorySize;
        }
        else {
            merged.push_back(regions[i]);
        }
    }
    regions.clear(); regions.shrink_to_fit();

    SIZE_T maxSize = 0;
    for (const auto& r : merged)
        if (r.dwMemorySize > maxSize) maxSize = r.dwMemorySize;

    const SIZE_T MAX_BUF = 16 * 1024 * 1024;
    if (maxSize > MAX_BUF) maxSize = MAX_BUF;

    unsigned numThreads = std::thread::hardware_concurrency();
    if (numThreads < 1) numThreads = 1;
    if (numThreads > 4) numThreads = 4;
    if (numThreads > (unsigned)merged.size()) numThreads = (unsigned)merged.size();

    std::vector<std::vector<DWORD_PTR>> threadResults(numThreads);
    std::vector<std::thread> threads;

    auto scanWorker = [&](unsigned tid, unsigned tcount) {
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_LOWEST);

        SIZE_T localBufSize = maxSize;
        BYTE* buffer = (BYTE*)VirtualAlloc(NULL, localBufSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if (!buffer) return;

        for (unsigned idx = tid; idx < (unsigned)merged.size(); idx += tcount) {
            const auto& region = merged[idx];
            SIZE_T remaining = region.dwMemorySize;
            DWORD_PTR baseAddr = region.dwBaseAddr;

            while (remaining > 0) {
                SIZE_T chunkSize = (remaining > localBufSize) ? localBufSize : remaining;
                SIZE_T bytesRead = 0;
                NTSTATUS status = _NtRVM(hProcess, (PVOID)baseAddr, buffer, chunkSize, &bytesRead);

                if (status == 0 && bytesRead > 0) {
                    DWORD_PTR offset = 0;
                    int found = MemfindNew(buffer, bytesRead, SearchBytes, SearchSize);
                    while (found != -1) {
                        offset += found;
                        threadResults[tid].push_back(baseAddr + offset);
                        offset += SearchSize;
                        if (offset >= bytesRead) break;
                        found = MemfindNew(buffer + offset, bytesRead - offset, SearchBytes, SearchSize);
                    }
                }

                baseAddr += chunkSize;
                remaining -= chunkSize;
                Sleep(1);
            }
        }

        VirtualFree(buffer, 0, MEM_RELEASE);
        };

    if (numThreads == 1) {
        scanWorker(0, 1);
    }
    else {
        for (unsigned t = 0; t < numThreads; ++t)
            threads.emplace_back(scanWorker, t, numThreads);
        for (auto& t : threads) t.join();
    }

    for (const auto& tr : threadResults)
        AddressRet.insert(AddressRet.end(), tr.begin(), tr.end());

    CloseHandle(hProcess);
    return true;
}

HANDLE AttachLocal(const char* procname = nullptr) {
    const char* emulators[] = { "HD-Player.exe" };
    if (!procname) {
        for (const char* emu : emulators) {
            HANDLE h = AttachLocal(emu);
            if (h) return h;
        }
        return nullptr;
    }
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    PROCESSENTRY32 pe = { sizeof(pe) };
    for (bool found = Process32First(hSnap, &pe); found; found = Process32Next(hSnap, &pe)) {
        if (_tcsicmp(pe.szExeFile, procname) == 0) {
            HANDLE h = HandleSteal::steal_process_handle(pe.th32ProcessID, PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION);
            CloseHandle(hSnap);
            return h;
        }
    }
    CloseHandle(hSnap);
    return nullptr;
}

struct Candidate {
    DWORD_PTR address;
    std::vector<BYTE> initialData;
    std::vector<BYTE> lastData;
    ULONGLONG lastChangeTime;
    bool processed;
};
std::vector<Candidate> candidates;
SIZE_T savedSearchSize = 0;

void FindPatternDouble(const char* procname, DWORD_PTR StartRange, DWORD_PTR EndRange, BYTE* SearchBytes, SIZE_T SearchSize) {
    std::vector<DWORD_PTR> firstResults;
    FindPatternNew(procname, StartRange, EndRange, SearchBytes, SearchSize, firstResults);
    if (firstResults.empty()) return;

    HANDLE hLocal = AttachLocal(procname);
    if (!hLocal) return;
    auto _NtRVM = CachedApi::getNtRVM();
    if (!_NtRVM) { CloseHandle(hLocal); return; }

    candidates.clear();
    savedSearchSize = SearchSize;

    ULONGLONG now = GetTickCount64();
    for (DWORD_PTR addr : firstResults) {
        std::vector<BYTE> buf(SearchSize);
        if (_NtRVM(hLocal, (PVOID)addr, buf.data(), SearchSize, nullptr) == 0) {
            candidates.push_back({ addr, buf, buf, now, false });
        }
    }
    CloseHandle(hLocal);
}

std::atomic<bool> bFilterRunning{ false };

void FilterThreadWorker() {
    HANDLE hFilter = nullptr;
    auto _NtRVM = CachedApi::getNtRVM();
    auto _NtWVM = CachedApi::getNtWVM();

    while (bFilterRunning) {
        if (candidates.empty() || !_NtRVM || !_NtWVM) {
            Sleep(100);
            continue;
        }

        if (!hFilter) {
            hFilter = AttachLocal();
            if (!hFilter) {
                Sleep(1000);
                continue;
            }
        }

        std::vector<DWORD_PTR> tempResult;
        ULONGLONG currentTime = GetTickCount64();

        for (auto it = candidates.begin(); it != candidates.end(); ) {
            std::vector<BYTE> secondBuf(savedSearchSize);

            if (_NtRVM(hFilter, (PVOID)it->address, secondBuf.data(), savedSearchSize, nullptr) == 0) {

                if (!std::equal(it->lastData.begin(), it->lastData.end(), secondBuf.begin())) {
                    it->lastData = secondBuf;
                    it->lastChangeTime = currentTime;
                }

                if (!std::equal(it->initialData.begin(), it->initialData.end(), secondBuf.begin())) {

                    if (currentTime - it->lastChangeTime <= 50000) {
                        tempResult.push_back(it->address);

                        if (!it->processed) {
                            DWORD buffer = 0;

                            DWORD_PTR read = 0;
                            DWORD_PTR write = 182L;

                            if (selection == 0) { read = 234L; }
                            else if (selection == 1) { read = 246L; }
                            else if (selection == 2) { read = 186L; }

                            if (selection == 0 || selection == 1 || selection == 2) {
                                if (_NtRVM(hFilter, (PVOID)(it->address + read), &buffer, sizeof(DWORD), nullptr) == 0) {
                                    _NtWVM(hFilter, (PVOID)(it->address + write), &buffer, sizeof(DWORD), nullptr);
                                }
                            }
                            it->processed = true;
                        }
                    }
                    else {
                        it->processed = false;
                    }
                }
                ++it;
            }
            else {
                it = candidates.erase(it);
            }
        }

        {
            std::lock_guard<std::mutex> lock(resultMutex);
            resultForAll = tempResult;
        }

        Sleep(10);
    }
    if (hFilter) CloseHandle(hFilter);
}

void StartFilterThread() {
    if (!bFilterRunning) {
        bFilterRunning = true;
        std::thread(FilterThreadWorker).detach();
    }
}

int Memfind(BYTE* buffer, uintptr_t dwBufferSize, BYTE* bstr, uintptr_t dwStrLen) {
    if (dwBufferSize < 0) return -1;
    for (uintptr_t i = 0; i < dwBufferSize; i++) {
        uintptr_t j;
        for (j = 0; j < dwStrLen; j++) {
            if (buffer[i + j] != bstr[j] && bstr[j] != '?') break;
        }
        if (j == dwStrLen) return i;
    }
    return -1;
}





std::vector<DWORD> FindProcessesByName(const wchar_t* processName) {
    std::vector<DWORD> pids;
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap == INVALID_HANDLE_VALUE) return pids;

    PROCESSENTRY32W pe = { sizeof(pe) };
    if (Process32FirstW(hSnap, &pe)) {
        do {
            if (_wcsicmp(pe.szExeFile, processName) == 0) {
                pids.push_back(pe.th32ProcessID);
            }
        } while (Process32NextW(hSnap, &pe));
    }
    CloseHandle(hSnap);
    return pids;
}

bool SetTargetPriority(DWORD pid, DWORD priorityClass) {
    HANDLE hProcess = OpenProcess(PROCESS_SET_INFORMATION, FALSE, pid);
    if (!hProcess) return false;
    BOOL result = SetPriorityClass(hProcess, priorityClass);
    CloseHandle(hProcess);
    return result != 0;
}

void BoostScanner() {
    SetPriorityClass(GetCurrentProcess(), ABOVE_NORMAL_PRIORITY_CLASS);
}

void NormalizeScanner() {
    SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);
}

void NoLag() {
    try {
        auto pids = FindProcessesByName(L"HD-Player.exe");
        if (pids.empty()) return;
        for (DWORD pid : pids) {
            SetTargetPriority(pid, NORMAL_PRIORITY_CLASS);
        }
        BoostScanner();
    }
    catch (...) {}
}

void Lag() {
    try {
        NormalizeScanner();
    }
    catch (...) {}
}






template<typename T>
bool FindPattern(uintptr_t StartRange, uintptr_t EndRange, BYTE* SearchBytes, std::vector<T>& AddressRet) {

    if (!Attach()) return false;

    BYTE* pCurrMemoryData = nullptr;
    MEMORY_BASIC_INFORMATION mbi;
    std::vector<MEMORY_REGION> m_vMemoryRegion;
    uintptr_t nSearchSize = _msize(SearchBytes);

    for (uintptr_t addr = StartRange; addr < EndRange; addr += mbi.RegionSize) {
        if (!VirtualQueryEx(ProcessHandle, (LPCVOID)addr, &mbi, sizeof(mbi)) || mbi.State != MEM_COMMIT || mbi.Protect == PAGE_NOACCESS) continue;

        MEMORY_REGION mData = { (uintptr_t)mbi.BaseAddress, mbi.RegionSize };
        m_vMemoryRegion.push_back(mData);
    }

    for (const auto& mData : m_vMemoryRegion) {
        pCurrMemoryData = new BYTE[mData.dwMemorySize];
        SIZE_T bytesRead = 0;
        if (ZwReadVirtualMemory(ProcessHandle, (LPVOID)mData.dwBaseAddr, pCurrMemoryData, mData.dwMemorySize, &bytesRead) >= 0) {
            uintptr_t dwOffset = 0;
            int iOffset = Memfind(pCurrMemoryData, bytesRead, SearchBytes, nSearchSize);
            while (iOffset != -1) {
                dwOffset += iOffset;
                AddressRet.push_back(mData.dwBaseAddr + dwOffset);
                dwOffset += nSearchSize;
                iOffset = Memfind(pCurrMemoryData + dwOffset, bytesRead - dwOffset - nSearchSize, SearchBytes, nSearchSize);
            }
        }
        delete[] pCurrMemoryData;
    }

    Beep(300, 300);
    return true;
}

template<typename T>
bool FindPatternFast(uintptr_t StartRange, uintptr_t EndRange, BYTE* SearchBytes, std::vector<T>& AddressRet) {
    if (!Attach()) return false;

    MEMORY_BASIC_INFORMATION mbi;
    std::vector<MEMORY_REGION> m_vMemoryRegion;
    uintptr_t nSearchSize = _msize(SearchBytes);

    for (uintptr_t addr = StartRange; addr < EndRange; addr += mbi.RegionSize) {
        if (!VirtualQueryEx(ProcessHandle, (LPCVOID)addr, &mbi, sizeof(mbi)) || mbi.State != MEM_COMMIT || mbi.Protect == PAGE_NOACCESS) continue;

        MEMORY_REGION mData = { (uintptr_t)mbi.BaseAddress, mbi.RegionSize };
        m_vMemoryRegion.push_back(mData);
    }

    std::vector<std::future<void>> futures;
    auto start_time = std::chrono::high_resolution_clock::now();

    for (const auto& mData : m_vMemoryRegion) {
        futures.push_back(std::async(std::launch::async, [&, mData]() {
            BYTE* pCurrMemoryData = new BYTE[mData.dwMemorySize];
            SIZE_T bytesRead = 0;
            if (ZwReadVirtualMemory(ProcessHandle, (LPVOID)mData.dwBaseAddr, pCurrMemoryData, mData.dwMemorySize, &bytesRead) >= 0) {
                uintptr_t dwOffset = 0;
                int iOffset = Memfind(pCurrMemoryData, bytesRead, SearchBytes, nSearchSize);
                while (iOffset != -1) {
                    dwOffset += iOffset;
                    std::lock_guard<std::mutex> lock(mtx);
                    AddressRet.push_back(mData.dwBaseAddr + dwOffset);
                    dwOffset += nSearchSize;
                    iOffset = Memfind(pCurrMemoryData + dwOffset, bytesRead - dwOffset - nSearchSize, SearchBytes, nSearchSize);
                }
            }
            delete[] pCurrMemoryData;
            }));

        auto elapsed = std::chrono::high_resolution_clock::now() - start_time;
        if (elapsed >= std::chrono::seconds(3)) break;
    }

    for (auto& future : futures) {
        future.wait_for(std::chrono::milliseconds(10));
    }
    Beep(300, 300);
    return true;
}

template<typename T>
bool FindPatternSelector(uintptr_t StartRange, uintptr_t EndRange, BYTE* SearchBytes, std::vector<T>& AddressRet) {
    if (fastMem) {
        if (!Attach()) return false;
        return FindPatternFast(StartRange, EndRange, SearchBytes, AddressRet);
    }
    else {
        if (!Attach()) return false;
        return FindPattern(StartRange, EndRange, SearchBytes, AddressRet);
    }
}

void EasyReplace(int& stats, std::string name, std::vector<BYTE> search, std::vector<BYTE> replace, uintptr_t bytes_to_replace, uintptr_t read, uintptr_t write, uintptr_t size_of)
{
    stats = 1;
    std::string apply = name + " modifying...";
    std::vector<DWORD_PTR> result;

    result.clear();
    FindPatternSelector(SAddress, EAddress, search.data(), result);

    if (!result.empty())
    {
        for (DWORD_PTR addr : result)
        {
            DWORD_PTR buffer;
            DWORD_PTR offsetOriginal = addr;

            WriteProcessMemory(ProcessHandle, (LPVOID)(offsetOriginal + bytes_to_replace), replace.data(), replace.size(), 0);
            ReadProcessMemory(ProcessHandle, (LPCVOID)(offsetOriginal + read), &buffer, sizeof(DWORD_PTR), 0);
            WriteProcessMemory(ProcessHandle, (LPVOID)(offsetOriginal + write), &buffer, sizeof(DWORD_PTR), 0);
        }

        stats = 0;
        std::string suc = name + " successfully modified";
        AddNotification(suc, Notify_Success);
    }
    else
    {
        stats = 2;
        std::string err = name + " failed to modified!";
        AddNotification(err, Notify_Error);
    }
}

void ByCode(int& stats, std::string name, std::vector<BYTE> search, std::vector<BYTE> replace)
{
    std::vector<uintptr_t> result;
    result.clear();

    stats = 1;
    std::string apply = name + " modifying...";


    FindPatternSelector(SAddress, EAddress, search.data(), result);
    if (result.empty()) {
        Beep(100, 100);
        stats = 2;
        std::string err = name + " failed to modified!";
        AddNotification(err, Notify_Error);
    }
    else {
        for (uintptr_t addr : result) {
            WriteProcessMemory(ProcessHandle, (LPVOID)addr, replace.data(), replace.size(), 0);
        }
        stats = 0;
        std::string suc = name + " successfully modified";
        AddNotification(suc, Notify_Success);
    }
}

void OneByte(int& stats, std::string name, std::vector<uintptr_t>& result, std::vector<BYTE> search, std::vector<BYTE> replace, uintptr_t bytes_to_replace, uintptr_t read, uintptr_t write)
{
    stats = 1;
    std::string apply = name + " modifying...";


    result.clear();
    FindPatternSelector(SAddress, EAddress, search.data(), result);

    if (!result.empty())
    {
        for (uintptr_t addr : result)
        {
            if (!(result.size() > 1)) {
                uintptr_t buffer;
                uintptr_t offsetOriginal = addr;

                WriteProcessMemory(ProcessHandle, (LPVOID)(offsetOriginal + bytes_to_replace), replace.data(), replace.size(), 0);
                if (read != 0 && write != 0) {
                    ReadProcessMemory(ProcessHandle, (LPCVOID)(offsetOriginal + read), &buffer, sizeof(buffer), 0);
                    WriteProcessMemory(ProcessHandle, (LPVOID)(offsetOriginal + write), &buffer, sizeof(buffer), 0);
                }
            }
        }

        stats = 0;
        std::string suc = name + " successfully modified";
        AddNotification(suc, Notify_Success);
    }
    else
    {
        stats = 2;
        std::string err = name + " failed to modified!";
        AddNotification(err, Notify_Error);
    }
}

void loopsOfaim() {
    HANDLE hAimProc = nullptr;
    while (true) {
        if (selection != 3) {
            Sleep(10);
            continue;
        }

        // No left mouse button pressed? Chill out, do nothing.
        if (!(GetAsyncKeyState(VK_LBUTTON) & 0x8000)) {
            Sleep(10);
            continue;
        }

        {
            std::lock_guard<std::mutex> lock(resultMutex);
            if (resultForAll.empty()) {
                Sleep(100);
                continue;
            }

            if (!hAimProc) hAimProc = AttachLocal("HD-Player.exe");

            int hitCount = 0;
            if (hAimProc) {
                bool opFailed = false;
                for (DWORD_PTR address : resultForAll) {
                    int buffer = 0;
                    DWORD_PTR readAddress = address + 0x100 - 2L;
                    DWORD_PTR writeAddress = address - 0x394 - 2L;

                    if (!ReadProcessMemory(hAimProc, (PVOID)readAddress, &buffer, sizeof(int), nullptr)) {
                        opFailed = true;
                        break;
                    }
                    if (!WriteProcessMemory(hAimProc, (PVOID)writeAddress, &buffer, sizeof(int), nullptr)) {
                        opFailed = true;
                        break;
                    }
                    hitCount++;
                }

                if (opFailed) {
                    CloseHandle(hAimProc);
                    hAimProc = nullptr;
                }
            }
        }
        Sleep(1);
    }
}

void loadAll()
{
    std::lock_guard<std::mutex> lk(resultMutex);
    resultForAll.clear();
    int stats = 1;
    std::string suc = "Loading data...";
    AddNotification(suc, Notify_Success);

    BYTE pattern[] = { 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, '?', '?', '?', '?', 0x00, 0x00, 0x00, 0x00, '?', '?', '?', '?', 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, '?', '?', 0xA5, 0x43, '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', 0x66, 0x66, 0x66, 0x3F, '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', 0xC0, 0x3F, '?', '?', '?', '?', 0x9A, 0x99, 0x99, 0x3F, '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', 0x00, 0x00, 0x00, '?', 0x00, 0x00, 0x00 }; // NOP NOP INT3
    SIZE_T patternSize = sizeof(pattern);
    FindPatternDouble("HD-Player.exe", SAddress, EAddress, pattern, patternSize);
    suc = "Successfully loaded data!";

    AddNotification(suc, Notify_Success);
    Beep(500, 500);
}

void femaleChanger(int& stats)
{
    {
        std::vector<uintptr_t> result;
        std::vector<BYTE> search = { 0x41, 0x23, 0x05, 0x06, 0x45, 0x23, 0x05, 0x06, 0x47, 0x23, 0x05, 0x06, 0x48, 0x23, 0x05, 0x06, 0x49, 0x23, 0x05, 0x06, 0x4A, 0x23, 0x05, 0x06, 0x4C, 0x23, 0x05, 0x06, 0x4D, 0x23, 0x05, 0x06, 0x4E, 0x23, 0x05, 0x06, 0x50, 0x23, 0x05, 0x06, 0x51, 0x23, 0x05, 0x06, 0x52, 0x23, 0x05, 0x06, 0x53, 0x23, 0x05, 0x06, 0x54, 0x23, 0x05, 0x06, 0x55, 0x23, 0x05, 0x06, 0x56, 0x23, 0x05, 0x06, 0x58, 0x23, 0x05, 0x06, 0x59, 0x23, 0x05, 0x06, 0x5A, 0x23, 0x05, 0x06, 0x5B, 0x23, 0x05, 0x06, 0x5C, 0x23, 0x05, 0x06, 0x84, 0x65, 0x14, 0x06, 0x86, 0x65, 0x14, 0x06, 0x87, 0x65, 0x14, 0x06, 0x88, 0x65, 0x14, 0x06, 0x89, 0x65, 0x14, 0x06, 0x8A, 0x65, 0x14, 0x06, 0x8B, 0x65, 0x14, 0x06, 0x8D, 0x65, 0x14, 0x06, 0x8E, 0x65, 0x14, 0x06, 0x91, 0x65, 0x14, 0x06, 0x92, 0x65, 0x14, 0x06, 0x93, 0x65, 0x14, 0x06, 0x95, 0x65, 0x14, 0x06, 0x96, 0x65, 0x14, 0x06, 0x97, 0x65, 0x14, 0x06, 0x98, 0x65, 0x14, 0x06, 0x99, 0x65, 0x14, 0x06, 0x9A, 0x65, 0x14, 0x06, 0x9C, 0x65, 0x14, 0x06, 0x9D, 0x65, 0x14, 0x06, 0x9E, 0x65, 0x14, 0x06, 0x9F, 0x65, 0x14, 0x06, 0xA0, 0x65, 0x14, 0x06, 0xA1, 0x65, 0x14, 0x06, 0xA2, 0x65, 0x14, 0x06, 0xA3, 0x65, 0x14, 0x06, 0xA4, 0x65, 0x14, 0x06, 0xA5, 0x65, 0x14, 0x06, 0xA6, 0x65, 0x14, 0x06, 0xA7, 0x65, 0x14, 0x06, 0xA8, 0x65, 0x14, 0x06, 0xC2 };
        std::vector<BYTE> replace = { 0x85, 0x65, 0x14, 0x06, 0x85, 0x65, 0x14, 0x06, 0x85, 0x65, 0x14, 0x06, 0x85, 0x65, 0x14, 0x06, 0x85, 0x65, 0x14, 0x06, 0x85, 0x65, 0x14, 0x06, 0x85, 0x65, 0x14, 0x06, 0x85, 0x65, 0x14, 0x06, 0x85, 0x65, 0x14, 0x06, 0x85, 0x65, 0x14, 0x06, 0x85, 0x65, 0x14, 0x06, 0x85, 0x65, 0x14, 0x06, 0x85, 0x65, 0x14, 0x06, 0x85, 0x65, 0x14, 0x06, 0x85, 0x65, 0x14, 0x06, 0x85, 0x65, 0x14, 0x06, 0x85, 0x65, 0x14, 0x06, 0x85, 0x65, 0x14, 0x06, 0x85, 0x65, 0x14, 0x06, 0x85, 0x65, 0x14, 0x06, 0x85, 0x65, 0x14, 0x06, 0x85, 0x65, 0x14 };

        EasyReplace(stats, "Female Changer 1", search, replace, 0, 0, 0, sizeof(uintptr_t));
    }

    {
        std::vector<uintptr_t> result;
        std::vector<BYTE> search = { 0xA4, 0x65, 0x14, 0x06, 0xA5, 0x65, 0x14, 0x06, 0xA6, 0x65, 0x14, 0x06, 0xA7, 0x65, 0x14, 0x06, 0xA8, 0x65, 0x14, 0x06, 0xC2, 0xA7, 0x23, 0x06, 0xC3, 0xA7, 0x23, 0x06, 0xC4, 0xA7, 0x23, 0x06, 0xA9, 0x65, 0x14, 0x06, 0xAC, 0x65, 0x14, 0x06, 0xAD, 0x65, 0x14, 0x06, 0xAE, 0x65, 0x14, 0x06, 0x78, 0xB3, 0x12, 0x06, 0x7F, 0x65, 0x14, 0x06, 0x71, 0x23, 0x05, 0x06, 0x72, 0x23, 0x05, 0x06, 0xB3, 0x65, 0x14, 0x06, 0x46, 0x23, 0x05, 0x06, 0x4F, 0x23, 0x05, 0x06, 0x85, 0x65, 0x14, 0x06, 0x9B, 0x65, 0x14, 0x06, 0x4B, 0x23, 0x05, 0x06, 0x57, 0x23, 0x05, 0x06, 0x8C, 0x65, 0x14, 0x06, 0x94, 0x65, 0x14, 0x06, 0x8F, 0x65, 0x14, 0x06, 0xAB, 0x65, 0x14, 0x06, 0x90, 0x65, 0x14, 0x06, 0xAA, 0x65, 0x14, 0x06 };
        std::vector<BYTE> replace = { 0xA4, 0x65, 0x14, 0x06, 0xA5, 0x65, 0x14, 0x06, 0xA6, 0x65, 0x14, 0x06, 0xA7, 0x65, 0x14, 0x06, 0xA8, 0x65, 0x14, 0x06, 0xC2, 0xA7, 0x23, 0x06, 0xC3, 0xA7, 0x23, 0x06, 0xC4, 0xA7, 0x23, 0x06, 0xA9, 0x65, 0x14, 0x06, 0xAC, 0x65, 0x14, 0x06, 0xAD, 0x65, 0x14, 0x06, 0xAE, 0x65, 0x14, 0x06, 0x78, 0xB3, 0x12, 0x06, 0x7F, 0x65, 0x14, 0x06, 0x85, 0x65, 0x14, 0x06, 0x85, 0x65, 0x14, 0x06, 0xB3, 0x65, 0x14, 0x06, 0x85, 0x65, 0x14, 0x06, 0x85, 0x65, 0x14, 0x06, 0x85, 0x65, 0x14, 0x06, 0x9B, 0x65, 0x14, 0x06, 0x85, 0x65, 0x14, 0x06, 0x85, 0x65, 0x14, 0x06, 0x8C, 0x65, 0x14, 0x06, 0x94, 0x65, 0x14, 0x06, 0x8F, 0x65, 0x14, 0x06, 0xAB, 0x65, 0x14, 0x06, 0x90, 0x65, 0x14, 0x06, 0xAA, 0x65, 0x14, 0x06 };

        EasyReplace(stats, "Female Changer 2", search, replace, 0, 0, 0, sizeof(uintptr_t));
    }
}

void quickSw(int& stats)
{
    {
        std::vector<uintptr_t> result;
        std::vector<BYTE> search = { 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, '?', '?', '?', 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, '?', '?', '?', '?', 0xC8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0xB4, 0xC8, 0xD6, 0x3F, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xB4, 0xC8, 0xD6, 0x3F, 0x00, 0x00, 0x00, 0x00, 0xB4, 0xC8, 0xD6, 0x3F, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x80, 0x3F, 0xCD, 0xCC, 0xCC, 0x3D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5C, 0x43, 0x00, 0x00, 0x90, 0x42, 0x00, 0x00, 0xB4, 0x42, 0x96, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3F, 0x00, 0x00, 0x80, 0x3E, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x20, 0x41, 0x00, 0x00, 0x34, 0x42, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F, 0x8F, 0xC2, 0x35, 0x3F, 0x9A, 0x99, 0x99, 0x3F, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x80, 0x3F, '?', '?', '?', '?', 0x01, 0x00, 0x00, 0x00, 0x0A, 0xD7, 0x23, 0x3C };
        std::vector<BYTE> replace = { 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, '?', '?', '?', 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, '?', '?', '?', '?', 0xC8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0xB4, 0xC8, 0xD6, 0x3F, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xB4, 0xC8, 0xD6, 0x3F, 0x00, 0x00, 0x00, 0x00, 0xB4, 0xC8, 0xD6, 0x3F, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x80, 0x3F, 0xCD, 0xCC, 0xCC, 0x3D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5C, 0x43, 0x00, 0x00, 0x90, 0x42, 0x00, 0x00, 0xB4, 0x42, 0x96, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1A, 0x00, 0x00, 0x80, 0x1A, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x20, 0x41, 0x00, 0x00, 0x34, 0x42, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F, 0x8F, 0xC2, 0x35, 0x3F, 0x9A, 0x99, 0x99, 0x3F, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x80, 0x3F };

        EasyReplace(stats, "Quick Switch 1", search, replace, 0, 0, 0, sizeof(DWORD_PTR));
    }

    {
        std::vector<uintptr_t> result;
        std::vector<BYTE> search = { 0x01, 0x00, 0x00, 0x00, 0xC3, 0xF5, 0xE8, 0x3F, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC3, 0xF5, 0xE8, 0x3F, 0x00, 0x00, 0x00, 0x00, 0xC3, 0xF5, 0xE8, 0x3F, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x80, 0x3F, 0xCD, 0xCC, 0xCC, 0x3D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5C, 0x43, 0x00, 0x00, 0x90, 0x42, 0x00, 0x00, 0xB4, 0x42, 0x96, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3F, 0x00, 0x00, 0x80, 0x3E, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x20, 0x41, 0x00, 0x00, 0x34, 0x42, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F, 0x0A, 0xD7, 0x23, 0x3F, 0x9A, 0x99, 0x99, 0x3F, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x0A, 0xD7, 0x23, 0x3C, 0xCD, 0xCC, 0xCC, 0x3D, 0x9A, 0x99, 0x19, 0x3F, 0x1F, 0x85, 0x6B, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3F, 0x00, 0x00, 0x00, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xCD, 0xCC, 0x4C, 0x3E, 0x01 };
        std::vector<BYTE> replace = { 0x01, 0x00, 0x00, 0x00, 0xC3, 0xF5, 0xE8, 0x3F, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC3, 0xF5, 0xE8, 0x3F, 0x00, 0x00, 0x00, 0x00, 0xC3, 0xF5, 0xE8, 0x3F, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x80, 0x3F, 0xCD, 0xCC, 0xCC, 0x3D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5C, 0x43, 0x00, 0x00, 0x90, 0x42, 0x00, 0x00, 0xB4, 0x42, 0x96, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1E, 0x00, 0x00, 0x80, 0x1E, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x20, 0x41, 0x00, 0x00, 0x34, 0x42, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F, 0x0A, 0xD7, 0x23, 0x3F, 0x9A, 0x99, 0x99, 0x3F, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x0A, 0xD7, 0x23, 0x3C, 0xCD, 0xCC, 0xCC, 0x3D, 0x9A, 0x99, 0x19, 0x3F, 0x1F, 0x85, 0x6B, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3F, 0x00, 0x00, 0x00, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xCD, 0xCC, 0x4C, 0x3E, 0x01 };

        EasyReplace(stats, "Quick Switch 2", search, replace, 0, 0, 0, sizeof(DWORD_PTR));
    }

    {
        std::vector<uintptr_t> result;
        std::vector<BYTE> search = { 0x80, 0x3F, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5C, 0x43, 0x00, 0x00, 0x28, 0x42, 0x00, 0x00, 0xB4, 0x42, 0x78, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x9A, 0x99, 0x19, 0x3F, 0x00, 0x00, 0x80, 0x3E, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x20, 0x41, 0x00, 0x00, 0x34, 0x42, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F, 0x9A, 0x99, 0x19, 0x3F, 0xCD, 0xCC, 0x8C, 0x3F, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x66, 0x66, 0x66, 0x3F, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x80, 0x3F };
        std::vector<BYTE> replace = { 0x80, 0x3F, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5C, 0x43, 0x00, 0x00, 0x28, 0x42, 0x00, 0x00, 0xB4, 0x42, 0x78, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x9A, 0x99, 0x19, 0x1E, 0x00, 0x00, 0x80, 0x1E, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x20, 0x41, 0x00, 0x00, 0x34, 0x42, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F, 0x9A, 0x99, 0x19, 0x3F, 0xCD, 0xCC, 0x8C, 0x3F, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x66, 0x66, 0x66, 0x3F, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x80, 0x3F };

        EasyReplace(stats, "Quick Switch 3", search, replace, 0, 0, 0, sizeof(DWORD_PTR));
    }
}

void glitch(int& stats)
{
    std::vector<uintptr_t> result;
    std::vector<BYTE> search = { 0x41, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0x3F, 0x00, 0x00, 0x00, 0x3F };
    std::vector<BYTE> replace = { 0x41, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0x9F, 0x00, 0x00, 0x00, 0x3F };

    OneByte(stats, "Glitch Fire", result, search, replace, 0, 0, 0);
}

void noreload(int& stats)
{
    std::vector<uintptr_t> result;
    std::vector<BYTE> search = { 0xA0, 0xE3, 0x6D, 0x00, 0x00, 0xEB, 0x00, 0x0A, 0xB7, 0xEE, 0x10, 0x0A, 0x01, 0xEE };
    std::vector<BYTE> replace = { 0xA0, 0xE3, 0x1D, 0x00, 0x00, 0xEB, 0x00, 0x0A, 0xB7, 0xEE, 0x10, 0x0A, 0x01, 0xEE };

    OneByte(stats, "No Reload", result, search, replace, 0, 0, 0);
}

void guest(int& stats)
{
    std::vector<uintptr_t> result;
    std::vector<BYTE> search = { 0x10, 0x40, 0x2D, 0xE9, 0xD0, 0x40, 0x9F, 0xE5, 0x04, 0x40, 0x8F, 0xE0, 0x00, 0x00, 0xD4, 0xE5, 0x00, 0x00, 0x50, 0xE3, 0x04, 0x00, 0x00, 0x1A, 0xC0, 0x00, 0x9F, 0xE5, 0x00, 0x00, 0x9F, 0xE7 };
    std::vector<BYTE> replace = { 0x01, 0x00, 0xA0, 0xE3, 0x1E, 0xFF, 0x2F, 0xE1, 0x04, 0x40, 0x8F, 0xE0, 0x00, 0x00, 0xD4, 0xE5, 0x00, 0x00, 0x50, 0xE3, 0x04, 0x00, 0x00, 0x1A, 0xC0, 0x00, 0x9F, 0xE5, 0x00, 0x00, 0x9F, 0xE7 };

    EasyReplace(stats, "Reset Guest", search, replace, 0, 0, 0, sizeof(uintptr_t));
}

void wall(int& stats)
{
    std::vector<uintptr_t> result;
    std::vector<BYTE> search = {  };
    std::vector<BYTE> replace = {  };

    EasyReplace(stats, "Wall Hack", search, replace, 0, 0, 0, sizeof(uintptr_t));
}

void sniperScope(int& stats)
{
    std::vector<uintptr_t> result;
    std::vector<BYTE> search = { 0xFF, 0xFF, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x40, 0xCD, 0xCC, 0x8C, 0x3F, 0x8F, 0xC2, 0xF5, 0x3C, 0xCD, 0xCC, 0xCC, 0x3D, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

    stats = 1;

    result.clear();
    FindPatternSelector(SAddress, EAddress, search.data(), result);

    storedSniper = result;

    stats = 0;
    std::string suc = "Sniper Scope successfully modified";
    AddNotification(suc, Notify_Success);
}

void speed(int& stats)
{
    std::vector<uintptr_t> result;
    std::vector<BYTE> search = { 0x02, 0x2B, 0x07, 0x3D, 0x02, 0x2B, 0x07, 0x3D, 0x02, 0x2B, 0x07, 0x3D, 0x00, 0x00, 0x00, 0x00, 0x9B, 0x6C };
    std::vector<BYTE> replace = { 0xE3, 0xA5, 0x9B, 0x3C, 0xE3, 0xA5, 0x9B, 0x3C, 0x02, 0x2B, 0x07, 0x3D, 0x00, 0x00, 0x00, 0x00, 0x9B, 0x6C };

    OneByte(stats, "Speed", result, search, replace, 0, 0, 0);
    storedSpeed = result;
}

void camera(int& stats)
{
    std::vector<uintptr_t> result;
    std::vector<BYTE> search = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xBF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xBF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xBF, 0x00, 0x00, 0x80, 0x7F, 0x00, 0x00, 0x80, 0x7F, 0x00, 0x00, 0x80, 0x7F, 0x00, 0x00, 0x80, 0xFF };
    std::vector<BYTE> replace = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x83, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xBF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xBF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xBF, 0x00, 0x00, 0x80, 0x7F, 0x00, 0x00, 0x80, 0x7F, 0x00, 0x00, 0x80, 0x7F, 0x00, 0x00, 0x00, 0x00 };

    OneByte(stats, "Camera", result, search, replace, 0, 0, 0);
    storedCamera = result;
}

void fastLanding(int& stats)
{
    std::vector<uintptr_t> result;
    std::vector<BYTE> search = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xBF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xBF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xBF, 0x00, 0x00, 0x80, 0x7F, 0x00, 0x00, 0x80, 0x7F, 0x00, 0x00, 0x80, 0x7F, 0x00, 0x00, 0x80, 0xFF };
    std::vector<BYTE> replace = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xBF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xBF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xBF, 0x00, 0x00, 0x80, 0x7F, 0x00, 0x00, 0x80, 0x7F, 0x00, 0x00, 0x80, 0x7F, 0x00, 0x00, 0x80, 0xFF };

    OneByte(stats, "Fast Landing", result, search, replace, 0, 0, 0);
    storedCamera = result;
}

void bypassLoading(int& stats)
{
    {
        std::vector<uintptr_t> result;
        std::vector<BYTE> search = { 0x30, 0x48, 0x2D, 0xE9, 0x0C, 0x41, 0x9F, 0xE5, 0x04, 0x40, 0x8F, 0xE0, 0x00, 0x00, 0xD4, 0xE5, 0x00, 0x00, 0x50, 0xE3, 0x07, 0x00, 0x00, 0x1A, 0xFC, 0x00, 0x9F, 0xE5, 0x00, 0x00, 0x9F, 0xE7, 0x35, 0xD8, 0xB5, 0xEB, 0xF4, 0x00, 0x9F, 0xE5, 0x00, 0x00, 0x9F, 0xE7, 0x32, 0xD8, 0xB5, 0xEB };
        std::vector<BYTE> replace = { 0x00, 0x00, 0xA0, 0xE3, 0x1E, 0xFF, 0x2F, 0xE1 };

        EasyReplace(stats, "Bypass Landing", search, replace, 0, 0, 0, sizeof(DWORD_PTR));
    }
}

void bypassLobby(int& stats)
{
    {
        std::vector<uintptr_t> result;
        std::vector<BYTE> search = { 0xF0, 0x4B, 0x2D, 0xE9, 0xF0, 0x51, 0x9F, 0xE5, 0x00, 0x40, 0xA0, 0xE1, 0x05, 0x50, 0x8F, 0xE0, 0x00, 0x00, 0xD5, 0xE5, 0x00, 0x00, 0x50, 0xE3, 0x07, 0x00, 0x00, 0x1A, 0xDC, 0x01, 0x9F, 0xE5 };
        std::vector<BYTE> replace = { 0x00, 0x00, 0xA0, 0xE3, 0x1E, 0xFF, 0x2F, 0xE1 };

        EasyReplace(stats, "Bypass Lobby", search, replace, 0, 0, 0, sizeof(DWORD_PTR));
    }
}

void KeyBindsFunction()
{
    while (true) {
        static bool dragOn = false;
        if ((GetAsyncKeyState(dragKey) & 1))
        {
            if (!resultForAll.empty()) {
                dragOn = !dragOn;
                if (selection == 0)
                {
                    for (auto addr : resultForAll)
                    {
                        uintptr_t readAddr = addr + (dragOn ? 234L : 194L);
                        uintptr_t writeAddr = addr + 182L;
                        uintptr_t buffer;
                        ReadProcessMemory(ProcessHandle, (LPCVOID)readAddr, &buffer, 4, 0);
                        WriteProcessMemory(ProcessHandle, (LPVOID)writeAddr, &buffer, 4, 0);
                    }
                    dragOn ? AddNotification("Drag On", Notify_Success) : AddNotification("Drag Off", Notify_Error);
                }
                else if (selection == 1)
                {
                    for (auto addr : resultForAll)
                    {
                        uintptr_t readAddr = addr + (dragOn ? 246L : 194L);
                        uintptr_t writeAddr = addr + 182L;
                        uintptr_t buffer;
                        ReadProcessMemory(ProcessHandle, (LPCVOID)readAddr, &buffer, 4, 0);
                        WriteProcessMemory(ProcessHandle, (LPVOID)writeAddr, &buffer, 4, 0);
                    }
                    dragOn ? AddNotification("Drag Fix On", Notify_Success) : AddNotification("Drag Fix Off", Notify_Error);
                }
            }
            else
            {
                Beep(500, 500);
                AddNotification("Open Cheat First!", Notify_Error);
            }
        }

        static bool isSpeedOn = false;
        if (GetAsyncKeyState(speedKey) & 1) {
            isSpeedOn = !isSpeedOn;

            if (storedSpeed.empty()) {
                Beep(500, 500);
                AddNotification("Open Cheat First!", Notify_Error);
                return;
            }

            std::vector<BYTE> replace_speed = isSpeedOn ?
                std::vector<BYTE>{ 0x02, 0x2B, 0x07, 0x3D, 0x02, 0x2B, 0x07, 0x3D, 0x02, 0x2B, 0x07, 0x3D, 0x00, 0x00, 0x00, 0x00, 0x9B, 0x6C } :
                std::vector<BYTE>{ 0xE3, 0xA5, 0x9B, 0x3C, 0xE3, 0xA5, 0x9B, 0x3C, 0x02, 0x2B, 0x07, 0x3D, 0x00, 0x00, 0x00, 0x00, 0x9B, 0x6C };

            for (uintptr_t addr : storedSpeed) {
                WriteProcessMemory(ProcessHandle, (LPVOID)addr, replace_speed.data(), replace_speed.size(), 0);
            }

            isSpeedOn ? AddNotification("Speed On", Notify_Success) : AddNotification("Speed Off", Notify_Error);
        }

        static bool isCameraOn = false;
        if (GetAsyncKeyState(cameraKey) & 1) {
            isCameraOn = !isCameraOn;

            if (storedCamera.empty()) {
                Beep(500, 500);
                return;
            }

            std::vector<BYTE> replace_speed = isCameraOn ?
                std::vector<BYTE>{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x83, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xBF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xBF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xBF, 0x00, 0x00, 0x80, 0x7F, 0x00, 0x00, 0x80, 0x7F, 0x00, 0x00, 0x80, 0x7F, 0x00, 0x00, 0x00, 0x00 } :
                std::vector<BYTE>{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xBF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xBF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xBF, 0x00, 0x00, 0x80, 0x7F, 0x00, 0x00, 0x80, 0x7F, 0x00, 0x00, 0x80, 0x7F, 0x00, 0x00, 0x80, 0xFF };

            for (uintptr_t addr : storedCamera) {
                WriteProcessMemory(ProcessHandle, (LPVOID)addr, replace_speed.data(), replace_speed.size(), 0);
            }

            isCameraOn ? AddNotification("Camera On", Notify_Success) : AddNotification("Camera Off", Notify_Error);
        }

        static bool isLandingOn = false;
        if (GetAsyncKeyState(landingKey) & 1) {
            isLandingOn = !isLandingOn;

            if (storedCamera.empty()) {
                Beep(500, 500);
                return;
            }

            std::vector<BYTE> replace_speed = isLandingOn ?
                std::vector<BYTE>{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xBF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xBF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xBF, 0x00, 0x00, 0x80, 0x7F, 0x00, 0x00, 0x80, 0x7F, 0x00, 0x00, 0x80, 0x7F, 0x00, 0x00, 0x80, 0xFF } :
                std::vector<BYTE>{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xBF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xBF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xBF, 0x00, 0x00, 0x80, 0x7F, 0x00, 0x00, 0x80, 0x7F, 0x00, 0x00, 0x80, 0x7F, 0x00, 0x00, 0x80, 0xFF };

            for (uintptr_t addr : storedCamera) {
                WriteProcessMemory(ProcessHandle, (LPVOID)addr, replace_speed.data(), replace_speed.size(), 0);
            }

            isLandingOn ? AddNotification("Landing On", Notify_Success) : AddNotification("Landing Off", Notify_Error);
        }


        if (GetAsyncKeyState(sniperKey) & 0x1)
        {
            if (storedSniper.empty()) {
                Beep(500, 500);
                return;
            }

            std::vector<BYTE> onBytes = { 0xFF, 0xFF, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x40, 0xCD, 0xCC, 0x8C, 0x3F, 0x8F, 0xC2, 0xF5, 0x3C, 0xCD, 0xCC, 0xCC, 0x3D, 0x06, 0x00, 0x00, 0x00, 0x57, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc6, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
            std::vector<BYTE> offBytes = { 0xFF, 0xFF, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x40, 0xCD, 0xCC, 0x8C, 0x3F, 0x8F, 0xC2, 0xF5, 0x3C, 0xCD, 0xCC, 0xCC, 0x3D, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

            for (uintptr_t addr : storedSniper) {
                WriteProcessMemory(ProcessHandle, (LPVOID)addr, onBytes.data(), onBytes.size(), 0);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(sliderSniper));
            //  Sleep(sliderSniper);
            for (uintptr_t addr : storedSniper) {
                WriteProcessMemory(ProcessHandle, (LPVOID)addr, offBytes.data(), offBytes.size(), 0);
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}
