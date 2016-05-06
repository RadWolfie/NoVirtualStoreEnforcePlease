#pragma once

extern _Bool isDirFileFullPermission(const wchar_t* str, unsigned int* result);

extern HMODULE GetCurrentModule();

extern void strcatW(wchar_t *dest, size_t len, const wchar_t *src);
extern void strcatA(char *dest, size_t len, const char *src);