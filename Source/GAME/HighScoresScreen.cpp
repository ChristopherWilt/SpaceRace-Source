#include "HighScoresManager.h"
#include "../DRAW/DrawComponents.h"
#include "../DRAW/Utility/TextMeshBuilder.h"
#include "../UTIL/Utilities.h"
#include "../APP/Window.hpp"
#include <gdiplus.h>
#include <string>

namespace GAME
{
    void DrawHighScores(entt::registry& registry)
    {
#ifdef _WIN32
        auto winView = registry.view<APP::Window>();
        if (winView.empty()) return;

        auto& win = registry.get<GW::SYSTEM::GWindow>(*winView.begin());

        GW::SYSTEM::UNIVERSAL_WINDOW_HANDLE uwh{};
        if (!+ win.GetWindowHandle(uwh))
            return;

        HWND hwnd = reinterpret_cast<HWND>(uwh.window);
        if (!hwnd)
            return;

        HDC hdc = GetDC(hwnd);
        if (!hdc) return;

        RECT rc; GetClientRect(hwnd, &rc);
        const int W = rc.right - rc.left;
        const int H = rc.bottom - rc.top;

        HDC memDC = CreateCompatibleDC(hdc);
        HBITMAP memBMP = CreateCompatibleBitmap(hdc, W, H);
        HGDIOBJ oldBMP = SelectObject(memDC, memBMP);

        // background fill
        HBRUSH bg = (HBRUSH)GetStockObject(BLACK_BRUSH);
        FillRect(memDC, &rc, bg);

        Gdiplus::Graphics g(memDC);
        g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
        Gdiplus::SolidBrush whiteBrush(Gdiplus::Color(255, 255, 255, 255));
        Gdiplus::FontFamily fontFamily(L"Consolas");
        Gdiplus::Font font(&fontFamily, 32, Gdiplus::FontStyleBold, Gdiplus::UnitPixel);
        Gdiplus::StringFormat format;
        format.SetAlignment(Gdiplus::StringAlignmentCenter);

        // title
        g.DrawString(L"HIGH SCORES", -1, &font, Gdiplus::PointF(W / 2.0f, 60.0f), &format, &whiteBrush);

        // access scores
        auto& mgr = registry.ctx().get<GAME::HighScoreManager>();
        float y = 140.f;
        int rank = 1;

        Gdiplus::Font entryFont(&fontFamily, 24, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);

        for (auto& s : mgr.entries)
        {
            std::wstring entry = std::to_wstring(rank) + L". " +
                std::wstring(s.name.begin(), s.name.end()) +
                L" - " + std::to_wstring(s.scores);
            g.DrawString(entry.c_str(), -1, &entryFont,
                Gdiplus::PointF(W / 2.0f, y), &format, &whiteBrush);
            y += 40.f;
            rank++;
            if (rank > 10) break;
        }

        std::wstring msg = L"Press BACKSPACE to return";
        g.DrawString(msg.c_str(), -1, &entryFont,
            Gdiplus::PointF(W / 2.0f, (float)H - 80.0f), &format, &whiteBrush);

        BitBlt(hdc, 0, 0, W, H, memDC, 0, 0, SRCCOPY);
        SelectObject(memDC, oldBMP);
        DeleteObject(memBMP);
        DeleteDC(memDC);
        ReleaseDC(hwnd, hdc);
#endif
    }
}