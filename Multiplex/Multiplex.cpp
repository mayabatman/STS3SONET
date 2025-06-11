#include <windows.h> 
#include <stdio.h> 
#include <tchar.h>
#include <strsafe.h>
#include <iostream>
#include <bitset>

#define BUFSIZE 1

int _tmain(VOID)
{
    BOOL   fConnected = FALSE;
    DWORD  dwThreadId = 0;
    HANDLE hPipe[3];
    LPCTSTR lpszPipename = TEXT("\\\\.\\pipe\\sts");


    for (int i = 0; i < 3; i++)
    {
        printf(TEXT("\nMultiplex awaiting client connection on %s\n"), lpszPipename);
        hPipe[i] = CreateNamedPipe(
            lpszPipename,             // имя пайпа
            PIPE_ACCESS_INBOUND,      // записывают клиенты, читает сервер
            PIPE_TYPE_BYTE |          // принимаем байтики
            PIPE_READMODE_BYTE |      // принимаем байтики
            PIPE_WAIT,                // функции write read блокируются пока не получится выполнить
            PIPE_UNLIMITED_INSTANCES, // максимальное количество экземпляров пайпа
            BUFSIZE,                  // размер буфера выхода
            BUFSIZE,                  // размер буфера входа
            0,                        // таймайт клиента будет 50 мс
            NULL);                    // аттрибуты безопасности будут по умолчанию

        if (hPipe[i] == INVALID_HANDLE_VALUE)
        {
            printf(TEXT("CreateNamedPipe failed, GLE=%d.\n"), GetLastError());
            return -1;
        }

        //ждем i-того STS

        fConnected = ConnectNamedPipe(hPipe[i], NULL) ?
            TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);

        if (fConnected)
        {
            printf("%d Client connected.\n", i + 1);
        }
        else {
            printf("%d Client connection fail %d", i+1, GetLastError());
            //если у STS не получилось подключиться нахер сворачиваемся
            CloseHandle(hPipe[i]);
            return -1;
        }
    }
    HANDLE hHeap = GetProcessHeap();
    BOOL fSuccess = FALSE;
    TCHAR* pchRequest = (TCHAR*)HeapAlloc(hHeap, 0, BUFSIZE * sizeof(TCHAR));
    DWORD cbBytesRead = 0;

    //тут будет фрейм из трех фреймов
    char frame[810*3];
    //тут следим сколько во фрейме набралось байтиков
    uint16_t byte_count = 0;
    //крутимся вертимся пока какой-нибудь STS не сломается
    while (hPipe[0] != INVALID_HANDLE_VALUE || hPipe[1] != INVALID_HANDLE_VALUE || hPipe[2] != INVALID_HANDLE_VALUE)
    {
        for (int i = 0; i < 3; i++) {
            //читаем (ждем) что там записал клиентик
            fSuccess = ReadFile(
                hPipe[i],                //экземпляр пайпа
                pchRequest,              //куда запишем пришедший байтик
                BUFSIZE * sizeof(TCHAR), //размер пришедшего байтика
                &cbBytesRead,            // сколько байтиков пришло
                NULL);                   // не используем overlapped I/O 

            if (!fSuccess || cbBytesRead == 0)
            {
                if (GetLastError() == ERROR_BROKEN_PIPE)
                {
                    printf(TEXT("client disconnected.\n"));
                }
                else
                {
                    printf(TEXT("ReadFile failed, GLE=%d.\n"), GetLastError());
                }
                //если была ошибка чтения, то сворачиваем данный экземляр и уходим
                FlushFileBuffers(hPipe[i]);
                DisconnectNamedPipe(hPipe[i]);
                CloseHandle(hPipe[i]);
                hPipe[i] = INVALID_HANDLE_VALUE;
                continue;
            }

            frame[byte_count] = (*pchRequest);
            byte_count++;

        }
        //как только набирается байтов для фрейма, выводим его и идем дальше
        if (byte_count >= sizeof(frame)) {
            // это если не побитно выводить будем
            printf(TEXT("\n\n---------FRAME OF %d BYTES--------------\n\n"), (int)sizeof(frame));
            for (int i = 0; i < sizeof(frame); i++) {
                printf(TEXT("%d"), frame[i]);
            }
            // а вот это уже побитно
            // включаем Lumen и подпеваем ~нули и единицы~
            //std::cout << "\n\n---------FRAME OF " << sizeof(frame)<<" BYTES--------------\n\n";
            //for (int i = 0; i < sizeof(frame); i++) {
            //    std::cout << std::bitset<8>(frame[i]);
            //}
            //обнуляем подсчет
            byte_count = 0;
        }
    }
    
    //освобождаем буфер
    HeapFree(hHeap, 0, pchRequest);
    //выход
    printf("exit\n");

    return 0;
}
