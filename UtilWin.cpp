#include "Util.h"

#include <sstream>

#include <Windows.h>


int Ut::last_error()
{
    return GetLastError();
}


std::string Ut::strerror(int err_code)
{
    std::string msg;
    LPWSTR errorText = NULL;

    FormatMessageW(
        // use system message tables to retrieve error text
        FORMAT_MESSAGE_FROM_SYSTEM
        // allocate buffer on local heap for error text
        | FORMAT_MESSAGE_ALLOCATE_BUFFER
        // Important! will fail otherwise, since we're not
        // (and CANNOT) pass insertion parameters
        | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,                      // unused with FORMAT_MESSAGE_FROM_SYSTEM
        err_code,
        MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),
        (LPWSTR)&errorText,        // output
        0,                         // minimum size for output buffer
        NULL );

    if (NULL != errorText)
    {
        msg = Ut::convert(errorText);

        // release memory allocated by FormatMessage()
        LocalFree(errorText);
    }
    else
    {
        std::stringstream ss;
        ss << err_code;
        msg = ss.str();
    }

    return msg;
}


#ifdef VT_USE_COM

#include <comdef.h>

std::string Ut::com_error(long hresult)
{
    _com_error err(hresult);
    LPCTSTR errMsg = err.ErrorMessage();
    return Ut::convert(errMsg);
}

#endif

std::string Ut::convert(const std::wstring& str, unsigned int /*codePage = CP_UTF8*/)
{
    return convert(str.c_str());
}


std::wstring Ut::convert(const std::string& str, unsigned int /*codePage = CP_UTF8*/)
{
    return convert(str.c_str());
}


std::string Ut::convert(const wchar_t* str, unsigned int /*codePage = CP_UTF8*/)
{
    // FIXME: implement proper conversion to other code pages from wchar_t

    int size = WideCharToMultiByte(CP_UTF8, 0, str, -1, nullptr, 0, nullptr, nullptr);
    std::string buf(size, '\0');  // size includes final '\0'

    if (0 == WideCharToMultiByte(CP_UTF8, 0, str, -1, &buf[0], size, nullptr, nullptr))
    {
        throw std::runtime_error(strerror(GetLastError()));
    }

    return buf;
}


std::wstring Ut::convert(const char* str, unsigned int /*codePage = CP_UTF8*/)
{
    // FIXME: implement proper conversion from other code pages

    int size = MultiByteToWideChar(CP_UTF8, 0, str, -1, nullptr, 0);
    std::wstring buf(size, '\0');  // size includes final '\0'

    if (0 == MultiByteToWideChar(CP_UTF8, 0, str, -1, &buf[0], size))
    {
        throw std::runtime_error(strerror(GetLastError()));
    }

    return buf;
}


std::string Ut::executable_path()
{
    const size_t size = 512;
    char filename[size];
    auto ret = GetModuleFileNameA(0, filename, size);
    if (ret == 0)
    {
        Ut::create_system_exception("Failed to get current executable path");
    }
    *(strrchr(filename, '\\') + 1) = '\0'; // Remove exe name
    return std::string(filename);
}


/*
std::string convert(const CString& mfcString)
{
#ifdef UNICODE
    return convert(std::wstring(mfcString.GetString()));
#else
    return mfcString.GetString();   // This branch is deprecated.
#endif
}

CString convert(const std::string& s)
{
#ifdef UNICODE
    return CString(convert(s).c_str());
#else
    Exceptions::Assert(false, "Unicode policy violation. See W569"); // This branch is deprecated as it does not support unicode
    return s.c_str();   
#endif
}
*/
