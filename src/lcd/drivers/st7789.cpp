//
// st7789display.cpp
//
// Based on:
//	https://github.com/pimoroni/st7789-python/blob/master/library/ST7789/__init__.py
//	Copyright (c) 2014 Adafruit Industries
//	Author: Tony DiCola
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
#include "lcd/drivers/st7789.h"
#include "lcd/font6x8.h"

#include "utility.h"
#include <assert.h>

#define ST7789_NOP	0x00
#define ST7789_SWRESET	0x01
#define ST7789_RDDID	0x04
#define ST7789_RDDST	0x09

#define ST7789_SLPIN	0x10
#define ST7789_SLPOUT	0x11
#define ST7789_PTLON	0x12
#define ST7789_NORON	0x13

#define ST7789_INVOFF	0x20
#define ST7789_INVON	0x21
#define ST7789_DISPOFF	0x28
#define ST7789_DISPON	0x29

#define ST7789_CASET	0x2A
#define ST7789_RASET	0x2B
#define ST7789_RAMWR	0x2C
#define ST7789_RAMRD	0x2E

#define ST7789_PTLAR	0x30
#define ST7789_MADCTL	0x36
#define ST7789_COLMOD	0x3A

#define ST7789_FRMCTR1	0xB1
#define ST7789_FRMCTR2	0xB2
#define ST7789_FRMCTR3	0xB3
#define ST7789_INVCTR	0xB4
#define ST7789_DISSET5	0xB6

#define ST7789_GCTRL	0xB7
#define ST7789_GTADJ	0xB8
#define ST7789_VCOMS	0xBB

#define ST7789_LCMCTRL	0xC0
#define ST7789_IDSET	0xC1
#define ST7789_VDVVRHEN	0xC2
#define ST7789_VRHS	0xC3
#define ST7789_VDVS	0xC4
#define ST7789_VMCTR1	0xC5
#define ST7789_FRCTRL2	0xC6
#define ST7789_CABCCTRL	0xC7

#define ST7789_PWCTRL1	0xD0

#define ST7789_RDID1	0xDA
#define ST7789_RDID2	0xDB
#define ST7789_RDID3	0xDC
#define ST7789_RDID4	0xDD

#define ST7789_GMCTRP1	0xE0
#define ST7789_GMCTRN1	0xE1

#define ST7789_PWCTR6	0xFC

CST7789::CST7789 (CSPIMaster *pSPIMaster,
				unsigned nDCPin, unsigned nResetPin, unsigned nBackLightPin,
				unsigned nWidth, unsigned nHeight,
				unsigned CPOL, unsigned CPHA, unsigned nClockSpeed,
				unsigned nChipSelect, TST7789Color FGColor, TST7789Color BGColor)
:	CLCD(nWidth, nHeight),
	m_pSPIMaster (pSPIMaster),
	m_nResetPin (nResetPin),
	m_nBackLightPin (nBackLightPin),
	m_nWidth (nWidth),
	m_nHeight (nHeight),
	m_CPOL (CPOL),
	m_CPHA (CPHA),
	m_nClockSpeed (nClockSpeed),
	m_nChipSelect (nChipSelect),
	m_DCPin (nDCPin, GPIOModeOutput),
	m_pTimer (CTimer::Get ()),
	m_FGColor (FGColor),
	m_BGColor (BGColor),
	m_FrameBuffers{0},
	m_nCurrentFrameBuffer{0}
{
	assert (nDCPin != None);

	if (m_nBackLightPin != None)
	{
		m_BackLightPin.AssignPin (m_nBackLightPin);
		m_BackLightPin.SetMode (GPIOModeOutput, FALSE);
	}

	if (m_nResetPin != None)
	{
		m_ResetPin.AssignPin (m_nResetPin);
		m_ResetPin.SetMode (GPIOModeOutput, FALSE);
	}
}

bool CST7789::Initialize (void) 
{
	assert (m_pSPIMaster != 0);
	assert (m_pTimer != 0);

	if (m_nBackLightPin != None)
	{
		m_BackLightPin.Write (LOW);
		m_pTimer->MsDelay (100);
		m_BackLightPin.Write (HIGH);
	}

	if (m_nResetPin != None)
	{
		m_ResetPin.Write (HIGH);
		m_pTimer->MsDelay (50);
		m_ResetPin.Write (LOW);
		m_pTimer->MsDelay (50);
		m_ResetPin.Write (HIGH);
		m_pTimer->MsDelay (50);
	}

	Command (ST7789_SWRESET);	// Software reset
	m_pTimer->MsDelay (150);

	Command (ST7789_MADCTL);
//	Data (0x70);
	Data (0x00);  // landscape 

	Command (ST7789_FRMCTR2);	// Frame rate ctrl - idle mode
	Data (0x0C);
	Data (0x0C);
	Data (0x00);
	Data (0x33);
	Data (0x33);

	Command (ST7789_COLMOD);
	Data (0x05);

	Command (ST7789_GCTRL);
	Data (0x14);

	Command (ST7789_VCOMS);
	Data (0x37);

	Command (ST7789_LCMCTRL);	// Power control
	Data (0x2C);

	Command (ST7789_VDVVRHEN);	// Power control
	Data (0x01);

	Command (ST7789_VRHS);		// Power control
	Data (0x12);

	Command (ST7789_VDVS);		// Power control
	Data (0x20);

	Command (ST7789_PWCTRL1);
	Data (0xA4);
	Data (0xA1);

	Command (ST7789_FRCTRL2);
	Data (0x0F);

	Command (ST7789_GMCTRP1);	// Set Gamma
	Data (0xD0);
	Data (0x04);
	Data (0x0D);
	Data (0x11);
	Data (0x13);
	Data (0x2B);
	Data (0x3F);
	Data (0x54);
	Data (0x4C);
	Data (0x18);
	Data (0x0D);
	Data (0x0B);
	Data (0x1F);
	Data (0x23);

	Command (ST7789_GMCTRN1);	// Set Gamma
	Data (0xD0);
	Data (0x04);
	Data (0x0C);
	Data (0x11);
	Data (0x13);
	Data (0x2C);
	Data (0x3F);
	Data (0x44);
	Data (0x51);
	Data (0x2F);
	Data (0x1F);
	Data (0x1F);
	Data (0x20);
	Data (0x23);

	Command (ST7789_INVON);		// Invert display

	Command (ST7789_SLPOUT);

	Clear ();

	On ();

	return TRUE;
}



// Drawing constants
//constexpr u8 BarSpacing = 2;

void CST7789::On (void)
{
	assert (m_pTimer != 0);

	Command (ST7789_DISPON);
	m_pTimer->MsDelay (100);
}

void CST7789::Off (void)
{
	Command (ST7789_DISPOFF);
}

/////////////////////

void CST7789::Clear(bool bImmediate)
{
	for (unsigned int y=0; y<m_nHeight; y++) 
	for (unsigned int x=0; x<m_nWidth; x++) 
	{ 
		m_FrameBuffers[m_nCurrentFrameBuffer][y][x] = m_BGColor;
	}
	if(bImmediate) {
		Flip();
	}
}
	


void CST7789::Print(const char* pText, u8 nX, u8 nY, bool bClearLine, bool bImmediate)
{
	unsigned nCharWidth = 6;
	unsigned nCharHeight = 8;
	DrawTextColor(nX*nCharWidth, nY*nCharHeight, pText, m_FGColor, m_BGColor);
}

void CST7789::SetPixel(u8 nX, u8 nY)
{
	SetPixelColor(nX, nY, m_FGColor);
}

void CST7789::ClearPixel(u8 nX, u8 nY)
{
	SetPixelColor(nX, nY, m_BGColor);
}

void CST7789::DrawFilledRect(u8 nX1, u8 nY1, u8 nX2, u8 nY2, bool bImmediate)
{
	for (unsigned int y = nY1; y < nY2; y++)
		for (unsigned int x = nX1; x < nX2; x++)
			SetPixelColor(x, y, m_FGColor);
	if (bImmediate)
		Flip();
}

void CST7789::DrawChar(char chChar, u8 nCursorX, u8 nCursorY, bool bInverted, bool bDoubleWidth)
{
	unsigned nCharWidth = 6;
	unsigned nCharHeight = 8;

	DrawCharColor(chChar, nCursorX * nCharWidth, nCursorY * nCharHeight, m_FGColor, m_BGColor);
};

void CST7789::DrawImage(TImage Image, bool bImmediate)
{
	unsigned nImageWidth = 16;
	unsigned nImageHeight = 32;
	const unsigned char *img;
	switch (Image) {
		case TImage::MT32PiLogo:
			img = MT32PiLogo128x32;
			break;
		case TImage::MisterLogo:
			img = MisterLogo128x32;
			break;
		default:
			return;
	}
			img = MT32PiLogo128x32;

	for (unsigned y = 0; y < nImageHeight; y++)
	for (unsigned x = 0; x < nImageWidth; x++)
	{
		u8 nImageColumn = img[y*nImageWidth + x];
		for (unsigned xx = 0; xx < 8; xx++)
		{
			m_FrameBuffers[m_nCurrentFrameBuffer][y][x*8+xx] = 
				(nImageColumn >> (7-xx)) & 0b00000001 
				? m_FGColor : m_BGColor;
		}
	}
	if (bImmediate)
		Flip();
};

void CST7789::Flip()
{

	SetWindow (0, 0, m_nWidth-1, m_nHeight-1);
	for (unsigned y = 0; y < m_nHeight; y++)
	{
		SendData (m_FrameBuffers[m_nCurrentFrameBuffer][y], sizeof m_FrameBuffers[m_nCurrentFrameBuffer][y]);
	}

	m_nCurrentFrameBuffer = (m_nCurrentFrameBuffer + 1) % 2;
};

void CST7789::SetBacklightState(bool bEnabled)
{

};







void CST7789::SetPixelColor (unsigned nPosX, unsigned nPosY, TST7789Color Color)
{
	m_FrameBuffers[m_nCurrentFrameBuffer][nPosY][nPosX] = Color;
}



void CST7789::DrawCharColor(char chChar, u8 nPosX, u8 nPosY, TST7789Color FC, TST7789Color BC)
{
	unsigned nCharWidth = 6;
	unsigned nCharHeight = 8;
	//TST7789Color Buf[nCharHeight][nCharWidth];

	for (unsigned y = 0; y < nCharHeight; y++)
	{
		u8 nFontColumn = Font6x8[static_cast<u8>(chChar - ' ')][y];
		for (unsigned x = 0; x < nCharWidth; x++)
		{
			m_FrameBuffers[m_nCurrentFrameBuffer][nPosY+y][nPosX+x] = 
				(nFontColumn >> (nCharWidth-x-1)) & 0b00000001 
				? m_FGColor : m_BGColor;
		}
	}

	/*
	SetWindow(nPosX, nPosY, nPosX+nCharWidth-1, nPosY+nCharHeight-1);
	SendData(Buffer, sizeof Buffer);
	*/
};

void CST7789::DrawTextColor (unsigned nPosX, unsigned nPosY, const char *pString,
			       TST7789Color FgColor, TST7789Color BgColor)
{
	assert (pString != 0);

	unsigned nCharWidth = 6;
	//unsigned nCharHeight = 8;


	char chChar;
	while ((chChar = *pString++) != '\0')
	{
		DrawCharColor(chChar, nPosX, nPosY, FgColor, BgColor);
		nPosX += nCharWidth;
	}
}







void CST7789::SetWindow (unsigned x0, unsigned y0, unsigned x1, unsigned y1)
{
	assert (x0 <= x1);
	assert (y0 <= y1);
	assert (x1 < m_nWidth);
	assert (y1 < m_nHeight);

	Command (ST7789_CASET);
	Data (x0 >> 8);
	Data (x0 & 0xFF);
	Data (x1 >> 8);
	Data (x1 & 0xFF);

	Command (ST7789_RASET);
	Data (y0 >> 8);
	Data (y0 & 0xFF);
	Data (y1 >> 8);
	Data (y1 & 0xFF);

	Command (ST7789_RAMWR);
}

void CST7789::SendByte (u8 uchByte, boolean bIsData)
{
	assert (m_pSPIMaster != 0);

	m_DCPin.Write (bIsData ? HIGH : LOW);

	m_pSPIMaster->SetClock (m_nClockSpeed);
	m_pSPIMaster->SetMode (m_CPOL, m_CPHA);

#ifndef NDEBUG
	int nResult =
#endif
		m_pSPIMaster->Write (m_nChipSelect, &uchByte, sizeof uchByte);
	assert (nResult == (int) sizeof uchByte);
}

void CST7789::SendData (const void *pData, size_t nLength)
{
	assert (pData != 0);
	assert (nLength > 0);
	assert (m_pSPIMaster != 0);

	m_DCPin.Write (HIGH);

	m_pSPIMaster->SetClock (m_nClockSpeed);
	m_pSPIMaster->SetMode (m_CPOL, m_CPHA);

#ifndef NDEBUG
	int nResult =
#endif
		m_pSPIMaster->Write (m_nChipSelect, pData, nLength);
	assert (nResult == (int) nLength);
}
