#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <ShellScalingApi.h>

#include <cstdio>

static BOOL CALLBACK enum_monitor_proc(
    HMONITOR monitor,
    HDC,
    LPRECT,
    LPARAM)
{
    MONITORINFOEXA mi = {};
    mi.cbSize = sizeof(mi);

    if (!GetMonitorInfoA(monitor, &mi)) {
        std::printf("monitor: <unknown>\n");
        std::printf("  GetMonitorInfo failed: %lu\n", GetLastError());
        return TRUE;
    }

    UINT dpi_x = 0;
    UINT dpi_y = 0;

    HRESULT hr = GetDpiForMonitor(
        monitor,
        MDT_EFFECTIVE_DPI,
        &dpi_x,
        &dpi_y);

    std::printf("monitor: %s%s\n",
        mi.szDevice,
        mi.dwFlags & MONITORINFOF_PRIMARY ? " primary" : "");

    std::printf("  rect:    left=%ld top=%ld right=%ld bottom=%ld\n",
        mi.rcMonitor.left,
        mi.rcMonitor.top,
        mi.rcMonitor.right,
        mi.rcMonitor.bottom);

    std::printf("  work:    left=%ld top=%ld right=%ld bottom=%ld\n",
        mi.rcWork.left,
        mi.rcWork.top,
        mi.rcWork.right,
        mi.rcWork.bottom);

    if (SUCCEEDED(hr)) {
        std::printf("  dpi:     %u x %u\n", dpi_x, dpi_y);
        std::printf("  scale:   %.2f%%\n", dpi_x * 100.0 / 96.0);
    } else {
        std::printf("  GetDpiForMonitor failed: HRESULT 0x%08lx\n",
            static_cast<unsigned long>(hr));
    }

    std::printf("\n");
    return TRUE;
}

int main()
{
    HRESULT hr = SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
    if (FAILED(hr) && hr != E_ACCESSDENIED) {
        std::printf("SetProcessDpiAwareness failed: HRESULT 0x%08lx\n",
            static_cast<unsigned long>(hr));
    }

    if (!EnumDisplayMonitors(NULL, NULL, enum_monitor_proc, 0)) {
        std::printf("EnumDisplayMonitors failed: %lu\n", GetLastError());
        return 1;
    }

    return 0;
}
