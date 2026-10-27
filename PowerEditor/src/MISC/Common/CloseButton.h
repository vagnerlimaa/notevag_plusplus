// This file is part of Notepad++ project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <windows.h>
#include <algorithm>
#include <cmath>

namespace CloseButton
{
	// Rasterize a small vector at the target DPI. Coverage sampling keeps the
	// diagonal strokes and rounded caps smooth without fonts or bitmap scaling.
	inline void draw(HDC dc, const RECT& bounds, UINT dpi, COLORREF foreground, COLORREF hoverBackground, bool hot, bool pressed)
	{
		const float scale = static_cast<float>(dpi) / USER_DEFAULT_SCREEN_DPI;
		const int size = static_cast<int>(std::min({ bounds.right - bounds.left, bounds.bottom - bounds.top, static_cast<LONG>(std::lround(20.0F * scale)) }));
		if (size <= 0)
			return;

		BITMAPINFO info{};
		info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		info.bmiHeader.biWidth = size;
		info.bmiHeader.biHeight = -size;
		info.bmiHeader.biPlanes = 1;
		info.bmiHeader.biBitCount = 32;
		info.bmiHeader.biCompression = BI_RGB;
		void* pixels = nullptr;
		HBITMAP bitmap = ::CreateDIBSection(dc, &info, DIB_RGB_COLORS, &pixels, nullptr, 0);
		if (!bitmap)
			return;
		HDC memory = ::CreateCompatibleDC(dc);
		if (!memory)
		{
			::DeleteObject(bitmap);
			return;
		}
		const auto oldBitmap = ::SelectObject(memory, bitmap);
		auto* output = static_cast<DWORD*>(pixels);
		constexpr int samples = 4;
		constexpr int sampleCount = samples * samples;
		const float halfSize = static_cast<float>(size) / 2.0F;
		const float extent = std::min(4.25F * scale, halfSize * 0.6F);
		const float strokeRadius = 0.75F * scale;
		const float corner = 4.0F * scale;
		const float inner = halfSize - corner;

		for (int y = 0; y < size; ++y)
		{
			for (int x = 0; x < size; ++x)
			{
				unsigned int red = 0, green = 0, blue = 0, covered = 0;
				for (int sy = 0; sy < samples; ++sy)
				{
					for (int sx = 0; sx < samples; ++sx)
					{
						const float px = static_cast<float>(x) + (static_cast<float>(sx) + 0.5F) / samples - halfSize;
						const float py = static_cast<float>(y) + (static_cast<float>(sy) + 0.5F) / samples - halfSize;
						const float alongFirst = std::clamp((px + py) / 2.0F, -extent, extent);
						const float alongSecond = std::clamp((px - py) / 2.0F, -extent, extent);
						const float first = (px - alongFirst) * (px - alongFirst) + (py - alongFirst) * (py - alongFirst);
						const float second = (px - alongSecond) * (px - alongSecond) + (py + alongSecond) * (py + alongSecond);
						const bool glyph = std::min(first, second) <= strokeRadius * strokeRadius;
						const float dx = std::max(0.0F, std::abs(px) - inner);
						const float dy = std::max(0.0F, std::abs(py) - inner);
						if (glyph || (hot && dx * dx + dy * dy <= corner * corner))
						{
							const COLORREF color = glyph ? foreground : hoverBackground;
							// A slightly stronger tint gives feedback while the mouse is held.
							red += !glyph && pressed ? (7U * GetRValue(color) + GetRValue(foreground)) / 8U : GetRValue(color);
							green += !glyph && pressed ? (7U * GetGValue(color) + GetGValue(foreground)) / 8U : GetGValue(color);
							blue += !glyph && pressed ? (7U * GetBValue(color) + GetBValue(foreground)) / 8U : GetBValue(color);
							++covered;
						}
					}
				}
				// AlphaBlend requires premultiplied BGRA pixels.
				output[y * size + x] = ((covered * 255U / sampleCount) << 24) | ((red / sampleCount) << 16) | ((green / sampleCount) << 8) | (blue / sampleCount);
			}
		}
		const BLENDFUNCTION blend{ AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
		::AlphaBlend(dc, bounds.left + (bounds.right - bounds.left - size) / 2, bounds.top + (bounds.bottom - bounds.top - size) / 2,
			size, size, memory, 0, 0, size, size, blend);
		::SelectObject(memory, oldBitmap);
		::DeleteDC(memory);
		::DeleteObject(bitmap);
	}
}
