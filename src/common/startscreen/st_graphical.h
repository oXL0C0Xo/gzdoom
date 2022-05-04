#pragma once
#include <stdint.h>
#include <functional>

struct BitmapInfoHeader 
{
	uint32_t      biSize;
	int32_t       biWidth;
	int32_t       biHeight;
	uint16_t      biPlanes;
	uint16_t      biBitCount;
	uint32_t      biCompression;
	uint32_t      biSizeImage;
	int32_t       biXPelsPerMeter;
	int32_t       biYPelsPerMeter;
	uint32_t      biClrUsed;
	uint32_t      biClrImportant;
};

struct RgbQuad 
{
	uint8_t    rgbBlue;
	uint8_t    rgbGreen;
	uint8_t    rgbRed;
	uint8_t    rgbReserved;
};


struct BitmapInfo 
{
	BitmapInfoHeader    bmiHeader;
	RgbQuad             bmiColors[1];
};


using InvalidateFunc = std::function<void(BitmapInfo*, int, int, int, int)>;

class FGraphicalStartupScreenRenderer
{
protected:
	int MaxPos, CurPos, NotchPos;
	int NetMaxPos, NetCurPos;
	InvalidateFunc InvalidateRect;	// Urgh. This is the only way to get around the OOP mess of the startup screen code...

	void PlanarToChunky4(uint8_t* dest, const uint8_t* src, int width, int height);
	void DrawBlock(BitmapInfo* bitmap_info, const uint8_t* src, int x, int y, int bytewidth, int height);
	void DrawBlock4(BitmapInfo* bitmap_info, const uint8_t* src, int x, int y, int bytewidth, int height);
	void ClearBlock(BitmapInfo* bitmap_info, uint8_t fill, int x, int y, int bytewidth, int height);
	BitmapInfo* CreateBitmap(int width, int height);
	void FreeBitmap(BitmapInfo* bitmap_info);
	void BitmapColorsFromPlaypal(BitmapInfo* bitmap_info);
	BitmapInfo* AllocTextBitmap();
	void DrawTextScreen(BitmapInfo* bitmap_info, const uint8_t* text_screen);
	int DrawChar(BitmapInfo* screen, int x, int y, unsigned charnum, uint8_t attrib);
	void UpdateTextBlink(BitmapInfo* bitmap_info, const uint8_t* text_screen, bool on);

public:
	BitmapInfo* StartupBitmap;
	int scale;

	FGraphicalStartupScreenRenderer(int max_progress, InvalidateFunc InvalidateRectFunc);
	static uint8_t* BitsForBitmap(BitmapInfo* bitmap_info);
	virtual ~FGraphicalStartupScreenRenderer() = default;
	virtual void Progress();
	virtual void LoadingStatus(const char* message, int colors);
	virtual void AppendStatusLine(const char* status);
	virtual void NetInit(int numplayers);
	virtual void NetProgress(int count);
	virtual void NetDone();
};

class FHereticStartupScreen : public FGraphicalStartupScreenRenderer
{
public:
	FHereticStartupScreen(int max_progress, InvalidateFunc InvalidateRectFunc, int* hr);

	void Progress() override;
	void LoadingStatus(const char *message, int colors);
	void AppendStatusLine(const char *status);
protected:

	int ThermX = 0, ThermY = 0, ThermWidth = 0, ThermHeight = 0;
	int HMsgY = 0, SMsgX = 0;
};

class FHexenStartupScreen : public FGraphicalStartupScreenRenderer
{
public:
	FHexenStartupScreen(int max_progress, InvalidateFunc InvalidateRectFunc, int *hr);
	~FHexenStartupScreen();

	void Progress() override;
	void NetProgress(int count) override;
	void NetDone() override;

	// Hexen's notch graphics, converted to chunky pixels.
	uint8_t * NotchBits;
	uint8_t * NetNotchBits;
};

class FStrifeStartupScreen : public FGraphicalStartupScreenRenderer
{
public:
	FStrifeStartupScreen(int max_progress, InvalidateFunc InvalidateRectFunc, int* hr);
	~FStrifeStartupScreen();

	void Progress();
protected:
	void DrawStuff(int old_laser, int new_laser);

	uint8_t *StartupPics[4+2+1];
};

class FEndoomScreen : public FGraphicalStartupScreenRenderer
{
public:
	FEndoomScreen(InvalidateFunc InvalidateRectFunc, int* hr);
	void Blink();
	bool Blinking() { return blinking; }

protected:
	bool blinking = false, blinkstate = false;
	uint8_t endoom_screen[4000];

};

