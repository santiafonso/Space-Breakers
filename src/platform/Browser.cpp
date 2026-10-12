#include "platform/Browser.hpp"

#include <cstdlib>

#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#endif

namespace sb {

bool openUrl(const std::string& url) {
#ifdef _WIN32
    return reinterpret_cast<INT_PTR>(ShellExecuteA(nullptr, "open", url.c_str(), nullptr, nullptr, SW_SHOWNORMAL)) > 32;
#elif defined(__APPLE__)
    return std::system(("open '" + url + "' >/dev/null 2>&1 &").c_str()) == 0;
#else
    return std::system(("xdg-open '" + url + "' >/dev/null 2>&1 &").c_str()) == 0;
#endif
}

}  // namespace sb
