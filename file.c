#ifndef FILE_C
#define FILE_C

#include <windows.h>
#include "core.c"
#include "app.c"

void FindAllFiles(char *folder, AppState *state)
{
    WIN32_FIND_DATAA findFileData;
    HANDLE hFind = INVALID_HANDLE_VALUE;

    StringBuffer path = InitStringBufferWithDoubledCapacity(folder);

    AppendStr(&path, "\\*");

    if (state->modal.searchTerm.size > 0)
    {
        AppendStr(&path, state->modal.searchTerm.content);
        AppendStr(&path, "*");
    }

    hFind = FindFirstFileA(path.content, &findFileData);

    for (int i = 0; i < state->modal.filesCount; i++)
    {
        VirtualFreeMemory(state->modal.files[i].content);
    }
    state->modal.filesCount = 0;

    if(hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            if (IsStrEqualToStr(findFileData.cFileName, ".") || IsStrEqualToStr(findFileData.cFileName, ".."))
                continue;

            if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                state->modal.files[state->modal.filesCount] = InitStringBufferSameCapacity(findFileData.cFileName);
                state->modal.filesCount++;
            }
        } while (FindNextFileA(hFind, &findFileData) != 0);
    }

    FindClose(hFind);

    // TODO: play around and see if I can detect a memory leak in proc monitor
    VirtualFreeMemory(path.content);

    state->modal.selectedFileIndex = 0;
}

#endif