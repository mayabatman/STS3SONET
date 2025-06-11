#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <iostream>
#include <string>
#include <thread>
#include <chrono>

void _tmain(int argc, TCHAR* argv[])
{
    STARTUPINFO si[4];
    PROCESS_INFORMATION pi[4];

    for (int i = 0; i < 4; i++) {
        ZeroMemory(&si[i], sizeof(si[i]));
        si[i].cb = sizeof(si[i]);
        ZeroMemory(&pi, sizeof(pi[i]));
    }
    //получаем полный путь к файлу
    TCHAR buffer[MAX_PATH] = { 0 };
    GetModuleFileName(NULL, buffer, MAX_PATH);
    std::string::size_type pos = std::string(buffer).find_last_of("\\/");
    std::string path = std::string(buffer).substr(0, pos);
    std::string path_sts = path + "\\STS.exe";
    std::string path_muliplex = path + "\\Multiplex.exe";

    // запускаем мультиплекс

    if (!CreateProcess(NULL,
        (LPTSTR)const_cast<char*>(path_muliplex.c_str()),        // Command line
        NULL,           // Process handle not inheritable
        NULL,           // Thread handle not inheritable
        FALSE,          // Set handle inheritance to FALSE
        0,              // No creation flags
        NULL,           // Use parent's environment block
        NULL,           // Use parent's starting directory 
        &si[0],            // Pointer to STARTUPINFO structure
        &pi[0])           // Pointer to PROCESS_INFORMATION structure
        )
    {
        printf("CreateProcess failed (%d).\n", GetLastError());
        return;
    }

    //задержка, чтобы пайп спокойно создался в мультиплексе
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    // запускаем sts-ы
    for (int i = 1; i <= 3; i++) {
        std::string path = path_sts + " "+std::to_string(i);
        if (!CreateProcess(NULL,
            (LPTSTR)const_cast<char*>(path.c_str()),        // Command line
            NULL,           // Process handle not inheritable
            NULL,           // Thread handle not inheritable
            FALSE,          // Set handle inheritance to FALSE
            0,              // No creation flags
            NULL,           // Use parent's environment block
            NULL,           // Use parent's starting directory 
            &si[i],            // Pointer to STARTUPINFO structure
            &pi[i])           // Pointer to PROCESS_INFORMATION structure
            )
        {
            printf("CreateProcess failed (%d).\n", GetLastError());
            return;
        }
    }

    for (int i = 0; i < 2; i++) {
        // Wait until child process exits.
        WaitForSingleObject(pi[i].hProcess, INFINITE);

        // Close process and thread handles. 
        CloseHandle(pi[i].hProcess);
        CloseHandle(pi[i].hThread);
    }

}