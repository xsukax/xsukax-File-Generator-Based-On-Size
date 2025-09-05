// xsukax_filegen.cpp - Minimal Windows Native C++ Version
#ifndef UNICODE
#define UNICODE
#endif

#include <windows.h>
#include <commctrl.h>
#include <string>
#include <thread>
#include <atomic>
#include <chrono>
#include <cmath>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <ctime>

// Control IDs
#define ID_EDIT_SIZE 1001
#define ID_BTN_GENERATE 1002
#define ID_BTN_STOP 1003
#define ID_PROGRESS 1004
#define ID_STATUS 1005
#define WM_UPDATE_PROGRESS (WM_USER + 1)
#define WM_COMPLETE_PROCESS (WM_USER + 2)
#define WM_STOP_PROCESS (WM_USER + 3)

// Thread data structure
struct ThreadData {
    std::atomic<uint64_t> bytes_written{0};
    uint64_t total_bytes = 0;
    std::atomic<bool> stop_requested{false};
    std::atomic<bool> update_pending{false};
    std::string error_msg;
    std::string output_filename;
    std::chrono::steady_clock::time_point start_time;
    HWND hwnd_main = nullptr;
    bool is_running = false;
};

// Global variables
HINSTANCE g_hInst;
HWND g_hwndMain;
HWND g_hwndEdit, g_hwndGenerate, g_hwndStop, g_hwndProgress, g_hwndStatus;
std::unique_ptr<ThreadData> g_threadData;

// xsukax repositories content
static const char XSUKAX_REPOS_TEXT[] = 
    "=== xsukax GitHub Profile ===\n"
    "Profile URL: https://github.com/xsukax\n"
    "Website: https://xsukax.com\n"
    "Mastodon: @xsukax@infosec.exchange\n"
    "Total Public Repositories: 6 | Followers: 10 | Following: 3\n"
    "Primary Focus: Windows utilities, Security tools, IPFS applications, DNS privacy\n\n"
    
    "Repository: xwgg\n"
    "Description: Simple tool to Generate batch script to install the most needed windows applications via winget tool\n"
    "Language: HTML | Stars: 7 | Forks: 3 | Status: Public\n"
    "Website: https://xwgg.xsukax.com/\n"
    "Features: Winget package manager integration, Batch script generation, Windows app installer\n"
    "Purpose: Automated Windows application installation using Microsoft's winget package manager\n"
    "This tool helps users quickly generate batch scripts to install common Windows applications.\n\n"
    
    "Repository: Encrypted-DNS\n"
    "Description: Simple Batch script to use Encrypted DNS (DOH) for Windows 10/11\n"
    "Language: Batchfile | Stars: 1 | Forks: 0 | Status: Public\n"
    "Features: DNS-over-HTTPS (DOH) support, Multiple DNS providers, Network interface selection\n"
    "Supported Providers: Google DNS, Quad9, AdGuard DNS, Cloudflare DNS\n"
    "Security: Enhances privacy by encrypting DNS queries using DOH protocol\n"
    "Implementation: Uses dnsproxy commandline application for encrypted DNS queries\n"
    "Compatibility: Windows 10 and Windows 11 operating systems\n\n"
    
    "Repository: ipfs-tools\n"
    "Description: Simple HTML CSS JS code to help IPFS Users to View & Download Files From Different Gateways\n"
    "Language: HTML | Stars: 0 | Forks: 0 | Status: Public\n"
    "Features: Multiple IPFS gateway support, File viewing interface, Download functionality\n"
    "Technology Stack: HTML, CSS, JavaScript (client-side application)\n"
    "Purpose: Provides a web interface for accessing IPFS content through various gateways\n"
    "Use Case: Helps users bypass blocked or slow IPFS gateways by providing alternatives\n\n"
    
    "Repository: xcidgw\n"
    "Description: Simple C++ Application to open IPFS CID in different gateways\n"
    "Language: Shell | Stars: 0 | Forks: 0 | Status: Public\n"
    "Features: IPFS CID handling, Multiple gateway support, Cross-platform compatibility\n"
    "Purpose: Command-line tool for opening IPFS Content Identifiers in various IPFS gateways\n"
    "Implementation: Shell scripting for gateway selection and URL handling\n"
    "Target Audience: IPFS users who need reliable access to distributed content\n\n"
    
    "Repository: xwinapps (Archived)\n"
    "Description: Windows Applications Installer by xsukax using winget command in a batch script\n"
    "Language: Batchfile | Status: Public Archive\n"
    "Legacy Status: This repository has been archived and is no longer actively maintained\n"
    "Successor: Functionality has been superseded by the newer 'xwgg' repository\n"
    "Historical Purpose: Automated Windows application installation using winget package manager\n"
    "Note: Users should refer to the 'xwgg' repository for current Windows app installation tools\n\n"
    
    "Repository: Windows-Cleaner.bat\n"
    "Description: An executable cmd batch script that Cleans Windows from unneeded temporary files\n"
    "Language: Batchfile | Status: Public (Forked from TarikSeyceri/Windows-Cleaner.bat)\n"
    "Original Author: TarikSeyceri | Forked and maintained by xsukax\n"
    "Features: Temporary file cleanup, System optimization, Registry cleaning, Disk space recovery\n"
    "Functionality: Removes unnecessary files to improve Windows system performance\n"
    "Safety: Includes safeguards to prevent deletion of critical system files\n"
    "Usage: Run as administrator for comprehensive system cleanup\n\n"
    
    "Additional Project: xsukax-AES-256-File-Folder-Encryption-Decryption-Tool\n"
    "Description: Comprehensive Python application providing military-grade AES-256 encryption\n"
    "Language: Python | Features: GUI and CLI modes, TAR archiving, Folder encryption\n"
    "Security: AES-256 encryption standard, Secure file and folder encryption/decryption\n"
    "Performance: High-speed processing with 125+ MB/s encryption rates\n"
    "Interfaces: Full-featured GUI and powerful command-line interface\n"
    "Capabilities: Individual file encryption, Complete folder structure encryption\n"
    "Technology: Uses Python cryptography library for secure encryption implementation\n\n"
    
    "Developer Profile Summary:\n"
    "xsukax is a developer focused on creating practical Windows utilities and security tools.\n"
    "Main interests include system administration, privacy tools, IPFS technology, and Windows automation.\n"
    "Projects demonstrate expertise in batch scripting, web technologies, and cryptographic applications.\n"
    "Active in the information security community with presence on Mastodon and personal website.\n"
    "Contributions include both original projects and community-driven forks and improvements.\n\n";

static const size_t XSUKAX_REPOS_TEXT_LEN = sizeof(XSUKAX_REPOS_TEXT) - 1;

// Utility functions
std::wstring StringToWString(const std::string& str) {
    if (str.empty()) return std::wstring();
    int size = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), nullptr, 0);
    std::wstring result(size, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &result[0], size);
    return result;
}

std::string WStringToString(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();
    int size = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), nullptr, 0, nullptr, nullptr);
    std::string result(size, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &result[0], size, nullptr, nullptr);
    return result;
}

// Generate unique filename with timestamp
std::string GenerateOutputFilename() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::tm* tm_info = std::localtime(&time_t);
    
    std::ostringstream oss;
    oss << "xsukax_output_"
        << std::put_time(tm_info, "%Y%m%d_%H%M%S")
        << "_" << std::setfill('0') << std::setw(3) << ms.count()
        << ".txt";
    
    return oss.str();
}

long long ParseSizeAccurate(const std::string& size_str) {
    if (size_str.empty()) return -1;
    
    std::istringstream iss(size_str);
    double number;
    std::string unit;
    
    if (!(iss >> number) || number <= 0) return -1;
    iss >> unit;
    
    // Convert to uppercase
    for (char& c : unit) {
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }
    
    double bytes_exact;
    if (unit.empty() || unit == "B") {
        bytes_exact = number;
    } else if (unit == "KB") {
        bytes_exact = number * 1024.0;
    } else if (unit == "MB") {
        bytes_exact = number * 1024.0 * 1024.0;
    } else if (unit == "GB") {
        bytes_exact = number * 1024.0 * 1024.0 * 1024.0;
    } else if (unit == "TB") {
        bytes_exact = number * 1024.0 * 1024.0 * 1024.0 * 1024.0;
    } else {
        return -1;
    }
    
    return (long long)round(bytes_exact);
}

void GenerateXsukaxTextFast(char* buffer, size_t size) {
    if (size == 0) return;
    
    const size_t text_len = XSUKAX_REPOS_TEXT_LEN;
    
    if (size <= text_len) {
        memcpy(buffer, XSUKAX_REPOS_TEXT, size);
        return;
    }
    
    size_t remaining = size;
    char* dest = buffer;
    
    while (remaining >= text_len) {
        memcpy(dest, XSUKAX_REPOS_TEXT, text_len);
        dest += text_len;
        remaining -= text_len;
    }
    
    if (remaining > 0) {
        memcpy(dest, XSUKAX_REPOS_TEXT, remaining);
    }
}

// File generation thread
void FileGenerationThread(ThreadData* t) {
    t->start_time = std::chrono::steady_clock::now();
    t->is_running = true;
    
    std::ofstream file(t->output_filename, std::ios::binary);
    if (!file.is_open()) {
        t->error_msg = "Failed to create file. Check permissions.";
        PostMessage(t->hwnd_main, WM_COMPLETE_PROCESS, 0, 0);
        return;
    }
    
    const size_t buffer_size = 16 * 1024 * 1024; // 16MB buffer
    auto buffer = std::make_unique<char[]>(buffer_size);
    
    uint64_t total_bytes = t->total_bytes;
    uint64_t bytes_written = 0;
    
    GenerateXsukaxTextFast(buffer.get(), buffer_size);
    
    while (bytes_written < total_bytes) {
        if (t->stop_requested.load()) {
            file.close();
            DeleteFileA(t->output_filename.c_str());
            PostMessage(t->hwnd_main, WM_STOP_PROCESS, 0, 0);
            return;
        }
        
        uint64_t remaining = total_bytes - bytes_written;
        size_t write_size = (remaining > buffer_size) ? buffer_size : (size_t)remaining;
        
        if (write_size < buffer_size) {
            GenerateXsukaxTextFast(buffer.get(), write_size);
        }
        
        file.write(buffer.get(), write_size);
        if (file.fail()) {
            t->error_msg = "Write failed. Check disk space.";
            PostMessage(t->hwnd_main, WM_COMPLETE_PROCESS, 0, 0);
            return;
        }
        
        bytes_written += write_size;
        t->bytes_written.store(bytes_written);
        
        if (!t->update_pending.exchange(true)) {
            PostMessage(t->hwnd_main, WM_UPDATE_PROGRESS, 0, 0);
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    file.close();
    
    if (!t->stop_requested.load()) {
        PostMessage(t->hwnd_main, WM_COMPLETE_PROCESS, 0, 0);
    }
}

// Window procedure
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE: {
        // Initialize common controls
        INITCOMMONCONTROLSEX icex;
        icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
        icex.dwICC = ICC_PROGRESS_CLASS;
        InitCommonControlsEx(&icex);
        
        // Create controls with simple layout
        CreateWindowW(L"STATIC", L"File Size:", WS_CHILD | WS_VISIBLE,
                     10, 10, 100, 20, hwnd, nullptr, g_hInst, nullptr);
        
        g_hwndEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"1GB",
                                    WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
                                    10, 35, 200, 25, hwnd, (HMENU)ID_EDIT_SIZE, g_hInst, nullptr);
        
        g_hwndGenerate = CreateWindowW(L"BUTTON", L"Generate File",
                                      WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                      10, 70, 100, 30, hwnd, (HMENU)ID_BTN_GENERATE, g_hInst, nullptr);
        
        g_hwndStop = CreateWindowW(L"BUTTON", L"Stop",
                                  WS_CHILD | BS_PUSHBUTTON,
                                  120, 70, 60, 30, hwnd, (HMENU)ID_BTN_STOP, g_hInst, nullptr);
        
        g_hwndProgress = CreateWindowExW(0, PROGRESS_CLASSW, nullptr,
                                        WS_CHILD | WS_VISIBLE | PBS_SMOOTH,
                                        10, 110, 300, 20, hwnd, (HMENU)ID_PROGRESS, g_hInst, nullptr);
        
        g_hwndStatus = CreateWindowW(L"STATIC", L"Ready to generate",
                                    WS_CHILD | WS_VISIBLE,
                                    10, 140, 300, 40, hwnd, (HMENU)ID_STATUS, g_hInst, nullptr);
        
        break;
    }
    
    case WM_COMMAND: {
        switch (LOWORD(wParam)) {
        case ID_BTN_GENERATE: {
            wchar_t buffer[256];
            GetWindowTextW(g_hwndEdit, buffer, 256);
            std::string size_str = WStringToString(buffer);
            
            long long bytes = ParseSizeAccurate(size_str);
            if (bytes <= 0) {
                SetWindowTextW(g_hwndStatus, L"Invalid size format. Try: 10MB, 2GB, 500KB");
                break;
            }
            
            // Update UI
            EnableWindow(g_hwndGenerate, FALSE);
            ShowWindow(g_hwndGenerate, SW_HIDE);
            ShowWindow(g_hwndStop, SW_SHOW);
            SendMessage(g_hwndProgress, PBM_SETPOS, 0, 0);
            SetWindowTextW(g_hwndStatus, L"Generating file...");
            
            // Start generation thread
            g_threadData = std::make_unique<ThreadData>();
            g_threadData->total_bytes = bytes;
            g_threadData->hwnd_main = hwnd;
            g_threadData->output_filename = GenerateOutputFilename();
            
            std::thread(FileGenerationThread, g_threadData.get()).detach();
            break;
        }
        
        case ID_BTN_STOP: {
            if (g_threadData) {
                g_threadData->stop_requested.store(true);
            }
            break;
        }
        }
        break;
    }
    
    case WM_UPDATE_PROGRESS: {
        if (g_threadData) {
            g_threadData->update_pending.store(false);
            
            double fraction = 0.0;
            if (g_threadData->total_bytes > 0) {
                fraction = (double)g_threadData->bytes_written.load() / (double)g_threadData->total_bytes;
                if (fraction > 1.0) fraction = 1.0;
            }
            
            // Update progress bar
            SendMessage(g_hwndProgress, PBM_SETPOS, (WPARAM)(fraction * 100), 0);
            
            // Calculate speed
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration<double>(now - g_threadData->start_time).count();
            double speed_mbps = (elapsed > 0.01) ? (g_threadData->bytes_written.load() / (1024.0 * 1024.0)) / elapsed : 0.0;
            
            std::wostringstream status;
            status << L"Generating: " << std::fixed << std::setprecision(1) << (fraction * 100.0) 
                   << L"% - " << std::setprecision(1) << speed_mbps << L" MB/s";
            SetWindowTextW(g_hwndStatus, status.str().c_str());
        }
        break;
    }
    
    case WM_COMPLETE_PROCESS: {
        if (g_threadData) {
            if (!g_threadData->error_msg.empty()) {
                SetWindowTextW(g_hwndStatus, StringToWString(g_threadData->error_msg).c_str());
            } else {
                SendMessage(g_hwndProgress, PBM_SETPOS, 100, 0);
                
                auto elapsed = std::chrono::duration<double>(
                    std::chrono::steady_clock::now() - g_threadData->start_time).count();
                
                std::wostringstream complete;
                complete << L"File '" << StringToWString(g_threadData->output_filename) 
                        << L"' created in " << std::fixed << std::setprecision(2) << elapsed << L" seconds";
                SetWindowTextW(g_hwndStatus, complete.str().c_str());
            }
            
            EnableWindow(g_hwndGenerate, TRUE);
            ShowWindow(g_hwndGenerate, SW_SHOW);
            ShowWindow(g_hwndStop, SW_HIDE);
            g_threadData.reset();
        }
        break;
    }
    
    case WM_STOP_PROCESS: {
        SendMessage(g_hwndProgress, PBM_SETPOS, 0, 0);
        SetWindowTextW(g_hwndStatus, L"Generation stopped by user");
        EnableWindow(g_hwndGenerate, TRUE);
        ShowWindow(g_hwndGenerate, SW_SHOW);
        ShowWindow(g_hwndStop, SW_HIDE);
        g_threadData.reset();
        break;
    }
    
    case WM_DESTROY: {
        // Clean up
        if (g_threadData) {
            g_threadData->stop_requested.store(true);
        }
        PostQuitMessage(0);
        break;
    }
    
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

// Main function
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow) {
    g_hInst = hInstance;
    
    // Register window class
    const wchar_t CLASS_NAME[] = L"XsukaxFileGenerator";
    
    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    
    RegisterClass(&wc);
    
    // Create window
    g_hwndMain = CreateWindowExW(
        0,
        CLASS_NAME,
        L"xsukax File Generator",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 340, 220,
        nullptr, nullptr, hInstance, nullptr
    );
    
    if (g_hwndMain == nullptr) {
        return 0;
    }
    
    ShowWindow(g_hwndMain, nCmdShow);
    UpdateWindow(g_hwndMain);
    
    // Message loop
    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    return 0;
}