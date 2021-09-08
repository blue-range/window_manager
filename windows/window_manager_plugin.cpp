#include "include/window_manager/window_manager_plugin.h"

// This must be included before many other Windows headers.
#include <windows.h>

#include <flutter/method_channel.h>
#include <flutter/plugin_registrar_windows.h>
#include <flutter/standard_method_codec.h>

#include <codecvt>
#include <map>
#include <memory>
#include <sstream>

namespace {
    std::unique_ptr<flutter::MethodChannel<flutter::EncodableValue>, std::default_delete<flutter::MethodChannel<flutter::EncodableValue>>> channel = nullptr;

    class WindowManagerPlugin : public flutter::Plugin {
    public:
        static void RegisterWithRegistrar(flutter::PluginRegistrarWindows* registrar);

        WindowManagerPlugin(flutter::PluginRegistrarWindows* registrar);

        virtual ~WindowManagerPlugin();

    private:
        flutter::PluginRegistrarWindows* registrar;

        bool g_is_maximized = false;
        bool g_is_minimized = false;

        bool g_is_window_fullscreen = false;
        // Initial window frame before going fullscreen & used for restoring window to
        // initial frame upon exiting fullscreen.
        RECT g_frame_before_fullscreen;

        // The ID of the WindowProc delegate registration.
        int window_proc_id = -1;

        // The minimum size set by the platform channel.
        POINT minimum_size = { 0, 0 };

        // The maximum size set by the platform channel.
        POINT maximum_size = { -1, -1 };

        // Called when a method is called on this plugin's channel from Dart.
        void HandleMethodCall(
            const flutter::MethodCall<flutter::EncodableValue>& method_call,
            std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);

        void WindowManagerPlugin::_EmitEvent(std::string eventName);
        HWND GetMainWindow();
        // Called for top-level WindowProc delegation.
        std::optional<LRESULT> WindowManagerPlugin::HandleWindowProc(HWND hwnd, UINT message,
            WPARAM wparam, LPARAM lparam);
        void WindowManagerPlugin::Focus(
            const flutter::MethodCall<flutter::EncodableValue>& method_call,
            std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);
        void WindowManagerPlugin::Blur(
            const flutter::MethodCall<flutter::EncodableValue>& method_call,
            std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);
        void WindowManagerPlugin::Show(
            const flutter::MethodCall<flutter::EncodableValue>& method_call,
            std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);
        void WindowManagerPlugin::Hide(
            const flutter::MethodCall<flutter::EncodableValue>& method_call,
            std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);
        void WindowManagerPlugin::IsVisible(
            const flutter::MethodCall<flutter::EncodableValue>& method_call,
            std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);
        void WindowManagerPlugin::IsMaximized(
            const flutter::MethodCall<flutter::EncodableValue>& method_call,
            std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);
        void WindowManagerPlugin::Maximize(
            const flutter::MethodCall<flutter::EncodableValue>& method_call,
            std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);
        void WindowManagerPlugin::Unmaximize(
            const flutter::MethodCall<flutter::EncodableValue>& method_call,
            std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);
        void WindowManagerPlugin::IsMinimized(
            const flutter::MethodCall<flutter::EncodableValue>& method_call,
            std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);
        void WindowManagerPlugin::Minimize(
            const flutter::MethodCall<flutter::EncodableValue>& method_call,
            std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);
        void WindowManagerPlugin::Restore(
            const flutter::MethodCall<flutter::EncodableValue>& method_call,
            std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);
        void WindowManagerPlugin::IsFullScreen(
            const flutter::MethodCall<flutter::EncodableValue>& method_call,
            std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);
        void WindowManagerPlugin::SetFullScreen(
            const flutter::MethodCall<flutter::EncodableValue>& method_call,
            std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);
        void WindowManagerPlugin::GetBounds(
            const flutter::MethodCall<flutter::EncodableValue>& method_call,
            std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);
        void WindowManagerPlugin::SetBounds(
            const flutter::MethodCall<flutter::EncodableValue>& method_call,
            std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);
        void WindowManagerPlugin::SetMinimumSize(
            const flutter::MethodCall<flutter::EncodableValue>& method_call,
            std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);
        void WindowManagerPlugin::SetMaximumSize(
            const flutter::MethodCall<flutter::EncodableValue>& method_call,
            std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);
        void WindowManagerPlugin::IsAlwaysOnTop(
            const flutter::MethodCall<flutter::EncodableValue>& method_call,
            std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);
        void WindowManagerPlugin::SetAlwaysOnTop(
            const flutter::MethodCall<flutter::EncodableValue>& method_call,
            std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);
        void WindowManagerPlugin::GetTitle(
            const flutter::MethodCall<flutter::EncodableValue>& method_call,
            std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);
        void WindowManagerPlugin::SetTitle(
            const flutter::MethodCall<flutter::EncodableValue>& method_call,
            std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);
        void WindowManagerPlugin::StartDragging(
            const flutter::MethodCall<flutter::EncodableValue>& method_call,
            std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);
        void WindowManagerPlugin::Terminate(
            const flutter::MethodCall<flutter::EncodableValue>& method_call,
            std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);
    };

    // static
    void WindowManagerPlugin::RegisterWithRegistrar(
        flutter::PluginRegistrarWindows* registrar) {
        channel =
            std::make_unique<flutter::MethodChannel<flutter::EncodableValue>>(
                registrar->messenger(), "window_manager",
                &flutter::StandardMethodCodec::GetInstance());

        auto plugin = std::make_unique<WindowManagerPlugin>(registrar);

        channel->SetMethodCallHandler(
            [plugin_pointer = plugin.get()](const auto& call, auto result)
        {
            plugin_pointer->HandleMethodCall(call, std::move(result));
        });

        registrar->AddPlugin(std::move(plugin));
    }


    WindowManagerPlugin::WindowManagerPlugin(flutter::PluginRegistrarWindows* registrar)
        : registrar(registrar) {
        window_proc_id = registrar->RegisterTopLevelWindowProcDelegate(
            [this](HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
                return HandleWindowProc(hwnd, message, wparam, lparam);
            });
    }

    WindowManagerPlugin::~WindowManagerPlugin() {
        registrar->UnregisterTopLevelWindowProcDelegate(window_proc_id);
    }

    void WindowManagerPlugin::_EmitEvent(std::string eventName) {
        flutter::EncodableMap args = flutter::EncodableMap();
            args[flutter::EncodableValue("eventName")] = flutter::EncodableValue(eventName);
        channel->InvokeMethod("onEvent", std::make_unique<flutter::EncodableValue>(args));
    }

    HWND WindowManagerPlugin::GetMainWindow() {
        return ::GetAncestor(registrar->GetView()->GetNativeWindow(), GA_ROOT);
    }

    std::optional<LRESULT> WindowManagerPlugin::HandleWindowProc(HWND hWnd,
        UINT message,
        WPARAM wParam,
        LPARAM lParam) {
        std::optional<LRESULT> result;
        if  (message == WM_GETMINMAXINFO) {
            MINMAXINFO* info = reinterpret_cast<MINMAXINFO*>(lParam);
            // For the special "unconstrained" values, leave the defaults.
            if (minimum_size.x != 0)
                info->ptMinTrackSize.x = minimum_size.x;
            if (minimum_size.y != 0)
                info->ptMinTrackSize.y = minimum_size.y;
            if (maximum_size.x != -1)
                info->ptMaxTrackSize.x = maximum_size.x;
            if (maximum_size.y != -1)
                info->ptMaxTrackSize.y = maximum_size.y;
            result = 0;
        } else if (message == WM_NCACTIVATE) {
            if (wParam == TRUE) {
                _EmitEvent("focus");
            } else {
                _EmitEvent("blur");
            }
        } else if (message == WM_SIZE) {
            if (wParam == SIZE_MAXIMIZED) {
                _EmitEvent("maximize");
                g_is_maximized = true;
                g_is_minimized = false;
            } else if (wParam == SIZE_MINIMIZED) {
                _EmitEvent("minimize");
                g_is_maximized = false;
                g_is_minimized = true;
            } else if (wParam == SIZE_RESTORED) {
                if (g_is_maximized) {
                    _EmitEvent("unmaximize");
                }
                if (g_is_minimized) {
                    _EmitEvent("restore");
                }
                g_is_maximized = false;
                g_is_minimized = false;
            }
        }
        return result;
    }

    void WindowManagerPlugin::Focus(
        const flutter::MethodCall<flutter::EncodableValue>& method_call,
        std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result) {

        result->Success(flutter::EncodableValue(true));
    }

    void WindowManagerPlugin::Blur(
        const flutter::MethodCall<flutter::EncodableValue>& method_call,
        std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result) {

        result->Success(flutter::EncodableValue(true));
    }

    void WindowManagerPlugin::Show(
        const flutter::MethodCall<flutter::EncodableValue>& method_call,
        std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result) {

        ShowWindowAsync(GetMainWindow(), SW_SHOW);
        SetForegroundWindow(GetMainWindow());

        result->Success(flutter::EncodableValue(true));
    }

    void WindowManagerPlugin::Hide(
        const flutter::MethodCall<flutter::EncodableValue>& method_call,
        std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result) {

        ShowWindow(GetMainWindow(), SW_HIDE);

        result->Success(flutter::EncodableValue(true));
    }

    void WindowManagerPlugin::IsVisible(
        const flutter::MethodCall<flutter::EncodableValue>& method_call,
        std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result) {

        bool isVisible = IsWindowVisible(GetMainWindow());

        result->Success(flutter::EncodableValue(isVisible));
    }

    void WindowManagerPlugin::IsMaximized(
        const flutter::MethodCall<flutter::EncodableValue>& method_call,
        std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result) {
        HWND mainWindow = GetMainWindow();
        WINDOWPLACEMENT windowPlacement;
        GetWindowPlacement(mainWindow, &windowPlacement);

        result->Success(flutter::EncodableValue(windowPlacement.showCmd == SW_MAXIMIZE));
    }

    void WindowManagerPlugin::Maximize(
        const flutter::MethodCall<flutter::EncodableValue>& method_call,
        std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result) {
        HWND mainWindow = GetMainWindow();
        WINDOWPLACEMENT windowPlacement;
        GetWindowPlacement(mainWindow, &windowPlacement);

        if (windowPlacement.showCmd != SW_MAXIMIZE) {
            windowPlacement.showCmd = SW_MAXIMIZE;
            SetWindowPlacement(mainWindow, &windowPlacement);
        }
        result->Success(flutter::EncodableValue(true));
    }

    void WindowManagerPlugin::Unmaximize(
        const flutter::MethodCall<flutter::EncodableValue>& method_call,
        std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result) {
        HWND mainWindow = GetMainWindow();
        WINDOWPLACEMENT windowPlacement;
        GetWindowPlacement(mainWindow, &windowPlacement);

        if (windowPlacement.showCmd != SW_NORMAL) {
            windowPlacement.showCmd = SW_NORMAL;
            SetWindowPlacement(mainWindow, &windowPlacement);
        }
        result->Success(flutter::EncodableValue(true));
    }

    void WindowManagerPlugin::IsMinimized(
        const flutter::MethodCall<flutter::EncodableValue>& method_call,
        std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result) {
        HWND mainWindow = GetMainWindow();
        WINDOWPLACEMENT windowPlacement;
        GetWindowPlacement(mainWindow, &windowPlacement);

        result->Success(flutter::EncodableValue(windowPlacement.showCmd == SW_SHOWMINIMIZED));
    }

    void WindowManagerPlugin::Minimize(
        const flutter::MethodCall<flutter::EncodableValue>& method_call,
        std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result) {
        HWND mainWindow = GetMainWindow();
        WINDOWPLACEMENT windowPlacement;
        GetWindowPlacement(mainWindow, &windowPlacement);

        if (windowPlacement.showCmd != SW_SHOWMINIMIZED) {
            windowPlacement.showCmd = SW_SHOWMINIMIZED;
            SetWindowPlacement(mainWindow, &windowPlacement);
        }
        result->Success(flutter::EncodableValue(true));
    }

    void WindowManagerPlugin::Restore(
        const flutter::MethodCall<flutter::EncodableValue>& method_call,
        std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result) {
        HWND mainWindow = GetMainWindow();
        WINDOWPLACEMENT windowPlacement;
        GetWindowPlacement(mainWindow, &windowPlacement);

        if (windowPlacement.showCmd != SW_NORMAL) {
            windowPlacement.showCmd = SW_NORMAL;
            SetWindowPlacement(mainWindow, &windowPlacement);
        }
        result->Success(flutter::EncodableValue(true));
    }

    void WindowManagerPlugin::IsFullScreen(
        const flutter::MethodCall<flutter::EncodableValue>& method_call,
        std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result) {

        result->Success(flutter::EncodableValue(g_is_window_fullscreen));
    }

    void WindowManagerPlugin::SetFullScreen(
        const flutter::MethodCall<flutter::EncodableValue>& method_call,
        std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result) {
        const flutter::EncodableMap& args = std::get<flutter::EncodableMap>(*method_call.arguments());

        bool isFullScreen = std::get<bool>(args.at(flutter::EncodableValue("isFullScreen")));

        HWND mainWindow = GetMainWindow();
        WINDOWPLACEMENT windowPlacement;
        GetWindowPlacement(mainWindow, &windowPlacement);

        // https://github.com/alexmercerind/flutter-desktop-embedding/blob/da98a3b5a0e2b9425fbcb2a3e4b4ba50754abf93/plugins/window_size/windows/window_size_plugin.cpp#L258
        if (isFullScreen) {
            HMONITOR monitor = ::MonitorFromWindow(mainWindow, MONITOR_DEFAULTTONEAREST);
            MONITORINFO info;
            info.cbSize = sizeof(MONITORINFO);
            ::GetMonitorInfo(monitor, &info);
            ::SetWindowLongPtr(mainWindow, GWL_STYLE, WS_POPUP | WS_VISIBLE);
            ::GetWindowRect(mainWindow, &g_frame_before_fullscreen);
            ::SetWindowPos(
                mainWindow, NULL, info.rcMonitor.left, info.rcMonitor.top,
                info.rcMonitor.right - info.rcMonitor.left,
                info.rcMonitor.bottom - info.rcMonitor.top, SWP_SHOWWINDOW);
            ::ShowWindow(mainWindow, SW_MAXIMIZE);
            _EmitEvent("enter-full-screen");
        }
        else {
            g_is_window_fullscreen = false;
            ::SetWindowLongPtr(mainWindow, GWL_STYLE, WS_OVERLAPPEDWINDOW | WS_VISIBLE);
            ::SetWindowPos(
                mainWindow, NULL, g_frame_before_fullscreen.left,
                g_frame_before_fullscreen.top,
                g_frame_before_fullscreen.right - g_frame_before_fullscreen.left,
                g_frame_before_fullscreen.bottom - g_frame_before_fullscreen.top,
                SWP_SHOWWINDOW);
            ::ShowWindow(mainWindow, SW_RESTORE);
            _EmitEvent("leave-full-screen");
        }

        result->Success(flutter::EncodableValue(true));
    }

    void WindowManagerPlugin::GetBounds(
        const flutter::MethodCall<flutter::EncodableValue>& method_call,
        std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result) {
        const flutter::EncodableMap& args = std::get<flutter::EncodableMap>(*method_call.arguments());

        double devicePixelRatio = std::get<double>(args.at(flutter::EncodableValue("devicePixelRatio")));

        flutter::EncodableMap resultMap = flutter::EncodableMap();
        RECT rect;
        if (GetWindowRect(GetMainWindow(), &rect)) {
            double x = rect.left / devicePixelRatio * 1.0f;
            double y = rect.top / devicePixelRatio * 1.0f;
            double width = (rect.right - rect.left) / devicePixelRatio * 1.0f;
            double height = (rect.bottom - rect.top) / devicePixelRatio * 1.0f;

            resultMap[flutter::EncodableValue("x")] = flutter::EncodableValue(x);
            resultMap[flutter::EncodableValue("y")] = flutter::EncodableValue(y);
            resultMap[flutter::EncodableValue("width")] = flutter::EncodableValue(width);
            resultMap[flutter::EncodableValue("height")] = flutter::EncodableValue(height);
        }
        result->Success(flutter::EncodableValue(resultMap));
    }

    void WindowManagerPlugin::SetBounds(
        const flutter::MethodCall<flutter::EncodableValue>& method_call,
        std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result) {
        const flutter::EncodableMap& args = std::get<flutter::EncodableMap>(*method_call.arguments());

        double devicePixelRatio = std::get<double>(args.at(flutter::EncodableValue("devicePixelRatio")));
        double x = std::get<double>(args.at(flutter::EncodableValue("x")));
        double y = std::get<double>(args.at(flutter::EncodableValue("y")));
        double width = std::get<double>(args.at(flutter::EncodableValue("width")));
        double height = std::get<double>(args.at(flutter::EncodableValue("height")));

        SetWindowPos(
            GetMainWindow(),
            HWND_TOP,
            int(x * devicePixelRatio),
            int(y * devicePixelRatio),
            int(width * devicePixelRatio),
            int(height * devicePixelRatio),
            SWP_SHOWWINDOW);

        result->Success(flutter::EncodableValue(true));
    }

    void WindowManagerPlugin::SetMinimumSize(
        const flutter::MethodCall<flutter::EncodableValue>& method_call,
        std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result) {
        const flutter::EncodableMap& args = std::get<flutter::EncodableMap>(*method_call.arguments());

        double devicePixelRatio = std::get<double>(args.at(flutter::EncodableValue("devicePixelRatio")));
        double width = std::get<double>(args.at(flutter::EncodableValue("width")));
        double height = std::get<double>(args.at(flutter::EncodableValue("height")));

        if (width >= 0 && height >= 0) {
            POINT point = {};
            point.x = static_cast<LONG>(width * devicePixelRatio);
            point.y = static_cast<LONG>(height * devicePixelRatio);
            minimum_size = point;
        }
        result->Success(flutter::EncodableValue(true));
    }

    void WindowManagerPlugin::SetMaximumSize(
        const flutter::MethodCall<flutter::EncodableValue>& method_call,
        std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result) {
        const flutter::EncodableMap& args = std::get<flutter::EncodableMap>(*method_call.arguments());

        double devicePixelRatio = std::get<double>(args.at(flutter::EncodableValue("devicePixelRatio")));
        double width = std::get<double>(args.at(flutter::EncodableValue("width")));
        double height = std::get<double>(args.at(flutter::EncodableValue("height")));

        if (width >= 0 && height >= 0) {
            POINT point = {};
            point.x = static_cast<LONG>(width * devicePixelRatio);
            point.y = static_cast<LONG>(height * devicePixelRatio);
            maximum_size = point;
        }
        result->Success(flutter::EncodableValue(true));
    }

    void WindowManagerPlugin::IsAlwaysOnTop(
        const flutter::MethodCall<flutter::EncodableValue>& method_call,
        std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result) {

        DWORD dwExStyle = GetWindowLong(GetMainWindow(), GWL_EXSTYLE);

        result->Success(flutter::EncodableValue((dwExStyle & WS_EX_TOPMOST) != 0));
    }

    void WindowManagerPlugin::SetAlwaysOnTop(
        const flutter::MethodCall<flutter::EncodableValue>& method_call,
        std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result) {
        const flutter::EncodableMap& args = std::get<flutter::EncodableMap>(*method_call.arguments());

        bool isAlwaysOnTop = std::get<bool>(args.at(flutter::EncodableValue("isAlwaysOnTop")));

        SetWindowPos(GetMainWindow(), isAlwaysOnTop ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

        result->Success(flutter::EncodableValue(true));
    }
    
    void WindowManagerPlugin::GetTitle(
        const flutter::MethodCall<flutter::EncodableValue>& method_call,
        std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result) {
       
        int const bufferSize = 1 + GetWindowTextLength(GetMainWindow());
        std::wstring title( bufferSize, L'\0' );
        GetWindowText(GetMainWindow(), &title[0], bufferSize);

        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        result->Success(flutter::EncodableValue(converter.to_bytes(title)));
    }
    
    void WindowManagerPlugin::SetTitle(
        const flutter::MethodCall<flutter::EncodableValue>& method_call,
        std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result) {
        const flutter::EncodableMap& args = std::get<flutter::EncodableMap>(*method_call.arguments());

        std::string title = std::get<std::string>(args.at(flutter::EncodableValue("title")));

        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        SetWindowText(GetMainWindow(), converter.from_bytes(title).c_str());

        result->Success(flutter::EncodableValue(true));
    }

    void WindowManagerPlugin::StartDragging(
        const flutter::MethodCall<flutter::EncodableValue>& method_call,
        std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result) {

        ReleaseCapture();
        SendMessage(GetMainWindow(), WM_SYSCOMMAND, SC_MOVE | HTCAPTION, 0);

        result->Success(flutter::EncodableValue(true));
    }

    void WindowManagerPlugin::Terminate(
        const flutter::MethodCall<flutter::EncodableValue>& method_call,
        std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result) {
        ExitProcess(1);
    }

    void WindowManagerPlugin::HandleMethodCall(
        const flutter::MethodCall<flutter::EncodableValue>& method_call,
        std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result) {
        if (method_call.method_name().compare("show") == 0) {
            Show(method_call, std::move(result));
        }
        else if (method_call.method_name().compare("hide") == 0) {
            Hide(method_call, std::move(result));
        }
        else if (method_call.method_name().compare("isVisible") == 0) {
            IsVisible(method_call, std::move(result));
        }
        else if (method_call.method_name().compare("isMaximized") == 0) {
            IsMaximized(method_call, std::move(result));
        }
        else if (method_call.method_name().compare("maximize") == 0) {
            Maximize(method_call, std::move(result));
        }
        else if (method_call.method_name().compare("unmaximize") == 0) {
            Unmaximize(method_call, std::move(result));
        }
        else if (method_call.method_name().compare("isMinimized") == 0) {
            IsMinimized(method_call, std::move(result));
        }
        else if (method_call.method_name().compare("minimize") == 0) {
            Minimize(method_call, std::move(result));
        }
        else if (method_call.method_name().compare("restore") == 0) {
            Restore(method_call, std::move(result));
        }
        else if (method_call.method_name().compare("isFullScreen") == 0) {
            IsFullScreen(method_call, std::move(result));
        }
        else if (method_call.method_name().compare("setFullScreen") == 0) {
            SetFullScreen(method_call, std::move(result));
        }
        else if (method_call.method_name().compare("getBounds") == 0) {
            GetBounds(method_call, std::move(result));
        }
        else if (method_call.method_name().compare("setBounds") == 0) {
            SetBounds(method_call, std::move(result));
        }
        else if (method_call.method_name().compare("setMinimumSize") == 0) {
            SetMinimumSize(method_call, std::move(result));
        }
        else if (method_call.method_name().compare("setMaximumSize") == 0) {
            SetMaximumSize(method_call, std::move(result));
        }
        else if (method_call.method_name().compare("isAlwaysOnTop") == 0) {
            IsAlwaysOnTop(method_call, std::move(result));
        }
        else if (method_call.method_name().compare("setAlwaysOnTop") == 0) {
            SetAlwaysOnTop(method_call, std::move(result));
        }
        else if (method_call.method_name().compare("getTitle") == 0) {
            GetTitle(method_call, std::move(result));
        }
        else if (method_call.method_name().compare("setTitle") == 0) {
            SetTitle(method_call, std::move(result));
        }
        else if (method_call.method_name().compare("startDragging") == 0) {
            StartDragging(method_call, std::move(result));
        }
        else if (method_call.method_name().compare("terminate") == 0) {
            Terminate(method_call, std::move(result));
        }
        else {
            result->NotImplemented();
        }
    }

} // namespace

void WindowManagerPluginRegisterWithRegistrar(
    FlutterDesktopPluginRegistrarRef registrar) {
    WindowManagerPlugin::RegisterWithRegistrar(
        flutter::PluginRegistrarManager::GetInstance()
        ->GetRegistrar<flutter::PluginRegistrarWindows>(registrar));
}
