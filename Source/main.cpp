// main entry point for the application
// enables components to define their behaviors locally in an .hpp file
#include "CCL.h"
#include "UTIL/Utilities.h"
// include all components, tags, and systems used by this program
#include "DRAW/DrawComponents.h"
#include "GAME/GameComponents.h"
#include "GAME/HighScoresManager.h"
#include "GAME/GameAudio.h"
#include "APP/Window.hpp"
#include <filesystem>
#include <random>

namespace GAME
{
	void ResetGame(entt::registry& registry);  // Function declaration
}


#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#undef min
#undef max
#include <gdiplus.h>
#pragma comment (lib, "Gdiplus.lib")

static std::wstring FindAsset(const wchar_t* file)
{
	wchar_t exePath[MAX_PATH];
	GetModuleFileNameW(nullptr, exePath, MAX_PATH);
	std::filesystem::path p = std::filesystem::path(exePath).parent_path();

	for (int up = 0; up <= 6; ++up) {
		auto candidate = p / L"assets" / L"ui" / file;
		if (GetFileAttributesW(candidate.c_str()) != INVALID_FILE_ATTRIBUTES)
			return candidate.wstring();
		p = p.parent_path();
	}
	return std::wstring(L"assets\\ui\\") + file;
}

namespace {
	struct MenuState {
		int selected = 0;
		RECT rects[6] = {};
		bool mouseDownLast = false;
		bool gdiReady = false;
		ULONG_PTR gdiToken = 0;
		//images
		Gdiplus::Image* bg = nullptr;
		Gdiplus::Image* logo = nullptr;
		Gdiplus::Image* ship = nullptr;
		Gdiplus::Image* buttonImgs[6] = { nullptr };
	};

	static MenuState g_menu;

	struct SettingsUIState {
		RECT sliderRects[3] = {};
	};
	static SettingsUIState g_settingsUI;
}

//Pause Menu
struct PauseUIState {
	RECT buttonRects[5] = {}; //Resume, Restart, Main Menu, Settings, Quit
	RECT yesRect = {};
	RECT noRect = {};
};
static PauseUIState g_pauseUI;

struct HowToPlayUIState {
	RECT nextRect = {}; // right arrow (page 2)
	RECT prevRect = {}; // left arrow  (page 1)
};
static HowToPlayUIState g_htpUI;

#endif

static void DrawStartOverlay(GW::SYSTEM::GWindow& gw, const char* title, const char* prompt)
{
#ifdef _WIN32
	using namespace Gdiplus;

	//initialize GDI+ one time
	if (!g_menu.gdiReady) {
		GdiplusStartupInput si;
		if (GdiplusStartup(&g_menu.gdiToken, &si, nullptr) == Ok) {
			g_menu.gdiReady = true;

			g_menu.bg = new Image(FindAsset(L"Background.png").c_str());
			g_menu.logo = new Image(FindAsset(L"Logo.png").c_str());
			g_menu.ship = new Image(FindAsset(L"Ship.png").c_str());

			g_menu.buttonImgs[0] = new Image(FindAsset(L"StartGame.png").c_str());
			g_menu.buttonImgs[1] = new Image(FindAsset(L"Settings.png").c_str());
			g_menu.buttonImgs[2] = new Image(FindAsset(L"HowToPlay.png").c_str());
			g_menu.buttonImgs[3] = new Image(FindAsset(L"HighScores.png").c_str());
			g_menu.buttonImgs[4] = new Image(FindAsset(L"Credits.png").c_str());
			g_menu.buttonImgs[5] = new Image(FindAsset(L"Quit.png").c_str());
		}
	}
	GW::SYSTEM::UNIVERSAL_WINDOW_HANDLE uwh{};
	if (!+ gw.GetWindowHandle(uwh))
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


	HBRUSH bg = (HBRUSH)GetStockObject(BLACK_BRUSH);
	FillRect(memDC, &rc, bg);


	Gdiplus::Graphics g(memDC);
	g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
	g.SetSmoothingMode(Gdiplus::SmoothingModeNone);
	g.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);




	//Menu Layout

	const int   kBtnH = 100;
	const int   kBtnGap = 0.20;
	const float kBtnWidthPct = 0.10;
	const float kMenuTopPct = 30.0f;
	const float kLogoMaxWPct = 0.35f;
	const float kLogoMaxHPct = 0.20f;
	const int   kLogoGap = 20;
	const int   kShipGap = 8;

	// background cover
	if (g_menu.bg && g_menu.bg->GetLastStatus() == Ok) {
		const int imgW = g_menu.bg->GetWidth(), imgH = g_menu.bg->GetHeight();
		const double s = std::max((double)W / imgW, (double)H / imgH);
		const int dw = (int)(imgW * s), dh = (int)(imgH * s);
		const int dx = (W - dw) / 2, dy = (H - dh) / 2;
		g.DrawImage(g_menu.bg, dx, dy, dw, dh);
	}

	HFONT font = (HFONT)CreateFontA(
		64, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
		ANSI_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
		DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
	HFONT old = (HFONT)SelectObject(memDC, font);
	SetBkMode(memDC, TRANSPARENT);
	SetTextColor(memDC, RGB(255, 255, 255));
	RECT rTitle = rc; rTitle.top = H / 12;
	DrawTextA(memDC, title, -1, &rTitle, DT_CENTER | DT_TOP | DT_SINGLELINE);
	SelectObject(memDC, old);
	DeleteObject(font);


	// menu geometry
	const int count = 6;
	int btnH = kBtnH;
	int gap = kBtnGap;
	const int btnW = (int)(W * kBtnWidthPct);
	int x = (W - btnW) / 2;

	const int topMargin = H / 5;
	const int bottomMargin = H / 10;

	int totalH = count * (btnH + gap) - gap;
	int availH = std::max(1, H - topMargin - bottomMargin);

	if (totalH > availH) {
		float s = (float)availH / (float)totalH;
		btnH = std::max(40, (int)(btnH * s));
		gap = std::max(4, (int)(gap * s));
		totalH = count * (btnH + gap) - gap;
	}
	int startY = topMargin + (availH - totalH) / 2;

	// logo location
	if (g_menu.logo && g_menu.logo->GetLastStatus() == Ok) {
		int lw = g_menu.logo->GetWidth(), lh = g_menu.logo->GetHeight();
		double sx = (W * kLogoMaxWPct) / lw;
		double sy = (H * kLogoMaxHPct) / lh;
		double s = std::min(sx, sy);
		int drawW = (int)(lw * s), drawH = (int)(lh * s);
		int dx = (W - drawW) / 2;
		int dy = std::max(16, startY - drawH - kLogoGap);
		g.DrawImage(g_menu.logo, dx, dy, drawW, drawH);
	}

	// mouse
	POINT mp; GetCursorPos(&mp); ScreenToClient(hwnd, &mp);
	int hover = -1;

	// draw buttons 
	for (int i = 0; i < count; ++i) {
		RECT r{ x, startY + i * (btnH + gap), x + btnW, startY + i * (btnH + gap) + btnH };
		g_menu.rects[i] = r;

		if (mp.x >= r.left && mp.x <= r.right && mp.y >= r.top && mp.y <= r.bottom)
			hover = i;

		if (g_menu.buttonImgs[i] && g_menu.buttonImgs[i]->GetLastStatus() == Ok) {
			int imgW = g_menu.buttonImgs[i]->GetWidth();
			int imgH = g_menu.buttonImgs[i]->GetHeight();
			double aspect = (double)imgW / imgH;

			int drawH = btnH;
			int drawW = (int)(drawH * aspect);
			int dx = r.left + (btnW - drawW) / 2;
			int dy = r.top;

			g.DrawImage(g_menu.buttonImgs[i], dx, dy, drawW, drawH);
		}
	}

	// ship next to active (hover has priority)
	int active = (hover != -1) ? hover : g_menu.selected;
	if (g_menu.ship && g_menu.ship->GetLastStatus() == Ok) {
		RECT r = g_menu.rects[active];

		int iconH = btnH;
		int iconW = (int)((double)g_menu.ship->GetWidth() * iconH / (double)g_menu.ship->GetHeight());
		int ix = r.left - iconW - kShipGap;
		int iy = r.top + (btnH - iconH) / 2;
		if (ix < 8) ix = 8;
		g.DrawImage(g_menu.ship, ix, iy, iconW, iconH);
	}

	BitBlt(hdc, 0, 0, W, H, memDC, 0, 0, SRCCOPY);
	SelectObject(memDC, oldBMP);
	DeleteObject(memBMP);
	DeleteDC(memDC);

	ReleaseDC(hwnd, hdc);

#endif
}

static void DrawInitialsOverlay(GW::SYSTEM::GWindow& gw, entt::registry& registry)
{
#ifdef _WIN32
	using namespace Gdiplus;

	// Get name input from context, not from an entity
	auto* nameInputPtr = registry.ctx().find<GAME::PlayerNameInput>();
	if (!nameInputPtr) {
		// Optional debug:
		// std::cout << "DrawInitialsOverlay: no PlayerNameInput in context\n";
		return;
	}
	const GAME::PlayerNameInput& nameInput = *nameInputPtr;

	GW::SYSTEM::UNIVERSAL_WINDOW_HANDLE uwh{};
	if (!+ gw.GetWindowHandle(uwh)) return;
	HWND hwnd = reinterpret_cast<HWND>(uwh.window);
	if (!hwnd) return;

	HDC hdc = GetDC(hwnd);
	if (!hdc) return;

	RECT rc; GetClientRect(hwnd, &rc);
	const int W = rc.right - rc.left;
	const int H = rc.bottom - rc.top;

	HDC memDC = CreateCompatibleDC(hdc);
	HBITMAP memBMP = CreateCompatibleBitmap(hdc, W, H);
	HGDIOBJ oldBMP = SelectObject(memDC, memBMP);

	Graphics g(memDC);
	g.SetSmoothingMode(SmoothingModeHighQuality);
	g.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);

	// Dim background
	SolidBrush bgBrush(Color(180, 0, 0, 0));
	g.FillRectangle(&bgBrush, 0, 0, W, H);

	// Consolas font family
	FontFamily fontFamily(L"Consolas");

	// Title font (smaller than initials)
	Font titleFont(&fontFamily, 36.0f, FontStyleRegular, UnitPixel);
	// Initials font (big)
	Font initialsFont(&fontFamily, 60.0f, FontStyleRegular, UnitPixel);
	// Hint font (small)
	Font hintFont(&fontFamily, 22.0f, FontStyleRegular, UnitPixel);

	SolidBrush whiteBrush(Color(255, 255, 255, 255));
	StringFormat centerFmt;
	centerFmt.SetAlignment(StringAlignmentCenter);
	centerFmt.SetLineAlignment(StringAlignmentCenter);

	// Title
	const wchar_t* titleText = L"ENTER INITIALS";
	RectF titleRect(0.0f, H * 0.22f, (REAL)W, 60.0f);
	g.DrawString(titleText, -1, &titleFont, titleRect, &centerFmt, &whiteBrush);

	// Initials text
	std::wstring wName(nameInput.name.begin(), nameInput.name.end());

	RectF layoutRect(0.0f, 0.0f, (REAL)W, 200.0f);
	RectF measured;
	g.MeasureString(wName.c_str(), (INT)wName.size(), &initialsFont, layoutRect, &measured);

	REAL textX = (W - measured.Width) * 0.5f;
	REAL textY = H * 0.45f;

	g.DrawString(wName.c_str(), (INT)wName.size(), &initialsFont,
		PointF(textX, textY), &whiteBrush);

	// Underline the selected letter
	if (!wName.empty()) {
		int count = (int)wName.size();
		REAL charWidth = measured.Width / count;

		REAL underlineWidth = charWidth * 0.8f;
		REAL underlineX = textX + charWidth * nameInput.currentIndex
			+ (charWidth - underlineWidth) * 0.5f;
		REAL underlineY = textY + measured.Height + 5.0f;
		REAL underlineHeight = 4.0f;

		SolidBrush underlineBrush(Color(255, 255, 255, 255));
		g.FillRectangle(&underlineBrush,
			underlineX, underlineY,
			underlineWidth, underlineHeight);
	}

	// Tooltip / hint text
	const wchar_t* hintText =
		L"Use W/S or UP/DOWN to change the letter\n"
		L"Use A/D or LEFT/RIGHT to move between letters\n"
		L"Press ENTER to confirm, BACKSPACE to reset";

	RectF hintRect(
		0.0f,
		(REAL)(H * 0.68f),   // a bit below the initials
		(REAL)W,
		100.0f
	);
	g.DrawString(hintText, -1, &hintFont, hintRect, &centerFmt, &whiteBrush);

	BitBlt(hdc, 0, 0, W, H, memDC, 0, 0, SRCCOPY);
	SelectObject(memDC, oldBMP);
	DeleteObject(memBMP);
	DeleteDC(memDC);
	ReleaseDC(hwnd, hdc);
#endif
}

static void DrawGameOverOverlay(GW::SYSTEM::GWindow& gw, entt::registry& registry)
{
#ifdef _WIN32
	using namespace Gdiplus;
	using namespace std::chrono;

	struct OverlayState {
		bool gdiReady = false;
		ULONG_PTR gdiToken = 0;
		Image* gameOverImg = nullptr;
		Image* restartImg = nullptr;
		Image* highscoreImg = nullptr;
		Image* quitImg = nullptr;
		Image* mainMenuImg = nullptr;
		RECT restartRect = {};
		RECT highscoreRect = {};
		RECT quitRect = {};
		RECT mainMenuRect = {};
		steady_clock::time_point startTime;
	};
	static OverlayState state;

	// Initialize GDI+ and load images
	if (!state.gdiReady) {
		GdiplusStartupInput si;
		if (GdiplusStartup(&state.gdiToken, &si, nullptr) == Ok) {
			state.gdiReady = true;
			state.gameOverImg = new Image(FindAsset(L"GameOver.png").c_str());
			state.restartImg = new Image(FindAsset(L"Restart.png").c_str());
			state.highscoreImg = new Image(FindAsset(L"HighScore.png").c_str());
			state.quitImg = new Image(FindAsset(L"Quit2.png").c_str());
			state.mainMenuImg = new Image(FindAsset(L"MainMenu.png").c_str());
			state.startTime = steady_clock::now();
		}
	}

	GW::SYSTEM::UNIVERSAL_WINDOW_HANDLE uwh{};
	if (!+ gw.GetWindowHandle(uwh)) return;
	HWND hwnd = reinterpret_cast<HWND>(uwh.window);
	if (!hwnd) return;

	HDC hdc = GetDC(hwnd);
	if (!hdc) return;

	RECT rc; GetClientRect(hwnd, &rc);
	const int W = rc.right - rc.left;
	const int H = rc.bottom - rc.top;

	HDC memDC = CreateCompatibleDC(hdc);
	HBITMAP memBMP = CreateCompatibleBitmap(hdc, W, H);
	HGDIOBJ oldBMP = SelectObject(memDC, memBMP);

	Graphics g(memDC);
	g.SetInterpolationMode(InterpolationModeHighQualityBicubic);
	g.SetCompositingMode(CompositingModeSourceOver);

	// Fade-in over 1 second
	std::chrono::steady_clock::time_point startTime;
	if (auto* data = registry.ctx().find<GAME::GameOverOverlayData>()) {
		startTime = data->start;
	}
	else {
		// fallback to the static startTime
		startTime = state.startTime;
	}
	double elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - startTime).count();
	BYTE alpha = static_cast<BYTE>(std::min(255.0, elapsed * 255.0)); // 1s fade

	ColorMatrix fadeMatrix = {
		1, 0, 0, 0, 0,
		0, 1, 0, 0, 0,
		0, 0, 1, 0, 0,
		0, 0, 0, alpha / 255.0f, 0,
		0, 0, 0, 0, 1
	};
	ImageAttributes attrs;
	attrs.SetColorMatrix(&fadeMatrix, ColorMatrixFlagsDefault, ColorAdjustTypeBitmap);

	// Draw GameOver image
	if (state.gameOverImg && state.gameOverImg->GetLastStatus() == Ok) {
		int iw = state.gameOverImg->GetWidth(), ih = state.gameOverImg->GetHeight();
		double scale = std::min(W * 0.5 / iw, H * 0.3 / ih);
		int dw = int(iw * scale), dh = int(ih * scale);
		int dx = (W - dw) / 2, dy = H / 3 - dh / 2;
		Rect dest(dx, dy, dw, dh);
		g.DrawImage(state.gameOverImg, dest, 0, 0, iw, ih, UnitPixel, &attrs);
	}

	// Restart button
	if (state.restartImg && state.restartImg->GetLastStatus() == Ok) {
		int iw = state.restartImg->GetWidth(), ih = state.restartImg->GetHeight();
		double scale = std::min(W * 0.25 / iw, H * 0.1 / ih);
		int dw = int(iw * scale), dh = int(ih * scale);
		int dx = (W - dw) / 2, dy = int(H * 0.50f);
		Rect dest(dx, dy, dw, dh);
		g.DrawImage(state.restartImg, dest, 0, 0, iw, ih, UnitPixel, &attrs);
		SetRect(&state.restartRect, dx, dy, dx + dw, dy + dh);
	}

	// Highscore button
	if (state.highscoreImg && state.highscoreImg->GetLastStatus() == Ok) {
		int iw = state.highscoreImg->GetWidth(), ih = state.highscoreImg->GetHeight();
		double scale = std::min(W * 0.25 / iw, H * 0.1 / ih);
		int dw = int(iw * scale), dh = int(ih * scale);
		int dx = (W - dw) / 2, dy = int(H * 0.60f);
		Rect dest(dx, dy, dw, dh);
		g.DrawImage(state.highscoreImg, dest, 0, 0, iw, ih, UnitPixel, &attrs);
		SetRect(&state.highscoreRect, dx, dy, dx + dw, dy + dh);
	}

	// Main Menu button
	if (state.mainMenuImg && state.mainMenuImg->GetLastStatus() == Ok) {
		int iw = state.mainMenuImg->GetWidth(), ih = state.mainMenuImg->GetHeight();
		double scale = std::min(W * 0.25 / iw, H * 0.1 / ih);
		int dw = int(iw * scale), dh = int(ih * scale);
		int dx = (W - dw) / 2, dy = int(H * 0.70f);
		Rect dest(dx, dy, dw, dh);
		g.DrawImage(state.mainMenuImg, dest, 0, 0, iw, ih, UnitPixel, &attrs);
		SetRect(&state.mainMenuRect, dx, dy, dx + dw, dy + dh);
	}

	// Quit button
	if (state.quitImg && state.quitImg->GetLastStatus() == Ok) {
		int iw = state.quitImg->GetWidth(), ih = state.quitImg->GetHeight();
		double scale = std::min(W * 0.25 / iw, H * 0.1 / ih);
		int dw = int(iw * scale), dh = int(ih * scale);
		int dx = (W - dw) / 2, dy = int(H * 0.80f);
		Rect dest(dx, dy, dw, dh);
		g.DrawImage(state.quitImg, dest, 0, 0, iw, ih, UnitPixel, &attrs);
		SetRect(&state.quitRect, dx, dy, dx + dw, dy + dh);
	}

	BitBlt(hdc, 0, 0, W, H, memDC, 0, 0, SRCCOPY);

	SelectObject(memDC, oldBMP);
	DeleteObject(memBMP);
	DeleteDC(memDC);
	ReleaseDC(hwnd, hdc);

	// Handle mouse clicks
	POINT mp; GetCursorPos(&mp); ScreenToClient(hwnd, &mp);
	bool mouseDown = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;

	static bool mouseWasDown = false;
	if (mouseDown && !mouseWasDown)
	{
		if (PtInRect(&state.restartRect, mp)) {
			// Same as pressing 'R'
			registry.ctx().emplace<GAME::RequestReset>();
		}
		else if (PtInRect(&state.highscoreRect, mp)) {
			// Navigate to high scores screen
			// First clear game over state
			if (registry.ctx().find<GAME::GameOver>()) {
				registry.ctx().erase<GAME::GameOver>();
			}
			if (registry.ctx().find<GAME::GameOverOverlayData>()) {
				registry.ctx().erase<GAME::GameOverOverlayData>();
			}
			// Destroy game over screen entities
			auto overView = registry.view<GAME::GameOverScreen>();
			for (auto e : overView) {
				registry.destroy(e);
			}
			// Create high scores screen
			auto hs = registry.create();
			registry.emplace<GAME::HighScoresScreen>(hs);
		}
		else if (PtInRect(&state.quitRect, mp)) {
			// Quit the game
			auto winView = registry.view<APP::Window>();
			for (auto entity : winView) {
				auto& gwClose = registry.get<GW::SYSTEM::GWindow>(entity);
				GW::SYSTEM::UNIVERSAL_WINDOW_HANDLE uwh3{};
				if (+gwClose.GetWindowHandle(uwh3)) {
					HWND h = reinterpret_cast<HWND>(uwh3.window);
					PostMessage(h, WM_CLOSE, 0, 0);
				}
			}
		}
		else if (PtInRect(&state.mainMenuRect, mp)) {
			// Navigate to main menu
			if (registry.ctx().find<GAME::GameOver>()) {
				registry.ctx().erase<GAME::GameOver>();
			}
			if (registry.ctx().find<GAME::GameOverOverlayData>()) {
				registry.ctx().erase<GAME::GameOverOverlayData>();
			}
			auto overView = registry.view<GAME::GameOverScreen>();
			for (auto e : overView) {
				registry.destroy(e);
			}
			GAME::CleanupGameplayEntities(registry);
			auto menu = registry.create();
			registry.emplace<GAME::StartScreen>(menu);
		}
	}
	mouseWasDown = mouseDown;

#endif
}

static void DrawEndCreditsOverlay(GW::SYSTEM::GWindow& gw, entt::registry& registry)
{
#ifdef _WIN32
	using namespace Gdiplus;
	using namespace std::chrono;

	struct CreditsOverlayState {
		bool gdiReady = false;
		ULONG_PTR gdiToken = 0;
		Image* bg = nullptr;
		Image* logo = nullptr;
	};
	static CreditsOverlayState state;

	// init GDI+ and load images
	if (!state.gdiReady) {
		GdiplusStartupInput si;
		if (GdiplusStartup(&state.gdiToken, &si, nullptr) == Ok) {
			state.gdiReady = true;
			state.logo = new Image(FindAsset(L"Logo.png").c_str()); // 
			state.bg = new Image(FindAsset(L"EndCreditsBG.png").c_str());
		
		}
	}

	GW::SYSTEM::UNIVERSAL_WINDOW_HANDLE uwh{};
	if (!+ gw.GetWindowHandle(uwh)) return;
	HWND hwnd = reinterpret_cast<HWND>(uwh.window);
	if (!hwnd) return;

	HDC hdc = GetDC(hwnd);
	if (!hdc) return;

	RECT rc; GetClientRect(hwnd, &rc);
	const int W = rc.right - rc.left;
	const int H = rc.bottom - rc.top;

	HDC memDC = CreateCompatibleDC(hdc);
	HBITMAP memBMP = CreateCompatibleBitmap(hdc, W, H);
	HGDIOBJ oldBMP = SelectObject(memDC, memBMP);

	Graphics g(memDC);
	g.SetInterpolationMode(InterpolationModeHighQualityBicubic);
	g.SetCompositingMode(CompositingModeSourceOver);

	// star background
	if (state.bg && state.bg->GetLastStatus() == Ok) {
		int iw = state.bg->GetWidth(), ih = state.bg->GetHeight();
		double s = std::max((double)W / iw, (double)H / ih);
		int dw = int(iw * s), dh = int(ih * s);
		int dx = (W - dw) / 2, dy = (H - dh) / 2;
		g.DrawImage(state.bg, dx, dy, dw, dh);
	}

	// timing for scroll
	steady_clock::time_point startTime;
	if (auto* data = registry.ctx().find<GAME::EndCreditsOverlayData>())
		startTime = data->start;
	else
		startTime = steady_clock::now();

	double elapsed = duration<double>(steady_clock::now() - startTime).count();

	// scroll parameters
	const float scrollSpeed = 40.0f;  // pixels per second
	const float lineSpacing = 32.0f;  // vertical spacing between slots
	float baseY = (float)H + 50.0f - (float)(elapsed * scrollSpeed);

	// credits text
	static const wchar_t* creditsLines[] = {
		L"",
		L"",
		L"",
		L"  Damon Simpson",
		L"- Game over screen, Restart GameRespawning",
		L"",
		L"",
		L"  Damien Remington",
		L"- High Score Menu, Text Implementation",
		L"",
		L"",
		L"  Nicholas Call",
		L"- Player Movement ,Flash Red(Damage Indicators), Power Up System"
		L"",
		L"",
		L"  David Oross",
		L"- Enemy movement, wave logic,stage logic"
		L"",
		L"",
		L" Christopher Wilt",
		L"- Audio,Power Up,stage logic"
		L"",
		L"",
		L" Luis R. Rojas",
		L"- UI, MEnus(Pause Menu + Main Menu)"
		L"",
		L"",
		L"",
		L"Models Used",
		L"Space Ships - https://psionicgames.itch.io/low-poly-space-ship-pack ",
		L"",
		L"",
		L"Design",
		L"  Team Orange",
		L"",
		L"",
		L"Thank you for playing!"
	};
	const int textCount = sizeof(creditsLines) / sizeof(creditsLines[0]);

	// slot 0 = logo, slots 1..N = text lines
	int totalSlots = 1 + textCount;

	FontFamily fontFamily(L"Segoe UI");
	Font sectionFont(&fontFamily, 24.0f, FontStyleBold, UnitPixel);
	Font bodyFont(&fontFamily, 20.0f, FontStyleRegular, UnitPixel);
	SolidBrush textBrush(Color(255, 255, 255, 255));

	StringFormat format;
	format.SetAlignment(StringAlignmentCenter);
	format.SetLineAlignment(StringAlignmentCenter);

	for (int slot = 0; slot < totalSlots; ++slot) {
		float y = baseY + slot * lineSpacing;
		if (y < -80 || y > H + 80) continue; // skip completely off-screen stuff

		if (slot == 0) {
			// draw the LOGO as the first scrolling element
			if (state.logo && state.logo->GetLastStatus() == Ok) {
				int iw = state.logo->GetWidth();
				int ih = state.logo->GetHeight();

				// scale nicely for window
				double maxW = W * 0.45;
				double maxH = H * 0.25;
				double s = std::min(maxW / iw, maxH / ih);
				int dw = int(iw * s), dh = int(ih * s);

				int dx = (W - dw) / 2;
				int dy = int(y - dh * 0.5f); // center logo on this y

				g.DrawImage(state.logo, dx, dy, dw, dh);
			}
		}
		else {
			const wchar_t* line = creditsLines[slot - 1];

			bool isSectionHeader =
				wcscmp(line, L"Programming") == 0 ||
				wcscmp(line, L"Design") == 0 ||
				wcscmp(line, L"Audio") == 0 ||
				wcscmp(line, L"Special Thanks") == 0;

			RectF layoutRect(
				0.0f,
				(Gdiplus::REAL)y,
				(Gdiplus::REAL)W,
				(Gdiplus::REAL)lineSpacing
			);

			g.DrawString(
				line,
				-1,
				isSectionHeader ? &sectionFont : &bodyFont,
				layoutRect,
				&format,
				&textBrush
			);
		}
	}

	// hint at bottom
	{
		const wchar_t* hintText = L"Press BACKSPACE to return";

		Font hintFont(&fontFamily, 18.0f, FontStyleRegular, UnitPixel);
		SolidBrush hintBrush(Color(200, 200, 200, 255));

		RectF layoutRect(
			0.0f,
			(Gdiplus::REAL)(H - 40),
			(Gdiplus::REAL)W,
			30.0f
		);
		g.DrawString(hintText, -1, &hintFont, layoutRect, &format, &hintBrush);
	}

	BitBlt(hdc, 0, 0, W, H, memDC, 0, 0, SRCCOPY);
	SelectObject(memDC, oldBMP);
	DeleteObject(memBMP);
	DeleteDC(memDC);
	ReleaseDC(hwnd, hdc);
#endif
}




static void DrawSettingsOverlay(GW::SYSTEM::GWindow& gw, entt::registry& registry, GAME::SettingsData& data)
{
#ifdef _WIN32
	using namespace Gdiplus;

	struct SettingsOverlayState {
		bool gdiReady = false;
		ULONG_PTR gdiToken = 0;
		Image* bg = nullptr;
		Image* masterLabel = nullptr;
		Image* musicLabel = nullptr;
		Image* sfxLabel = nullptr;
	};
	static SettingsOverlayState state;

	//start GDI+ and load pictures once
	if (!state.gdiReady) {
				GdiplusStartupInput si;
		if (GdiplusStartup(&state.gdiToken, &si, nullptr) == Ok) {
			state.gdiReady = true;
			state.bg = new Image(FindAsset(L"Settings_Background.png").c_str());
			state.masterLabel = new Image(FindAsset(L"MasterVolume.png").c_str());
			state.musicLabel = new Image(FindAsset(L"MusicVolume.png").c_str());
			state.sfxLabel = new Image(FindAsset(L"SoundEffects.png").c_str());
		}
	}
	GW::SYSTEM::UNIVERSAL_WINDOW_HANDLE uwh{};
	if (!+ gw.GetWindowHandle(uwh)) return;
	HWND hwnd = reinterpret_cast<HWND>(uwh.window);
	if (!hwnd) return;

	HDC hdc = GetDC(hwnd);
	if (!hdc) return;

	RECT rc; GetClientRect(hwnd, &rc);
	const int W = rc.right - rc.left;
	const int H = rc.bottom - rc.top;

	HDC memDC = CreateCompatibleDC(hdc);
	HBITMAP memBMP = CreateCompatibleBitmap(hdc, W, H);
	HGDIOBJ oldBMP = SelectObject(memDC, memBMP);

	Graphics g(memDC);
	g.SetInterpolationMode(InterpolationModeHighQualityBicubic);
	g.SetCompositingMode(CompositingModeSourceOver);
	//draw background
	if (state.bg && state.bg->GetLastStatus() == Ok) {
		int iw = state.bg->GetWidth();
		int ih = state.bg->GetHeight();
		double s = std::min((double)W / iw, (double)H / ih);
		int dw = int(iw * s), dh = int(ih * s);
		int dx = (W - dw) / 2, dy = (H - dh) / 2;
		g.DrawImage(state.bg, dx, dy, dw, dh);
	}

	// Layout
	const int centerX = W / 2;
	const float startYPct = 0.001f;
	const float spacing = 0.11f;
	const int sliderW = (int)(W * 0.55f);
	const int knobRadius = 8;
	const int sliderGap = 10;
	
	Image* labels[3] = { state.masterLabel, state.musicLabel, state.sfxLabel };
	float values[3] = { data.masterVolume, data.musicVolume, data.sfxVolume };

	Pen linePen(Color(255, 255, 255), 6);
	SolidBrush knobBrush(Color(0, 200, 255));
	Pen selectedOutline(Color(255, 255, 0), 3);

	for (int i = 0; i < 3; ++i) {
		float rowY = H * (startYPct + i * spacing);
		int labelY = (int)rowY;
		// Draw label
		int labelH = 0;
		if (labels[i] && labels[i]->GetLastStatus() == Ok) {
			int lw = labels[i]->GetWidth();
			int lh = labels[i]->GetHeight();
			labelH = lh;

			double maxW = W * 0.5;
			double s = std::min(1.0, maxW / lw);
			int dw = int(lw * s), dh = int(lh * s);
			int dx = centerX - dw / 2;
			int dy = labelY;

			g.DrawImage(labels[i], dx, dy, dw, dh);
		}

		//slider position

		int lineY = labelY + labelH / 2 + sliderGap;
		int lineL = centerX - sliderW / 2;
		int lineR = centerX + sliderW / 2;

		g.DrawLine(&linePen, lineL, lineY, lineR, lineY);

		SetRect(&g_settingsUI.sliderRects[i],
			lineL, lineY - knobRadius * 2,
			lineR, lineY + knobRadius * 2);

		float v = std::max(0.0f, std::min(1.0f, values[i]));
		int knobX = lineL + int(v * (lineR - lineL));
		int knobY = lineY;

		Rect knobRect(knobX - knobRadius, knobY - knobRadius,
			knobRadius * 2, knobRadius * 2);
		g.FillEllipse(&knobBrush, knobRect);

		if (data.selectedSlider == i)
			g.DrawEllipse(&selectedOutline, knobRect);
	}

	//Draw "press BACKSPACE to return" at bottom

	{
		const wchar_t* hintText = L"Press BACKSPACE to return";

		Gdiplus::FontFamily fontFamily(L"Segoe UI");
		Gdiplus::Font font(&fontFamily, 18.0f, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
		Gdiplus::SolidBrush textBrush(Gdiplus::Color(255, 255, 255, 255));

		Gdiplus::StringFormat format;
		format.SetAlignment(Gdiplus::StringAlignmentCenter);
		format.SetLineAlignment(Gdiplus::StringAlignmentCenter);

		Gdiplus::RectF layoutRect(
			0.0f,
			static_cast<Gdiplus::REAL>(H - 40), // 40px from bottom
			static_cast<Gdiplus::REAL>(W),
			30.0f
		);

		g.DrawString(
			hintText,
			-1,
			&font,
			layoutRect,
			&format,
			&textBrush
		);
	}

	

	BitBlt(hdc, 0, 0, W, H, memDC, 0, 0, SRCCOPY);
	SelectObject(memDC, oldBMP);
	DeleteObject(memBMP);
	DeleteDC(memDC);
	ReleaseDC(hwnd, hdc);
#endif
}

static void DrawHowToPlayOverlay(GW::SYSTEM::GWindow& gw, entt::registry& registry)
{
#ifdef _WIN32
	using namespace Gdiplus;

	struct HTPState {
		bool gdiReady = false;
		ULONG_PTR gdiToken = 0;
		Image* page1 = nullptr;      // Controls + Power Ups
		Image* page2 = nullptr;      // Enemy Types & Drops
		Image* arrowNext = nullptr;  // right triangle
		Image* arrowPrev = nullptr;  // left triangle
	};
	static HTPState state;

	// Init GDI+ and load images once
	if (!state.gdiReady) {
		GdiplusStartupInput si;
		if (GdiplusStartup(&state.gdiToken, &si, nullptr) == Ok) {
			state.gdiReady = true;

			state.page1 = new Image(FindAsset(L"HTP_Controls&Powers.png").c_str());
			state.page2 = new Image(FindAsset(L"HTP_EnemyTypes.png").c_str());
			state.arrowNext = new Image(FindAsset(L"ToPage2.png").c_str());
			state.arrowPrev = new Image(FindAsset(L"BackToPage1.png").c_str());
		}
	}

	GW::SYSTEM::UNIVERSAL_WINDOW_HANDLE uwh{};
	if (!+ gw.GetWindowHandle(uwh)) return;
	HWND hwnd = reinterpret_cast<HWND>(uwh.window);
	if (!hwnd) return;

	HDC hdc = GetDC(hwnd);
	if (!hdc) return;

	RECT rc; GetClientRect(hwnd, &rc);
	const int W = rc.right - rc.left;
	const int H = rc.bottom - rc.top;

	HDC memDC = CreateCompatibleDC(hdc);
	HBITMAP memBMP = CreateCompatibleBitmap(hdc, W, H);
	HGDIOBJ oldBMP = SelectObject(memDC, memBMP);

	Graphics g(memDC);
	g.SetInterpolationMode(InterpolationModeHighQualityBicubic);
	g.SetCompositingMode(CompositingModeSourceOver);

	// Which page are we on?
	int page = 0;
	if (auto* s = registry.ctx().find<GAME::HowToPlayState>())
		page = s->page;

	Image* bg = (page == 0) ? state.page1 : state.page2;

	// Draw background page image fullscreen
	if (bg && bg->GetLastStatus() == Ok) {
		int iw = bg->GetWidth(), ih = bg->GetHeight();
		double s = std::min((double)W / iw, (double)H / ih);
		int dw = int(iw * s), dh = int(ih * s);
		int dx = (W - dw) / 2, dy = (H - dh) / 2;
		g.DrawImage(bg, dx, dy, dw, dh);
	}

	// Reset rects by default
	SetRect(&g_htpUI.nextRect, 0, 0, 0, 0);
	SetRect(&g_htpUI.prevRect, 0, 0, 0, 0);

	// Draw the appropriate triangle and store its rect
	const int margin = (int)(std::min(W, H) * 0.05f);
	const int arrowSize = (int)(std::min(W, H) * 0.10f);

	if (page == 0 && state.arrowNext && state.arrowNext->GetLastStatus() == Ok) {
		// right arrow at bottom-right
		int iw = state.arrowNext->GetWidth(), ih = state.arrowNext->GetHeight();
		double s = std::min((double)arrowSize / std::max(iw, ih), 1.0);
		int dw = int(iw * s), dh = int(ih * s);
		int dx = W - dw - margin;
		int dy = H - dh - margin;
		Rect dest(dx, dy, dw, dh);
		g.DrawImage(state.arrowNext, dest, 0, 0, iw, ih, UnitPixel);

		SetRect(&g_htpUI.nextRect, dx, dy, dx + dw, dy + dh);
	}
	else if (page == 1 && state.arrowPrev && state.arrowPrev->GetLastStatus() == Ok) {
		// left arrow at bottom-left
		int iw = state.arrowPrev->GetWidth(), ih = state.arrowPrev->GetHeight();
		double s = std::min((double)arrowSize / std::max(iw, ih), 1.0);
		int dw = int(iw * s), dh = int(ih * s);
		int dx = margin;
		int dy = H - dh - margin;
		Rect dest(dx, dy, dw, dh);
		g.DrawImage(state.arrowPrev, dest, 0, 0, iw, ih, UnitPixel);

		SetRect(&g_htpUI.prevRect, dx, dy, dx + dw, dy + dh);
	}
	// Draw "Press BACKSPACE to return" at bottom center
	{
		const wchar_t* hintText = L"Press BACKSPACE to return";

		Gdiplus::FontFamily fontFamily(L"Segoe UI");
		Gdiplus::Font font(&fontFamily, 18.0f, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
		Gdiplus::SolidBrush textBrush(Gdiplus::Color(255, 255, 255, 255));

		Gdiplus::StringFormat format;
		format.SetAlignment(Gdiplus::StringAlignmentCenter);
		format.SetLineAlignment(Gdiplus::StringAlignmentCenter);

		Gdiplus::RectF layoutRect(
			0.0f,
			static_cast<Gdiplus::REAL>(H - 40),  // 40px from bottom
			static_cast<Gdiplus::REAL>(W),
			30.0f
		);

		g.DrawString(
			hintText,
			-1,
			&font,
			layoutRect,
			&format,
			&textBrush
		);
	}

	BitBlt(hdc, 0, 0, W, H, memDC, 0, 0, SRCCOPY);
	SelectObject(memDC, oldBMP);
	DeleteObject(memBMP);
	DeleteDC(memDC);
	ReleaseDC(hwnd, hdc);
#endif
}


static void DrawPauseOverlay(GW::SYSTEM::GWindow& gw, entt::registry& registry)
{
#ifdef _WIN32
	using namespace Gdiplus;

	auto* pause = registry.ctx().find<GAME::PauseMenuState>();
	if (!pause) return;

	struct PauseOverlayState
	{
		bool gdiReady = false;
		ULONG_PTR gdiToken = 0;


		Image* bg = nullptr;
		Image* resume = nullptr;
		Image* restart = nullptr;
		Image* mainMenu = nullptr;
		Image* settings = nullptr;
		Image* quit = nullptr;

		Image* confirmRestart = nullptr;
		Image* confirmMainMenu = nullptr;
		Image* confirmQuit = nullptr;
	};
	static PauseOverlayState state;

	//initialize GDI+ and load img
	if (!state.gdiReady) {
		GdiplusStartupInput si;
		if (GdiplusStartup(&state.gdiToken, &si, nullptr) == Ok) {
			state.gdiReady = true;

			//load images
			state.bg = new Image(FindAsset(L"Pause Menu.png").c_str());
			state.resume = new Image(FindAsset(L"pm_resume.png").c_str());
			state.restart = new Image(FindAsset(L"pm_restart.png").c_str());
			state.mainMenu = new Image(FindAsset(L"pm_mainmenu.png").c_str());
			state.quit = new Image(FindAsset(L"pm_quitgame.png").c_str());
			state.settings = new Image(FindAsset(L"pm_settings.png").c_str());

			state.confirmRestart = new Image(FindAsset(L"pm_youwanttorestart.png").c_str());
			state.confirmMainMenu = new Image(FindAsset(L"pm_returntomainmenu.png").c_str());
			state.confirmQuit = new Image(FindAsset(L"pm_youwantoquit.png").c_str());
		}
	}
	GW::SYSTEM::UNIVERSAL_WINDOW_HANDLE uwh{};
	if (!+ gw.GetWindowHandle(uwh)) return;
	HWND hwnd = reinterpret_cast<HWND>(uwh.window);
	if (!hwnd) return;

	HDC hdc = GetDC(hwnd);
	if (!hdc) return;

	RECT rc; GetClientRect(hwnd, &rc);
	const int W = rc.right - rc.left;
	const int H = rc.bottom - rc.top;

	HDC memDC = CreateCompatibleDC(hdc);
	HBITMAP memBMP = CreateCompatibleBitmap(hdc, W, H);
	HGDIOBJ oldBMP = SelectObject(memDC, memBMP);

	Graphics g(memDC);
	g.SetInterpolationMode(InterpolationModeHighQualityBicubic);
	g.SetCompositingMode(CompositingModeSourceOver);

	//Draw background (blurred?)
	if (state.bg && state.bg->GetLastStatus() == Ok) {
		int iw = state.bg->GetWidth(), ih = state.bg->GetHeight();
		double s = std::min((double)W / iw, (double)H / ih);
		int dw = int(iw * s), dh = int(ih * s);
		int dx = (W - dw) / 2, dy = (H - dh) / 2;
		g.DrawImage(state.bg, dx, dy, dw, dh);
	}  

	//when pressing Restart, Main Menu, or Quit, show confirmation dialog
	if (pause->confirm != GAME::PauseMenuState::ConfirmType::None) {
		Image* confirmImg = nullptr;
		switch (pause->confirm) {
		case GAME::PauseMenuState::ConfirmType::Restart:   confirmImg = state.confirmRestart;   break;
		case GAME::PauseMenuState::ConfirmType::MainMenu:  confirmImg = state.confirmMainMenu;  break;
		case GAME::PauseMenuState::ConfirmType::Quit:      confirmImg = state.confirmQuit;      break;
		default: break;
		}

		if (confirmImg && confirmImg->GetLastStatus() == Ok) {
			int iw = confirmImg->GetWidth(), ih = confirmImg->GetHeight();
			double s = std::min(W * 1.0 / iw, H * 0.50 / ih);
			int dw = int(iw * s), dh = int(ih * s);
			int dx = (W - dw) / 2;
			int dy = (H - dh) / 2;
			Rect dest(dx, dy, dw, dh);
			g.DrawImage(confirmImg, dest, 0, 0, iw, ih, UnitPixel);

			// left half = YES, right half = NO
			int textBoxWidth = dw / 3;  
			int textBoxHeight = dh / 5;    
			int textY = dy + dh / 2; 
			int spacingX = dw / 15;  

			// YES box (left)
			g_pauseUI.yesRect = {
				dx + dw / 2 - textBoxWidth - spacingX,
				textY - textBoxHeight / 2,
				dx + dw / 2 - spacingX,
				textY + textBoxHeight / 2
			};

			// NO box (right)
			g_pauseUI.noRect = {
				dx + dw / 2 + spacingX,
				textY - textBoxHeight / 2,
				dx + dw / 2 + textBoxWidth + spacingX,
				textY + textBoxHeight / 2
			};

			// draw rectangles around the currently selected YES/NO area
			Pen highlight(Color(255, 255, 255), 3);
			RECT sel = pause->confirmYesSelected ? g_pauseUI.yesRect : g_pauseUI.noRect;
			g.DrawRectangle(&highlight,
				sel.left,
				sel.top,
				sel.right - sel.left,
				sel.bottom - sel.top);
		}

		BitBlt(hdc, 0, 0, W, H, memDC, 0, 0, SRCCOPY);
		SelectObject(memDC, oldBMP);
		DeleteObject(memBMP);
		DeleteDC(memDC);
		ReleaseDC(hwnd, hdc);
		return;
	}
	// 2) draw normal pause menu buttons 

	Image* buttons[5] = {
	   state.resume,
	   state.restart,
	   state.mainMenu,
	   state.quit,
	   state.settings
	};

	const int count = 5;
	int btnH = int(H * 0.40f);
	int gap = int(H * 0.03f);
	int btnW = int(W * 1.0f);
	int x = (W - btnW) / 2;

	int topMargin = int(H * 0.20f); 
	int bottomMargin = int(H * 0.15f);

	int totalH = count * (btnH + gap) - gap;
	int availH = std::max(1, H - topMargin - bottomMargin);
	if (totalH > availH) {
		float s = (float)availH / (float)totalH;
		btnH = std::max(50, int(btnH * s));
		gap = std::max(6, int(gap * s));
		totalH = count * (btnH + gap) - gap;
	}

	int startY = topMargin + (availH - totalH) / 2;

	Pen highlight(Color(255, 255, 255), 3);

	for (int i = 0; i < count; ++i) {
		RECT r{ x, startY + i * (btnH + gap), x + btnW, startY + i * (btnH + gap) + btnH };
		g_pauseUI.buttonRects[i] = r;

		if (g_menu.ship && g_menu.ship->GetLastStatus() == Gdiplus::Ok) {
			int active = pause->selected;
			RECT r = g_pauseUI.buttonRects[active];

			int iconH = r.bottom - r.top; // same height as button
			int iconW = int((double)g_menu.ship->GetWidth() * iconH / (double)g_menu.ship->GetHeight());

			// Put the ship almost touching the button
			int ix = r.left + 100;   // move closer (5px inside the button's left)
			int iy = r.top;                // align top with button

			g.DrawImage(g_menu.ship, ix, iy, iconW, iconH);
		}
		

		Image* img = buttons[i];
		if (img && img->GetLastStatus() == Ok) {
			int iw = img->GetWidth(), ih = img->GetHeight();
			double aspect = (double)iw / ih;
			int drawH = btnH;
			int drawW = int(drawH * aspect);
			int dx = r.left + (btnW - drawW) / 2;
			int dy = r.top;
			g.DrawImage(img, dx, dy, drawW, drawH);
		}
		
	}

	BitBlt(hdc, 0, 0, W, H, memDC, 0, 0, SRCCOPY);
	SelectObject(memDC, oldBMP);
	DeleteObject(memBMP);
	DeleteDC(memDC);
	ReleaseDC(hwnd, hdc);

#endif
}

/*static void DrawHUDOverlay(GW::SYSTEM::GWindow& gw, entt::registry& registry)
{
#ifdef _WIN32
	// Find a player that has Health and Score
	auto playerView = registry.view<GAME::Player, GAME::Health, GAME::Score>();
	auto it = playerView.begin();
	if (it == playerView.end()) {
		return; // no player -> nothing to draw
	}

	entt::entity player = *it;
	auto& health = playerView.get<GAME::Health>(player);
	auto& score = playerView.get<GAME::Score>(player);

	// Lives: from context if saved there, otherwise from component
	int lives = 0;
	if (auto* livesCtx = registry.ctx().find<GAME::Lives>()) {
		lives = livesCtx->remaining;
	}
	else if (auto* livesComp = registry.try_get<GAME::Lives>(player)) {
		lives = livesComp->remaining;
	}

	// Format HUD string
	char buffer[128];
	sprintf_s(buffer, "HP: %d   Lives: %d   Score: %ld",
		health.hitPoints, lives, score.score);

	GW::SYSTEM::UNIVERSAL_WINDOW_HANDLE uwh{};
	if (!+ gw.GetWindowHandle(uwh)) return;
	HWND hwnd = reinterpret_cast<HWND>(uwh.window);
	if (!hwnd) return;

	HDC hdc = GetDC(hwnd);
	if (!hdc) return;

	RECT rc;
	GetClientRect(hwnd, &rc);
	const int W = rc.right - rc.left;
	const int H = rc.bottom - rc.top;

	// --- Double-buffer like the other overlays ---
	HDC memDC = CreateCompatibleDC(hdc);
	HBITMAP memBMP = CreateCompatibleBitmap(hdc, W, H);
	HGDIOBJ oldBMP = SelectObject(memDC, memBMP);

	// Copy the current Vulkan-rendered frame into the memory DC
	BitBlt(memDC, 0, 0, W, H, hdc, 0, 0, SRCCOPY);

	// Draw HUD text onto the memory DC
	SetBkMode(memDC, TRANSPARENT);
	SetTextColor(memDC, RGB(255, 255, 255));

	HFONT font = (HFONT)CreateFontA(
		24, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
		ANSI_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
		DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Consolas"
	);
	HFONT oldFont = (HFONT)SelectObject(memDC, font);

	RECT hudRect;
	hudRect.left = 20;
	hudRect.top = 10;
	hudRect.right = rc.right - 20;
	hudRect.bottom = 60;

	DrawTextA(memDC, buffer, -1, &hudRect, DT_LEFT | DT_TOP | DT_SINGLELINE);

	SelectObject(memDC, oldFont);
	DeleteObject(font);

	// Blit the combined image (3D + HUD) back to the window
	BitBlt(hdc, 0, 0, W, H, memDC, 0, 0, SRCCOPY);

	// Cleanup
	SelectObject(memDC, oldBMP);
	DeleteObject(memBMP);
	DeleteDC(memDC);
	ReleaseDC(hwnd, hdc);
#endif
}
*/

// Local routines for specific application behavior
void GraphicsBehavior(entt::registry& registry);
void GameplayBehavior(entt::registry& registry);
void MainLoopBehavior(entt::registry& registry);

// Architecture is based on components/entities pushing updates to other components/entities (via "patch" function)
int main()
{

	// All components, tags, and systems are stored in a single registry
	entt::registry registry;

	// initialize the ECS Component Logic
	CCL::InitializeComponentLogic(registry);

	// Seed the rand
	unsigned int time = std::chrono::steady_clock::now().time_since_epoch().count();
	srand(time);

	registry.ctx().emplace<UTIL::Config>();

	registry.ctx().emplace<UTIL::Random>(UTIL::Random(1, 100));
	GAME::HighScoreManager highScores;
	GAME::LoadHighScores(highScores);
	registry.ctx().emplace<GAME::HighScoreManager>(highScores);
	registry.ctx().emplace<GAME::SpecialCooldown>();

	// Running total score for this playthrough
	registry.ctx().emplace<GAME::PlayerTotalScore>(GAME::PlayerTotalScore{ 0 });

	registry.ctx().emplace<GAME::HUDData>();

	// --- Audio System Setup ---
	// Emplace GameAudio system into the registry's context
	registry.ctx().emplace<GameAudio>();

	// Get a reference to it and initialize it
	auto& audioSystem = registry.ctx().get<GameAudio>();
	if (!audioSystem.Init()) {
		std::cerr << "Failed to initialize audio system!" << std::endl;
		return -1; // Exit if audio fails
	}
	// --- End of Audio System Setup ---

	GraphicsBehavior(registry); // create windows, surfaces, and renderers

	GameplayBehavior(registry); // create entities and components for gameplay

	MainLoopBehavior(registry); // update windows and input


	auto displayView = registry.view<DRAW::VulkanRenderer>();
	if (!displayView.empty()) {
		auto displayEntity = displayView.front();

		// Get the renderer
		if (registry.all_of<DRAW::VulkanRenderer>(displayEntity)) {
			auto& vulkanRenderer = registry.get<DRAW::VulkanRenderer>(displayEntity);

			// Wait for all GPU work to complete
			if (vulkanRenderer.device != nullptr) {
				vkDeviceWaitIdle(vulkanRenderer.device);
			}

			// Destroy all gameplay entities first (they have mesh collections that reference buffers)
			std::vector<entt::entity> gameplayEntities;

			auto playerView = registry.view<GAME::Player>();
			for (auto e : playerView) gameplayEntities.push_back(e);

			auto enemyView = registry.view<GAME::Enemy>();
			for (auto e : enemyView) gameplayEntities.push_back(e);

			auto bulletView = registry.view<GAME::Bullet>();
			for (auto e : bulletView) gameplayEntities.push_back(e);

			// Destroy mesh collections first, then parent entities
			for (auto entity : gameplayEntities) {
				if (registry.valid(entity) && registry.any_of<DRAW::MeshCollection>(entity)) {
					DRAW::MeshCollection& meshCollection = registry.get<DRAW::MeshCollection>(entity);
					for (entt::entity meshEntity : meshCollection.entities) {
						if (registry.valid(meshEntity)) {
							registry.destroy(meshEntity);
						}
					}
				}
				if (registry.valid(entity)) {
					registry.destroy(entity);
				}
			}

			// Now manually remove buffer components while device is still valid
			if (registry.all_of<DRAW::VulkanIndexBuffer>(displayEntity)) {
				registry.remove<DRAW::VulkanIndexBuffer>(displayEntity);
			}
			if (registry.all_of<DRAW::VulkanVertexBuffer>(displayEntity)) {
				registry.remove<DRAW::VulkanVertexBuffer>(displayEntity);
			}
			if (registry.all_of<DRAW::VulkanGPUInstanceBuffer>(displayEntity)) {
				registry.remove<DRAW::VulkanGPUInstanceBuffer>(displayEntity);
			}
			if (registry.all_of<DRAW::VulkanUniformBuffer>(displayEntity)) {
				registry.remove<DRAW::VulkanUniformBuffer>(displayEntity);
			}
		}
	}



	// clear all entities and components from the registry
	// invokes on_destroy() for all components that have it
	// registry will still be intact while this is happening
	registry.clear();


	return 0; // now destructors will be called for all components
}

// This function will be called by the main loop to update the graphics
// It will be responsible for loading the Level, creating the VulkanRenderer, and all VulkanInstances
void GraphicsBehavior(entt::registry& registry)
{
	std::shared_ptr<const GameConfig> config = registry.ctx().get<UTIL::Config>().gameConfig;

	// Add an entity to handle all the graphics data
	auto display = registry.create();

	// TODO: Emplace CPULevel. Placing here to reduce occurrence of a json race condition crash
	DRAW::CPULevel cpuLevel = {};
	cpuLevel.levelFile = (*config).at("Level1").at("levelFile").as<std::string>();
	cpuLevel.modelPath = (*config).at("Level1").at("modelPath").as<std::string>();
	registry.emplace<DRAW::CPULevel>(display, cpuLevel);

	// Emplace and initialize Window component
	int windowWidth = (*config).at("Window").at("width").as<int>();
	int windowHeight = (*config).at("Window").at("height").as<int>();
	int startX = (*config).at("Window").at("xstart").as<int>();
	int startY = (*config).at("Window").at("ystart").as<int>();
	registry.emplace<APP::Window>(display,
		APP::Window{ startX, startY, windowWidth, windowHeight, GW::SYSTEM::GWindowStyle::WINDOWEDBORDERED, "SPACE RACE" });

	// Create the input
	auto& input = registry.ctx().emplace<UTIL::Input>();
	auto& window = registry.get<GW::SYSTEM::GWindow>(display);
	input.bufferedInput.Create(window);
	input.immediateInput.Create(window);
	input.gamePads.Create();
	auto& pressEvents = registry.ctx().emplace<GW::CORE::GEventCache>();
	pressEvents.Create(32);
	input.bufferedInput.Register(pressEvents);
	input.gamePads.Register(pressEvents);

	// Create a transient component to initialize the Renderer
	std::string vertShader = (*config).at("Shaders").at("vertex").as<std::string>();
	std::string pixelShader = (*config).at("Shaders").at("pixel").as<std::string>();
	registry.emplace<DRAW::VulkanRendererInitialization>(display,
		DRAW::VulkanRendererInitialization{
			vertShader, pixelShader,
			{ {0.2f, 0.2f, 0.25f, 1} } , { 1.0f, 0u }, 75.f, 0.1f, 100.0f });
	registry.emplace<DRAW::VulkanRenderer>(display);

	// TODO : Emplace GPULevel
	registry.emplace<DRAW::GPULevel>(display);

	// Register for Vulkan clean up
	GW::CORE::GEventResponder shutdown;
	shutdown.Create([&](const GW::GEvent& e) {
		GW::GRAPHICS::GVulkanSurface::Events event;
		GW::GRAPHICS::GVulkanSurface::EVENT_DATA data;
		if (+e.Read(event, data) && event == GW::GRAPHICS::GVulkanSurface::Events::RELEASE_RESOURCES) {
			//registry.clear<DRAW::VulkanRenderer>();
		}
		});
	registry.get<DRAW::VulkanRenderer>(display).vlkSurface.Register(shutdown);
	registry.emplace<GW::CORE::GEventResponder>(display, shutdown.Relinquish());

	// Create a camera and emplace it
	GW::MATH::GMATRIXF initialCamera;
	GW::MATH::GVECTORF translate = { 0.0f,  45.0f, -5.0f };
	GW::MATH::GVECTORF lookat = { 0.0f, 0.0f, 0.0f };
	GW::MATH::GVECTORF up = { 0.0f, 1.0f, 0.0f };
	GW::MATH::GMatrix::TranslateGlobalF(initialCamera, translate, initialCamera);
	GW::MATH::GMatrix::LookAtLHF(translate, lookat, up, initialCamera);
	// Inverse to turn it into a camera matrix, not a view matrix. This will let us do
	// camera manipulation in the component easier
	GW::MATH::GMatrix::InverseF(initialCamera, initialCamera);
	registry.emplace<DRAW::Camera>(display,
		DRAW::Camera{ initialCamera });
}

// This function will be called by the main loop to update the gameplay
// It will be responsible for updating the VulkanInstances and any other gameplay components
void GameplayBehavior(entt::registry& registry)
{
	std::shared_ptr<const GameConfig> config = registry.ctx().get<UTIL::Config>().gameConfig;

	DRAW::ModelManager* modelManager = registry.ctx().find<DRAW::ModelManager>();
	if (!modelManager) {
		return;
	}

	//GAME::SpawnPlayer(registry);
	//GAME::SpawnEnemy(registry);
	//GAME::SpawnExplosiveEnemy(registry, std::nullopt);
	entt::entity start = registry.create();
	registry.emplace<GAME::StartScreen>(start);

	// Create GameManager
	entt::entity gameManagerEntity = registry.create();
	registry.emplace<GAME::GameManager>(gameManagerEntity);
	registry.ctx().emplace<entt::entity>(gameManagerEntity);

	//create WaveLogic
	entt::entity waveLogicEntity = registry.create();
	registry.emplace<GAME::WaveLogic>(waveLogicEntity);
}

// This function will be called by the main loop to update the main loop
// It will be responsible for updating any created windows and handling any input
void MainLoopBehavior(entt::registry& registry)
{
	int closedCount;
	auto winView = registry.view<APP::Window>();
	auto& deltaTime = registry.ctx().emplace<UTIL::DeltaTime>().dtSec;

	do {
		static auto start = std::chrono::steady_clock::now();
		double elapsed = std::chrono::duration<double>(
			std::chrono::steady_clock::now() - start).count();
		start = std::chrono::steady_clock::now();
		if (elapsed > 1.0 / 30.0) elapsed = 1.0 / 30.0;
		deltaTime = elapsed;

		//start screen
		auto startView = registry.view<GAME::StartScreen>();
		if (!startView.empty())
		{
			// Play menu music
			auto& audioSystem = registry.ctx().get<GameAudio>();
			audioSystem.PlayMusic("BackgroundMenuMix");

			// START SCREEN
			GAME::CleanupGameplayEntities(registry);
			closedCount = 0;
			for (auto entity : winView) {
				if (registry.any_of<APP::WindowClosed>(entity))
					++closedCount;
				else {
					auto& gw = registry.get<GW::SYSTEM::GWindow>(entity);
					gw.ProcessWindowEvents();
				}
			}

			for (auto display : winView) {
				auto& gw = registry.get<GW::SYSTEM::GWindow>(display);
				DrawStartOverlay(gw, "", "");
			}

			if (closedCount > 0) {
#ifdef _WIN32
				FreeConsole();
				ExitProcess(0);
#endif
			}

			if (auto* in = registry.ctx().find<UTIL::Input>()) {
				float up = 0, down = 0, enter = 0;
				in->immediateInput.GetState(G_KEY_UP, up);
				in->immediateInput.GetState(G_KEY_DOWN, down);
				in->immediateInput.GetState(G_KEY_ENTER, enter);

				static double lastMoveTime = 0;
				static auto navClockStart = std::chrono::steady_clock::now();
				auto now = std::chrono::steady_clock::now();
				double t = std::chrono::duration<double>(now - navClockStart).count();

				auto stepSelect = [&](int dir) {
					g_menu.selected = (g_menu.selected + dir + 6) % 6;
					};

				if ((up > 0.5f || down > 0.5f) && (t - lastMoveTime) > 0.15) {
					if (up > 0.5f) stepSelect(-1);
					if (down > 0.5f) stepSelect(+1);
					lastMoveTime = t;
					audioSystem.Play("ui_hover");
				}

#ifdef _WIN32
				GW::SYSTEM::UNIVERSAL_WINDOW_HANDLE uwh2{};
				if (+registry.get<GW::SYSTEM::GWindow>(*winView.begin()).GetWindowHandle(uwh2))
				{
					HWND hwnd2 = reinterpret_cast<HWND>(uwh2.window);
					POINT mp; GetCursorPos(&mp); ScreenToClient(hwnd2, &mp);
					bool mouseDown = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;

					if (mouseDown && !g_menu.mouseDownLast) {
						for (int i = 0; i < 6; ++i) {
							RECT r = g_menu.rects[i];
							if (mp.x >= r.left && mp.x <= r.right && mp.y >= r.top && mp.y <= r.bottom) {
								g_menu.selected = i;
								enter = 1.0f;
								break;
							}
						}
					}
					g_menu.mouseDownLast = mouseDown;
				}
#endif
				if (enter > 0.5f) {
					if (g_menu.selected == 0) {
						// Start Game
						audioSystem.StopMusic("BackgroundMenuMix");

						for (auto e : startView) registry.destroy(e);
						GAME::SpawnPlayer(registry);
						//GAME::SpawnEnemy(registry);
						//GAME::SpawnExplosiveEnemy(registry, std::nullopt);

						//play was pressed, start stage 1
						GAME::WaveStageFunctions::playStage(registry, 1);


						GAME::createBackgroundStars(registry);
					}
					else if (g_menu.selected == 2) {
						// HOW TO PLAY (2-page screen)
						for (auto e : startView) registry.destroy(e);

						auto htpEntity = registry.create();
						registry.emplace<GAME::HowToPlayScreen>(htpEntity);

						// reset page to 0 every time we open it
						if (registry.ctx().find<GAME::HowToPlayState>())
							registry.ctx().erase<GAME::HowToPlayState>();
						registry.ctx().emplace<GAME::HowToPlayState>(GAME::HowToPlayState{ 0 });
					}
					else if (g_menu.selected == 1) {
						// SETTINGS
						for (auto e : startView) registry.destroy(e);
						auto settingsEntity = registry.create();
						registry.emplace<GAME::SettingsScreen>(settingsEntity);
					}
					else if (g_menu.selected == 3) { // High Scores
						for (auto e : startView)
							registry.destroy(e);
						auto hs = registry.create();
						registry.emplace<GAME::HighScoresScreen>(hs);
					}
					else if (g_menu.selected == 4) {
						// CREDITS
						for (auto e : startView)
							registry.destroy(e);

						auto creditsEntity = registry.create();
						registry.emplace<GAME::EndCreditScreen>(creditsEntity);

						// Reset credits timer
						if (registry.ctx().find<GAME::EndCreditsOverlayData>())
							registry.ctx().erase<GAME::EndCreditsOverlayData>();

						registry.ctx().emplace<GAME::EndCreditsOverlayData>(
							GAME::EndCreditsOverlayData{ std::chrono::steady_clock::now() });
					}

					else if (g_menu.selected == 5) {
						// Quit
#ifdef _WIN32			audioSystem.StopMusic("BackgroundMenuMix");


						for (auto entity : winView) {
							auto& gwClose = registry.get<GW::SYSTEM::GWindow>(entity);
							GW::SYSTEM::UNIVERSAL_WINDOW_HANDLE uwh3{};
							if (+gwClose.GetWindowHandle(uwh3)) {
								HWND h = reinterpret_cast<HWND>(uwh3.window);
								PostMessage(h, WM_CLOSE, 0, 0);
							}
						}
						FreeConsole();
						ExitProcess(0);
#endif
					}
				}
			}
			continue;
		}

		// SETTINGS SCREEN
		{
			auto settingsView = registry.view<GAME::SettingsScreen>();
			if (!settingsView.empty()) {
				GAME::SettingsData* settings = registry.ctx().find<GAME::SettingsData>();
				if (!settings) {
					settings = &registry.ctx().emplace<GAME::SettingsData>();
				}

				closedCount = 0;
				for (auto entity : winView) {
					if (registry.any_of<APP::WindowClosed>(entity))
						++closedCount;
					else {
						auto& gw = registry.get<GW::SYSTEM::GWindow>(entity);
						gw.ProcessWindowEvents();
						DrawSettingsOverlay(gw, registry, *settings);
					}
				}

				if (auto* in = registry.ctx().find<UTIL::Input>()) {
					static bool waitForReleaseBack = true;
					static double lastNavTime = 0;
					static auto navClockStart = std::chrono::steady_clock::now();

					auto now = std::chrono::steady_clock::now();
					double t = std::chrono::duration<double>(now - navClockStart).count();

					float up = 0, down = 0, left = 0, right = 0, back = 0;
					in->immediateInput.GetState(G_KEY_UP, up);
					in->immediateInput.GetState(G_KEY_DOWN, down);
					in->immediateInput.GetState(G_KEY_LEFT, left);
					in->immediateInput.GetState(G_KEY_RIGHT, right);
					in->immediateInput.GetState(G_KEY_BACKSPACE, back);

					// slider navigation
					if ((up > 0.5f || down > 0.5f) && (t - lastNavTime) > 0.15) {
						if (up > 0.5f)
							settings->selectedSlider = (settings->selectedSlider + 2) % 3;
						if (down > 0.5f)
							settings->selectedSlider = (settings->selectedSlider + 1) % 3;
						lastNavTime = t;
					}

					auto adjustValue = [&](int dir)
						{
							const float step = 0.05f;
							float* v = nullptr;
							if (settings->selectedSlider == 0) v = &settings->masterVolume;
							else if (settings->selectedSlider == 1) v = &settings->musicVolume;
							else v = &settings->sfxVolume;

							if (v) {
								*v += dir * step;
								if (*v < 0.0f) *v = 0.0f;
								if (*v > 1.0f) *v = 1.0f;
							}
						};

					// move the knob left/right
					if ((left > 0.5f || right > 0.5f) && (t - lastNavTime) > 0.05) {
						if (left > 0.5f)  adjustValue(-1);
						if (right > 0.5f) adjustValue(+1);
						lastNavTime = t;

						// Audio system volume change could go here maybe
					}

#ifdef _WIN32
					// mouse: click/drag on slider bar
					GW::SYSTEM::UNIVERSAL_WINDOW_HANDLE uwh2{};
					if (+registry.get<GW::SYSTEM::GWindow>(*winView.begin()).GetWindowHandle(uwh2)) {
						HWND hwnd2 = reinterpret_cast<HWND>(uwh2.window);
						POINT mp; GetCursorPos(&mp); ScreenToClient(hwnd2, &mp);
						bool mouseDown = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;

						if (mouseDown) {
							for (int i = 0; i < 3; ++i) {
								RECT r = g_settingsUI.sliderRects[i];
								if (PtInRect(&r, mp)) {
									settings->selectedSlider = i;
									float tNorm = float(mp.x - r.left) / float(r.right - r.left);
									if (tNorm < 0.0f) tNorm = 0.0f;
									if (tNorm > 1.0f) tNorm = 1.0f;

									if (i == 0) settings->masterVolume = tNorm;
									else if (i == 1) settings->musicVolume = tNorm;
									else settings->sfxVolume = tNorm;

									// Audio system volume change could go here as well
									break;
								}
							}
						}
					}
#endif
					// BACKSPACE -> return  ( if in maain menu, go back there; if in pause, return to pause )
					if (waitForReleaseBack) {
						if (back < 0.1f)
							waitForReleaseBack = false;
					}
					else if (back > 0.5f) {
						waitForReleaseBack = true;

						// remove all settings entities
						for (auto e : settingsView) registry.destroy(e);

						// if settings was opened from pause menu, go back to pause
						if (registry.ctx().find<GAME::SettingsFromPause>()) {    
							registry.ctx().erase<GAME::SettingsFromPause>();     

							if (auto* pauseState = registry.ctx().find<GAME::PauseMenuState>()) {
								pauseState->active = true;                        
								pauseState->confirm = GAME::PauseMenuState::ConfirmType::None; 
							}
						}
						// otherwise we were in the main menu, return there
						else {                                                     
							auto menu = registry.create();                         
							registry.emplace<GAME::StartScreen>(menu);            
						}
					}
				}

				continue; // skip rest of game loop while in Settings
			}
		}
		// HOW TO PLAY SCREEN
		{
			auto htpView = registry.view<GAME::HowToPlayScreen>();
			if (!htpView.empty()) {
				closedCount = 0;
				for (auto entity : winView) {
					if (registry.any_of<APP::WindowClosed>(entity))
						++closedCount;
					else {
						auto& gw = registry.get<GW::SYSTEM::GWindow>(entity);
						gw.ProcessWindowEvents();
						DrawHowToPlayOverlay(gw, registry);
					}
				}

				if (auto* in = registry.ctx().find<UTIL::Input>()) {
					static bool waitForReleaseBack = true;

					float back = 0;
					in->immediateInput.GetState(G_KEY_BACKSPACE, back);

					// BACKSPACE -> return to main menu
					if (waitForReleaseBack) {
						if (back < 0.1f)
							waitForReleaseBack = false;
					}
					else if (back > 0.5f) {
						waitForReleaseBack = true;

						for (auto e : htpView) registry.destroy(e);
						if (registry.ctx().find<GAME::HowToPlayState>())
							registry.ctx().erase<GAME::HowToPlayState>();

						auto menu = registry.create();
						registry.emplace<GAME::StartScreen>(menu);

						continue; // skip rest of loop this frame
					}

#ifdef _WIN32
					// Mouse clicks on the triangles
					GW::SYSTEM::UNIVERSAL_WINDOW_HANDLE uwh2{};
					if (+registry.get<GW::SYSTEM::GWindow>(*winView.begin()).GetWindowHandle(uwh2)) {
						HWND hwnd2 = reinterpret_cast<HWND>(uwh2.window);
						POINT mp; GetCursorPos(&mp); ScreenToClient(hwnd2, &mp);
						bool mouseDown = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
						static bool mouseDownLastHTP = false;

						if (mouseDown && !mouseDownLastHTP) {
							if (PtInRect(&g_htpUI.nextRect, mp)) {
								if (auto* s = registry.ctx().find<GAME::HowToPlayState>()) {
									s->page = 1; // go to enemy types page
								}
							}
							else if (PtInRect(&g_htpUI.prevRect, mp)) {
								if (auto* s = registry.ctx().find<GAME::HowToPlayState>()) {
									s->page = 0; // go back to controls page
								}
							}
						}
						mouseDownLastHTP = mouseDown;
					}
#endif
				}

				continue; // while in How To Play, skip the rest of the game loop
			}
		}

		// END CREDITS SCREEN
		{
			auto creditsView = registry.view<GAME::EndCreditScreen>();
			if (!creditsView.empty()) {
				using namespace std::chrono;

				// ensure overlay data exists
				if (!registry.ctx().find<GAME::EndCreditsOverlayData>()) {
					registry.ctx().emplace<GAME::EndCreditsOverlayData>(
						GAME::EndCreditsOverlayData{ steady_clock::now() });
				}

				closedCount = 0;
				for (auto entity : winView) {
					if (registry.any_of<APP::WindowClosed>(entity))
						++closedCount;
					else {
						auto& gw = registry.get<GW::SYSTEM::GWindow>(entity);
						gw.ProcessWindowEvents();
						DrawEndCreditsOverlay(gw, registry);
					}
				}

				// allow BACKSPACE to leave early
				if (auto* in = registry.ctx().find<UTIL::Input>()) {
					static bool waitForRelease = true;
					float back = 0;
					in->immediateInput.GetState(G_KEY_BACKSPACE, back);

					if (waitForRelease) {
						if (back < 0.1f) waitForRelease = false;
					}
					else if (back > 0.5f) {
						waitForRelease = true;
						for (auto e : creditsView) registry.destroy(e);
						if (registry.ctx().find<GAME::EndCreditsOverlayData>())
							registry.ctx().erase<GAME::EndCreditsOverlayData>();

						auto menu = registry.create();
						registry.emplace<GAME::StartScreen>(menu);
					}
				}

				// auto-return after full scroll (~35 seconds, tweak as needed)
				if (auto* data = registry.ctx().find<GAME::EndCreditsOverlayData>()) {
					double elapsed = duration<double>(steady_clock::now() - data->start).count();
					if (elapsed > 35.0) {
						for (auto e : creditsView) registry.destroy(e);
						registry.ctx().erase<GAME::EndCreditsOverlayData>();
						auto menu = registry.create();
						registry.emplace<GAME::StartScreen>(menu);
					}
				}

				continue; // skip normal game loop while credits are showing
			}
		}

			// HIGH SCORES SCREEN
			auto hsView = registry.view<GAME::HighScoresScreen>();
			if (!hsView.empty())
			{
				closedCount = 0;
				for (auto entity : winView) {
					if (registry.any_of<APP::WindowClosed>(entity))
						++closedCount;
					else {
						auto& gw = registry.get<GW::SYSTEM::GWindow>(entity);
						gw.ProcessWindowEvents();
					}
				}

				// Draw the high scores first
				GAME::DrawHighScores(registry);

				if (auto* in = registry.ctx().find<UTIL::Input>()) {
					static bool waitForRelease = true;

					float back = 0;
					in->immediateInput.GetState(G_KEY_BACKSPACE, back);

					// Wait until all keys are released before enabling exit
					if (waitForRelease) {
						if (back < 0.1f)
							waitForRelease = false;
						continue;
					}

					// Use only BACKSPACE to return to main menu
					if (back > 0.5f) {
						waitForRelease = true; // reset for next time
						for (auto e : hsView) registry.destroy(e);
						auto menu = registry.create();
						registry.emplace<GAME::StartScreen>(menu);
					}
				}
				continue;
			}

			entt::entity* gameManagerEntity = registry.ctx().find<entt::entity>();

			// set up / find pause state if gameplay exists
			GAME::PauseMenuState* pauseState = nullptr;
			if (gameManagerEntity) {
				pauseState = registry.ctx().find<GAME::PauseMenuState>();
				if (!pauseState) {
					pauseState = &registry.ctx().emplace<GAME::PauseMenuState>();
				}
				// press pause with ESC when not in game over
				if (!registry.ctx().find<GAME::GameOver>()) {
					if (auto* in = registry.ctx().find<UTIL::Input>()) {
						float esc = 0;
						in->immediateInput.GetState(G_KEY_ESCAPE, esc);
						static bool escWasDown = false;
						bool escDown = esc > 0.5f;

						if (escDown && !escWasDown) {
							if (!pauseState->active) {
								// open pause
								pauseState->active = true;
								pauseState->selected = 0;
								pauseState->confirm = GAME::PauseMenuState::ConfirmType::None;
								pauseState->confirmYesSelected = true;
							}
							else {
								// if already open, ESC either cancels confirm or resumes
								if (pauseState->confirm == GAME::PauseMenuState::ConfirmType::None)
									pauseState->active = false;
								else
									pauseState->confirm = GAME::PauseMenuState::ConfirmType::None;
							}
						}
						escWasDown = escDown;
					}
				}
			}
			bool isPaused = pauseState && pauseState->active;

			// Check if reset was requested before updating game over screen
			if (registry.ctx().find<GAME::RequestReset>()) {
				registry.ctx().erase<GAME::RequestReset>();
				if (registry.ctx().find<GAME::GameOverOverlayData>()) {
					registry.ctx().erase<GAME::GameOverOverlayData>();
				}
				GAME::ResetGame(registry);
			}

			// Check if main menu was requested before updating game over screen
			else if (registry.ctx().find<GAME::RequestMainMenu>()) {
				registry.ctx().erase<GAME::RequestMainMenu>();

				// Clear game over state
				if (registry.ctx().find<GAME::GameOver>()) {
					registry.ctx().erase<GAME::GameOver>();
				}
				if (registry.ctx().find<GAME::GameOverOverlayData>()) {
					registry.ctx().erase<GAME::GameOverOverlayData>();
				}

				// Destroy game over screen entities
				auto overView = registry.view<GAME::GameOverScreen>();
				for (auto e : overView) {
					registry.destroy(e);
				}

				// Clean up gameplay entities
				GAME::CleanupGameplayEntities(registry);

				// Create main menu screen
				auto menu = registry.create();
				registry.emplace<GAME::StartScreen>(menu);
			}

			// Game over screen handling
			// Game over handling (initials entry first, then Game Over menu)
			else if (registry.ctx().find<GAME::GameOver>()) {

				// 1) If initials entry is active, run that instead of GameOverScreen
				auto initialsView = registry.view<GAME::InitialsEntryScreen>();
				if (!initialsView.empty()) {
					// We expect just one initials entity
					entt::entity initialsEntity = *initialsView.begin();

					// Process window events and draw initials overlay
					closedCount = 0;
					for (auto winEntity : winView) {
						if (registry.any_of<APP::WindowClosed>(winEntity))
							++closedCount;
						else {
							auto& gw = registry.get<GW::SYSTEM::GWindow>(winEntity);
							gw.ProcessWindowEvents();
							DrawInitialsOverlay(gw, registry);
						}
					}

					// Let the InitialsEntryScreen logic system run
					registry.patch<GAME::InitialsEntryScreen>(initialsEntity);

					// Skip normal game logic / Game Over PNG while entering initials
					continue;
				}

				// 2) No initials entry screen active -> show regular Game Over PNG overlay
				if (!registry.ctx().find<GAME::GameOverOverlayData>()) {
					registry.ctx().emplace<GAME::GameOverOverlayData>(
						GAME::GameOverOverlayData{ std::chrono::steady_clock::now() }
					);
				}

				closedCount = 0;
				for (auto entity : winView) {
					if (registry.any_of<APP::WindowClosed>(entity))
						++closedCount;
					else {
						auto& gw = registry.get<GW::SYSTEM::GWindow>(entity);
						gw.ProcessWindowEvents();
						DrawGameOverOverlay(gw, registry);
					}
				}

				auto overView = registry.view<GAME::GameOverScreen>();
				for (auto entity : overView) {
					registry.patch<GAME::GameOverScreen>(entity);
				}

				continue;
			}

			//pause screen handling 
			else if (isPaused && pauseState) {
				// draw pause overlay and process window events
				closedCount = 0;
				for (auto entity : winView) {
					if (registry.any_of<APP::WindowClosed>(entity))
						++closedCount;
					else {
						auto& gw = registry.get<GW::SYSTEM::GWindow>(entity);
						gw.ProcessWindowEvents();
						DrawPauseOverlay(gw, registry);
					}
				}
				if (auto* in = registry.ctx().find<UTIL::Input>()) {
					static double lastNavTime = 0;
					static auto navClockStart = std::chrono::steady_clock::now();
					auto now = std::chrono::steady_clock::now();
					double t = std::chrono::duration<double>(now - navClockStart).count();

					float up = 0, down = 0, left = 0, right = 0, enter = 0;
					in->immediateInput.GetState(G_KEY_UP, up);
					in->immediateInput.GetState(G_KEY_DOWN, down);
					in->immediateInput.GetState(G_KEY_LEFT, left);
					in->immediateInput.GetState(G_KEY_RIGHT, right);
					in->immediateInput.GetState(G_KEY_ENTER, enter);


					// Navigate menu
					if (pauseState->confirm == GAME::PauseMenuState::ConfirmType::None) {
						// keyboard up/down
						if ((up > 0.5f || down > 0.5f) && (t - lastNavTime) > 0.15) {
							if (up > 0.5f)
								pauseState->selected = (pauseState->selected + 4) % 5; // wrap upwards
							if (down > 0.5f)
								pauseState->selected = (pauseState->selected + 1) % 5; // wrap downwards
							lastNavTime = t;
						}

#ifdef _WIN32
						// mouse selection on pause buttons
						GW::SYSTEM::UNIVERSAL_WINDOW_HANDLE uwh2{};
						if (+registry.get<GW::SYSTEM::GWindow>(*winView.begin()).GetWindowHandle(uwh2)) {
							HWND hwnd2 = reinterpret_cast<HWND>(uwh2.window);
							POINT mp; GetCursorPos(&mp); ScreenToClient(hwnd2, &mp);
							bool mouseDown = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
							static bool mouseDownLastPauseButtons = false;

							if (mouseDown && !mouseDownLastPauseButtons) {
								for (int i = 0; i < 5; ++i) {
									RECT r = g_pauseUI.buttonRects[i];
									if (PtInRect(&r, mp)) {
										pauseState->selected = i;
										enter = 1.0f; // click = ENTER
										break;
									}
								}
							}
							mouseDownLastPauseButtons = mouseDown;
						}
#endif
						if (enter > 0.5f) {
							// Handle menu selection
							switch (pauseState->selected) {
							case 0: // Resume
								pauseState->active = false;
								break;
							case 1: // Restart
								pauseState->confirm = GAME::PauseMenuState::ConfirmType::Restart;
								pauseState->confirmYesSelected = true;
								break;
							case 2: // Main Menu
								pauseState->confirm = GAME::PauseMenuState::ConfirmType::MainMenu;
								pauseState->confirmYesSelected = true;
								break;
							case 3: // Quit
							{
								// Immediately quit the game (keyboard or mouse)
								pauseState->active = false; // hide pause menu

#ifdef _WIN32
								auto winView2 = registry.view<APP::Window>();
								for (auto entity : winView2) {
									auto& gwClose = registry.get<GW::SYSTEM::GWindow>(entity);
									GW::SYSTEM::UNIVERSAL_WINDOW_HANDLE uwh3{};
									if (+gwClose.GetWindowHandle(uwh3)) {
										HWND h = reinterpret_cast<HWND>(uwh3.window);
										PostMessage(h, WM_CLOSE, 0, 0);
									}
								}
#endif
								break;
							}
							case 4: // Settings
							{
								pauseState->active = false; // hide pause menu
								registry.ctx().emplace<GAME::SettingsFromPause>();

								auto s = registry.create();
								registry.emplace<GAME::SettingsScreen>(s);
								break;
							}
							}
						}
					}
					//confirmation dialog navigation
					else {
						// toggle YES/NO with arrows
						  // keyboard: toggle YES/NO
						if ((left > 0.5f || right > 0.5f || up > 0.5f || down > 0.5f) &&
							(t - lastNavTime) > 0.15) {
							pauseState->confirmYesSelected = !pauseState->confirmYesSelected;
							lastNavTime = t;
						}

						bool triggerEnter = (enter > 0.5f);

#ifdef _WIN32
						//mouse click on YES/NO
						GW::SYSTEM::UNIVERSAL_WINDOW_HANDLE uwh2{};
						if (+registry.get<GW::SYSTEM::GWindow>(*winView.begin()).GetWindowHandle(uwh2)) {
							HWND hwnd2 = reinterpret_cast<HWND>(uwh2.window);
							POINT mp; GetCursorPos(&mp); ScreenToClient(hwnd2, &mp);
							bool mouseDown = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
							static bool mouseDownLastPauseYN = false;

							if (mouseDown && !mouseDownLastPauseYN) {
								if (PtInRect(&g_pauseUI.yesRect, mp)) {
									pauseState->confirmYesSelected = true;
									triggerEnter = true;
								}
								else if (PtInRect(&g_pauseUI.noRect, mp)) {
									pauseState->confirmYesSelected = false;
									triggerEnter = true;
								}
							}
							mouseDownLastPauseYN = mouseDown;
						}

#endif 
						if (triggerEnter) {
							bool yesSelected = pauseState->confirmYesSelected;
							auto action = pauseState->confirm;

							// leave confirmation mode
							pauseState->confirm = GAME::PauseMenuState::ConfirmType::None;

							if (!yesSelected) {
							}
							else {
								// YES -> actually do the action
								switch (action) {
								case GAME::PauseMenuState::ConfirmType::Restart:
									registry.ctx().emplace<GAME::RequestReset>();
									pauseState->active = false;
									break;

								case GAME::PauseMenuState::ConfirmType::MainMenu:
									pauseState->active = false;
									{
										auto menu = registry.create();
										registry.emplace<GAME::StartScreen>(menu);
									}
									break;

								case GAME::PauseMenuState::ConfirmType::Quit:
#ifdef _WIN32
									for (auto entity : winView) {
										auto& gwClose = registry.get<GW::SYSTEM::GWindow>(entity);
										GW::SYSTEM::UNIVERSAL_WINDOW_HANDLE uwh3{};
										if (+gwClose.GetWindowHandle(uwh3)) {
											HWND h = reinterpret_cast<HWND>(uwh3.window);
											PostMessage(h, WM_CLOSE, 0, 0);
										}
									}
#endif
									break;

								default:
									break;
								}
							}
						}
					}
				}
				// Don't update normal game logic
				continue;
			}


			// Normal Gameplay
else if (gameManagerEntity) {
	registry.patch<GAME::GameManager>(*gameManagerEntity);
}

closedCount = 0;
for (auto entity : winView) {
	if (registry.any_of<APP::WindowClosed>(entity)) {
		++closedCount;
	}
	else {
		registry.patch<APP::Window>(entity);
	}
}

		if (closedCount > 0) {
#ifdef _WIN32
		HWND console = GetConsoleWindow();
		if (console)
		ExitProcess(0);
#endif
			}
		} while (winView.size() != closedCount);

	}
