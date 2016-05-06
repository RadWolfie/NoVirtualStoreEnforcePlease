#include "stdafx.h"

extern _Bool isDirFileFullPermission(const wchar_t* str, unsigned int* result) {
    struct _stat info;
    *result = _wstat64i32(str, &info);
    if (!(info.st_mode & S_IREAD))
        return 0;
    else if (!(info.st_mode & S_IWRITE))
        return 0;
    else if (info.st_nlink == 1 && info.st_mode & S_IFREG)
        return 1;
    else if (!(info.st_mode & S_IEXEC))
        return 0;
    else
        return 1;
}

extern HMODULE GetCurrentModule() { // NB: XP+ solution!
    HMODULE hModule = NULL;
    GetModuleHandleEx(
        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
        (LPCTSTR)GetCurrentModule,
        &hModule);

    return hModule;
}

extern void strcatW(wchar_t *dest, size_t len, const wchar_t *src) {
    size_t i = 0;
    size_t add = 0;
    while (i < len) {
        if (add) {
            goto wcsAppend;
        }
        else if (dest[i] == 0) {
        wcsAppend:
            (wchar_t)dest[i] = src[add];
            if (!src[add])
                break;
            add++;
        }
        i++;
    }
}
extern void strcatA(char *dest, size_t len, const char *src) {
    size_t i = 0;
    size_t add = 0;
    while (i < len) {
        if (add) {
            goto strAppend;
        }
        else if (dest[i] == 0) {
        strAppend:
            (char)dest[i] = src[add];
            if (!src[add])
                break;
            add++;
        }
        i++;
    }
}