#include "CrashSuppressor.h"

PVOID CrashSuppressor::s_vectoredHandler = nullptr;

void CrashSuppressor::Install() {
    if (!s_vectoredHandler) {
        s_vectoredHandler = AddVectoredExceptionHandler(1, ExceptionHandler);
    }
}

void CrashSuppressor::Remove() {
    if (s_vectoredHandler) {
        RemoveVectoredExceptionHandler(s_vectoredHandler);
        s_vectoredHandler = nullptr;
    }
}

bool CrashSuppressor::IsInstalled() {
    return s_vectoredHandler != nullptr;
}

LONG NTAPI CrashSuppressor::ExceptionHandler(PEXCEPTION_POINTERS ExceptionInfo) {
    // Suppress all crashes by continuing execution
    return EXCEPTION_CONTINUE_EXECUTION;
}