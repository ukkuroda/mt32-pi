//
/// \file st7789display.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2021  R. Stange <rsta2@o2online.de>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
#ifndef _display_st7789display_h
#define _display_st7789display_h

#include <circle/spimaster.h>
#include <circle/gpiopin.h>
#include <circle/chargenerator.h>
#include <circle/timer.h>
#include <circle/util.h>
#include <circle/types.h>
#include "lcd/lcd.h"

class CST7789 : public CLCD	/// Driver for ST7789-based dot-matrix displays
{
public:
	static const unsigned None = GPIO_PINS;

	typedef u16 TST7789Color;
// really ((green) & 0x3F) << 5, but to have a 0-31 range for all colors
#define ST7789_COLOR(red, green, blue)	  bswap16 (((red) & 0x1F) << 11 \
					| ((green) & 0x1F) << 6 \
					| ((blue) & 0x1F))
#define ST7789_BLACK_COLOR	0
#define ST7789_RED_COLOR	0x00F8
#define ST7789_GREEN_COLOR	0xE007
#define ST7789_BLUE_COLOR	0x1F00
#define ST7789_WHITE_COLOR	0xFFFF

	// CLCD
	bool Initialize();
	TType GetType() const override { return TType::Graphical; };

	// Character functions
	void Clear(bool bImmediate = false);
	void Print(const char* pText, u8 nCursorX, u8 nCursorY, bool bClearLine = false, bool bImmediate = false);

	// Graphics functions
	void SetPixel(u8 nX, u8 nY);
	void ClearPixel(u8 nX, u8 nY);
	void DrawFilledRect(u8 nX1, u8 nY1, u8 nX2, u8 nY2, bool bImmediate = false);
	virtual void DrawChar(char chChar, u8 nCursorX, u8 nCursorY, bool bInverted = false, bool bDoubleWidth = false) override;
	virtual void DrawImage(TImage Image, bool bImmediate = false) override;
	virtual void Flip() override;

	virtual void SetBacklightState(bool bEnabled) override;
	void WriteFrameBuffer(bool bForceFullUpdate);
	void SwapFrameBuffers();

 
	/// \param pSPIMaster Pointer to SPI master object
	/// \param nDCPin GPIO pin number for DC pin
	/// \param nResetPin GPIO pin number for Reset pin (optional)
	/// \param nBackLightPin GPIO pin number for backlight pin (optional)
	/// \param nWidth Display width in number of pixels (default 240)
	/// \param nHeight Display height in number of pixels (default 240)
	/// \param CPOL SPI clock polarity (0 or 1, default 0)
	/// \param CPHA SPI clock phase (0 or 1, default 0)
	/// \param nClockSpeed SPI clock frequency in Hz
	/// \param nChipSelect SPI chip select (if connected, otherwise don't care)
	/// \note GPIO pin numbers are SoC number, not header positions.
	/// \note If SPI chip select is not connected, CPOL should probably be 1.
	CST7789 (CSPIMaster *pSPIMaster,
			unsigned nDCPin, unsigned nResetPin = None, unsigned nBackLightPin = None,
			unsigned nWidth = 240, unsigned nHeight = 240,
			unsigned CPOL = 0, unsigned CPHA = 0, unsigned nClockSpeed = 15000000,
			unsigned nChipSelect = 0, 
			TST7789Color FGColor = ST7789_WHITE_COLOR, TST7789Color BGColor = ST7789_BLACK_COLOR);

	/// \return Display width in number of pixels
	unsigned GetWidth (void) const		{ return m_nWidth; }
	/// \return Display height in number of pixels
	unsigned GetHeight (void) const		{ return m_nHeight; }

	/// \return Operation successful?
//	bool Initialize (void);

	/// \brief Set display on
	void On (void);
	/// \brief Set display off
	void Off (void);


	/// \brief Clear entire display with color
	/// \param Color RGB565 color with swapped bytes (see: ST7789_COLOR())
	void ClearColor (TST7789Color Color = ST7789_BLACK_COLOR);

	/// \brief Set a single pixel to color
	/// \param nPosX X-position (0..width-1)
	/// \param nPosY Y-postion (0..height-1)
	/// \param Color RGB565 color with swapped bytes (see: ST7789_COLOR())
	void SetPixelColor (unsigned nPosX, unsigned nPosY, TST7789Color Color);

	/// \brief Draw an ISO8859-1 string at a specific pixel position
	/// \param nPosX X-position (0..width-1)
	/// \param nPosY Y-postion (0..height-1)
	/// \param pString 0-terminated string of printable characters
	/// \param Color RGB565 foreground color with swapped bytes (see: ST7789_COLOR())
	/// \param BgColor RGB565 background color with swapped bytes (see: ST7789_COLOR())
	void DrawTextColor (unsigned nPosX, unsigned nPosY, const char *pString,
		       TST7789Color Color, TST7789Color BgColor = ST7789_BLACK_COLOR);
	void DrawCharColor(char chChar, u8 nPosX, u8 nPosY, bool bInverted, bool bDoubleWidth, TST7789Color FC, TST7789Color BC);

private:
	void SetWindow (unsigned x0, unsigned y0, unsigned x1, unsigned y1);

	void SendByte (u8 uchByte, boolean bIsData);

	void Command (u8 uchByte)	{ SendByte (uchByte, FALSE); }
	void Data (u8 uchByte)		{ SendByte (uchByte, TRUE); }

	void SendData (const void *pData, size_t nLength);

private:
	CSPIMaster *m_pSPIMaster;
	unsigned m_nResetPin;
	unsigned m_nBackLightPin;
	unsigned m_nWidth;
	unsigned m_nHeight;
	unsigned m_CPOL;
	unsigned m_CPHA;
	unsigned m_nClockSpeed;
	unsigned m_nChipSelect;

	CGPIOPin m_DCPin;
	CGPIOPin m_ResetPin;
	CGPIOPin m_BackLightPin;

	CCharGenerator m_CharGen;
	CTimer *m_pTimer;

	TST7789Color m_FGColor;
	TST7789Color m_BGColor;

	TST7789Color m_FrameBuffers[2][240][240];
	unsigned m_nCurrentFrameBuffer;

};

#endif
