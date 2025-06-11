#include <windows.h> 
#include <stdio.h>
#include <conio.h>
#include <tchar.h>
#include <iostream>
#include <fstream>
#include <bitset>
#include <string>
#include <sstream>
#include <windows.h> 
#include <stdio.h>
#include <conio.h>
#include <tchar.h>
#include <filesystem>
#include <thread>
#define BUFSIZE 8

int _tmain(int argc, TCHAR* argv[])
{

    if (argc <= 1)
    {
        return -1;
    }

    setlocale(LC_CTYPE, "rus");

    std::ifstream  f1;
    uint8_t bites[810];
    char* end_str;
    end_str = (char*)(".png");

    HANDLE hPipe;
    LPTSTR lpvMessage = (LPTSTR)TEXT("Default message from client.");
    TCHAR  chBuf[BUFSIZE];
    BOOL   fSuccess = FALSE;
    DWORD  cbRead, cbToWrite, cbWritten, dwMode;
    LPTSTR lpszPipename = (LPTSTR)TEXT("\\\\.\\pipe\\sts");
    
    // каждый STS открывает свой файл
    std::string str_file = std::string(argv[1]) + ".png";

    //получаем полный путь к файлу
    TCHAR buffer[MAX_PATH] = { 0 };
    GetModuleFileName(NULL, buffer, MAX_PATH);
    std::string::size_type pos = std::string(buffer).find_last_of("\\/");
    std::string path = std::string(buffer).substr(0, pos);
    path +="\\"+str_file;

    //открываем бинарненько
    f1.open(path, std::ios::in | std::ios::binary);

    while (1)
    {
        //открываем пайп
        hPipe = CreateFile(
            lpszPipename,   // пайп
            GENERIC_WRITE,  // только писать будем
            0,              // без настроек sharing
            NULL,           // аттрибуты безопасности по умолчанию потому что он все равно игнорится когда это пайп
            OPEN_EXISTING,  // открываем существующий пайп
            0,              // аттрибуты по умолчанию
            NULL);          // без файла атрибутов потому что он все равно игнорится когда это пайп

        //выходим если получили нормальный хэндл на пайп
        if (hPipe != INVALID_HANDLE_VALUE)
            break;

        //если ошибка не в том, что пайп занят, то до свидания
        if (GetLastError() != ERROR_PIPE_BUSY)
        {
            printf(TEXT("Could not open pipe. GLE=%d\n"), GetLastError());
            return -1;
        }

        //ждем освобождения максимум 20 сек
        if (!WaitNamedPipe(lpszPipename, 20000))
        {
            printf("Could not open pipe: 20 second wait timed out.");
            return -1;
        }
    }

    //читаем и отправляем файл
    while (!f1.eof()) {
        //прочитали 810 байтов
        f1.read((char*)bites, sizeof(bites));
        //побайтно отправляем
        for (int i = 0; i < sizeof(bites); i++) {
            //сообщение
            lpvMessage = (LPTSTR)(&bites[i]);

            fSuccess = WriteFile(
                hPipe,                  // пайп
                lpvMessage,             // сообщение 
                1,                      // длина сообщения (всегда 1 байт отправляем) 
                &cbWritten,             // сюда запишется сколько байт послалось 
                NULL);                  // не overlapped 

            if (!fSuccess)
            {
                printf(TEXT("WriteFile to pipe failed. GLE=%d\n"), GetLastError());
                return -1;
            }
        }
    }

    //если файл закончился продолжаем отправлять пустые данные
    char null_c = 0;
    while (true) {
        lpvMessage = (LPTSTR)(&null_c);

        cbToWrite = (lstrlen(lpvMessage) + 1) * sizeof(TCHAR);

        fSuccess = WriteFile(
            hPipe,                  // пайп
            lpvMessage,             // сообщение 
            1,                      // длина сообщения (всегда 1 байт отправляем) 
            &cbWritten,             // сюда запишется сколько байт послалось 
            NULL);                  // не overlapped 

        if (!fSuccess)
        {
            printf(TEXT("WriteFile to pipe failed. GLE=%d\n"), GetLastError());
            return -1;
        }
    }

    CloseHandle(hPipe);

    return 0;
}
