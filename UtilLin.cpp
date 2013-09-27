#include "Util.h"

#include <string.h>
#include <sstream>
#include <stdexcept>

#include <unistd.h>
#include <iconv.h>
#include <errno.h>


int Ut::last_error()
{
    return errno;
}


std::string Ut::strerror(int err_code)
{
    char buf[256];
    char* msg = strerror_r(err_code, buf, 256);
    return std::string(msg);
}


std::string Ut::convert(const std::wstring& str, unsigned int /*codePage = CP_UTF8*/)
{
    return convert(str.c_str());
}


std::wstring Ut::convert(const std::string& str, unsigned int /*codePage = CP_UTF8*/)
{
    return convert(str.c_str());
}


std::string Ut::convert(const wchar_t* /*str*/, unsigned int /*codePage = CP_UTF8*/)
{
    assert(0 && "Should not be used on Linux");
    return "";
}


std::string Ut::convert(const uint16_t* str, unsigned int /*codePage = CP_UTF8*/)
{
    size_t outleft = 0;
    size_t converted = 0;
    char* output = nullptr;
    char* outbuf = nullptr;
    char* tmp = nullptr;
    iconv_t cd;
    
    const char* to_charset = "UTF-8";
    const char* from_charset = "UTF-16";
    
    if ((cd = iconv_open(to_charset, from_charset)) == (iconv_t)-1)
        return "";
    
    size_t inleft = 0;
    const uint16_t* ptr = str;
    while (*ptr++ != 0)
        ++inleft;
    
    const char* inbuf = reinterpret_cast<const char*>(str);
    size_t outlen = inleft * 2;

    /* we allocate 4 bytes more than what we need for nul-termination... */
    if (!(output = (char*)malloc(outlen + 4)))
    {
        iconv_close(cd);
        return "";
    }

    do
    {
        errno = 0;
        outbuf = output + converted;
        outleft = outlen - converted;

        converted = iconv(cd, (char **)&inbuf, &inleft, &outbuf, &outleft);
        if (converted != (size_t)-1 || errno == EINVAL)
        {
            /*
             * EINVAL  An  incomplete  multibyte sequence has been encoun­-
             *         tered in the input.
             *
             * We'll just truncate it and ignore it.
             */
            break;
        }

        if (errno != E2BIG)
        {
            /*
             * EILSEQ An invalid multibyte sequence has been  encountered
             *        in the input.
             *
             * Bad input, we can't really recover from this. 
             */
            iconv_close(cd);
            free(output);
            return "";
        }

        /*
         * E2BIG   There is not sufficient room at *outbuf.
         *
         * We just need to grow our outbuffer and try again.
         */

        converted = outbuf - output;
        outlen += inleft * 2 + 8;

        if (!(tmp = (char*)realloc(output, outlen + 4)))
        {
            iconv_close(cd);
            free(output);
            return "";
        }

        output = tmp;
        outbuf = output + converted;
        
    } while (1);

    /* flush the iconv conversion */
    iconv(cd, NULL, NULL, &outbuf, &outleft);
    iconv_close(cd);

    /* Note: not all charsets can be nul-terminated with a single
     * nul byte. UCS2, for example, needs 2 nul bytes and UCS4
     * needs 4. I hope that 4 nul bytes is enough to terminate all
     * multibyte charsets? */

    /* nul-terminate the string */
    memset(outbuf, 0, 4);

    return output;
}


std::wstring Ut::convert(const char* /*str*/, unsigned int /*codePage = CP_UTF8*/)
{
    assert(0 && "Should not be used on Linux");
    return L"";
}


std::string Ut::executable_path()
{
    const ssize_t size = 512;
    char buffer[size];
    ssize_t ret = readlink("/proc/self/exe", buffer, size);

    if (ret == -1)
        Ut::create_system_exception("Failed to get current executable path");
    else
        buffer[(ret < size ? ret : size - 1)] = '\0';

    *(strrchr(buffer, '/') + 1) = '\0'; // Remove executable name
    return std::string(buffer);
}
