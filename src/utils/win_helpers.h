#pragma once
#include <windows.h>
#include <utility>
#include <stdexcept>
#include <string>

namespace utils {

//-----------------------------------------------------------------------------
// Generic handle RAII wrapper
// Usage: unique_handle<HANDLE, CloseHandle> hFile(CreateFile(...));
//-----------------------------------------------------------------------------
template<typename T, auto Deleter, T NullValue = nullptr>
class unique_handle {
public:
    explicit unique_handle(T h = NullValue) noexcept : handle_(h) {}

    unique_handle(const unique_handle&) = delete;
    unique_handle& operator=(const unique_handle&) = delete;

    unique_handle(unique_handle&& other) noexcept
        : handle_(std::exchange(other.handle_, NullValue)) {}

    unique_handle& operator=(unique_handle&& other) noexcept {
        if (this != &other) {
            reset();
            handle_ = std::exchange(other.handle_, NullValue);
        }
        return *this;
    }

    ~unique_handle() { reset(); }

    void reset(T h = NullValue) noexcept {
        if (handle_ != NullValue) Deleter(handle_);
        handle_ = h;
    }

    [[nodiscard]] T get() const noexcept { return handle_; }
    [[nodiscard]] T* ptr() noexcept { return &handle_; }
    [[nodiscard]] explicit operator bool() const noexcept { return handle_ != NullValue; }

    T release() noexcept { return std::exchange(handle_, NullValue); }

private:
    T handle_;
};

// Specialization for HANDLE (invalid value is INVALID_HANDLE_VALUE for file handles)
struct file_handle_deleter {
    void operator()(HANDLE h) const { if (h != INVALID_HANDLE_VALUE) CloseHandle(h); }
};

using file_handle   = unique_handle<HANDLE, CloseHandle>;
using event_handle  = unique_handle<HANDLE, CloseHandle>;
using mutex_handle  = unique_handle<HANDLE, CloseHandle>;
using thread_handle = unique_handle<HANDLE, CloseHandle>;

//-----------------------------------------------------------------------------
// GDI object wrappers
//-----------------------------------------------------------------------------
struct gdi_deleter { void operator()(HGDIOBJ obj) const { DeleteObject(obj); } };

using hfont_owner   = unique_handle<HFONT,   DeleteObject>;
using hbrush_owner  = unique_handle<HBRUSH,  DeleteObject>;
using hpen_owner    = unique_handle<HPEN,    DeleteObject>;
using hbitmap_owner = unique_handle<HBITMAP, DeleteObject>;
using hdc_owner     = unique_handle<HDC,     DeleteDC>;

//-----------------------------------------------------------------------------
// DC Save/Restore scope guard
//-----------------------------------------------------------------------------
class dc_state_guard {
public:
    explicit dc_state_guard(HDC hdc) : hdc_(hdc), save_(SaveDC(hdc)) {}
    ~dc_state_guard() { RestoreDC(hdc_, save_); }
    dc_state_guard(const dc_state_guard&) = delete;
    dc_state_guard& operator=(const dc_state_guard&) = delete;
private:
    HDC hdc_;
    int save_;
};

//-----------------------------------------------------------------------------
// Memory DC with optional bitmap
//-----------------------------------------------------------------------------
class memory_dc {
public:
    explicit memory_dc(HDC compatible_dc) {
        hdc_.reset(CreateCompatibleDC(compatible_dc));
    }

    memory_dc(HDC compatible_dc, int w, int h) {
        hdc_.reset(CreateCompatibleDC(compatible_dc));
        hbm_.reset(CreateCompatibleBitmap(compatible_dc, w, h));
        old_bm_ = SelectObject(hdc_.get(), hbm_.get());
    }

    ~memory_dc() {
        if (hdc_ && old_bm_) SelectObject(hdc_.get(), old_bm_);
    }

    HDC get() const { return hdc_.get(); }
    HBITMAP bitmap() const { return hbm_.get(); }
    explicit operator bool() const { return static_cast<bool>(hdc_); }

private:
    hdc_owner    hdc_;
    hbitmap_owner hbm_;
    HGDIOBJ      old_bm_{ nullptr };
};

//-----------------------------------------------------------------------------
// DIB section for layered window rendering
//-----------------------------------------------------------------------------
class dib_section {
public:
    dib_section() = default;

    bool create(HDC screen_dc, int w, int h) {
        width_ = w; height_ = h;
        BITMAPINFOHEADER bmi{};
        bmi.biSize        = sizeof(bmi);
        bmi.biWidth       = w;
        bmi.biHeight      = -h; // top-down
        bmi.biPlanes      = 1;
        bmi.biBitCount    = 32;
        bmi.biCompression = BI_RGB;

        hbm_.reset(CreateDIBSection(screen_dc,
            reinterpret_cast<BITMAPINFO*>(&bmi),
            DIB_RGB_COLORS, &bits_, nullptr, 0));
        return static_cast<bool>(hbm_);
    }

    void clear() {
        if (bits_) memset(bits_, 0, width_ * height_ * 4);
    }

    HBITMAP bitmap() const { return hbm_.get(); }
    void*   bits()   const { return bits_; }
    int     width()  const { return width_; }
    int     height() const { return height_; }
    explicit operator bool() const { return static_cast<bool>(hbm_); }

private:
    hbitmap_owner hbm_;
    void*         bits_{ nullptr };
    int           width_{ 0 };
    int           height_{ 0 };
};

//-----------------------------------------------------------------------------
// Helpers
//-----------------------------------------------------------------------------
inline std::wstring get_last_error_string(DWORD err = GetLastError()) {
    wchar_t* buf = nullptr;
    FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                   nullptr, err, 0, reinterpret_cast<LPWSTR>(&buf), 0, nullptr);
    std::wstring msg = buf ? buf : L"Unknown error";
    LocalFree(buf);
    return msg;
}

inline POINT cursor_pos() {
    POINT pt{};
    GetCursorPos(&pt);
    return pt;
}

} // namespace utils
