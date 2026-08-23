#include "dev/RendererDiagnostics.hpp"
#include "platform/Platform.hpp"
#include "renderer/VulkanRenderer.hpp"
#include "dev/GuiDiagnostics.hpp"
#include "input/RenderCursor.hpp"

#include <algorithm>
#include <chrono>

namespace
{
    using Clock =
        std::chrono::steady_clock;

    constexpr int kNormalTileSize = 128;
	constexpr int kAlternateTileSize = 64;

	int gBlackTileX = -1;
	int gBlackTileY = -1;

	Clock::time_point gBlackTileUntil{};

	bool gInvertColors = false;
	bool gPurpleOverride = false;
	bool gAlternateTileSize = false;

	int GetTileSize()
	{
		return
			gAlternateTileSize
				? kAlternateTileSize
				: kNormalTileSize;
	}
}

namespace OpenLRR::Dev::RendererDiagnostics
{
    void Update()
    {
        using namespace OpenLRR::Platform;

        const auto now =
            Clock::now();

        if (gBlackTileUntil != Clock::time_point{} &&
            now >= gBlackTileUntil)
        {
            gBlackTileX = -1;
            gBlackTileY = -1;
            gBlackTileUntil = Clock::time_point{};
        }

        if (WasKeyPressed(Key::F5)) {
			ToggleInvertColors();
		}

        if (!WasMouseButtonPressed(MouseButton::Left)) {
            return;
        }

        OpenLRR::Input::RenderCursorPosition cursor{};

		if (!OpenLRR::Input::GetRenderCursorPosition(
				&cursor))
		{
			return;
		}

		if (OpenLRR::Dev::GuiDiagnostics::BlocksSceneInput(
				cursor.x,
				cursor.y))
		{
			return;
		}

		gBlackTileX =
			static_cast<int>(cursor.x) /
			GetTileSize();

		gBlackTileY =
			static_cast<int>(cursor.y) /
			GetTileSize();

		gBlackTileUntil =
			now + std::chrono::seconds(1);
    }

    void RenderCheckerboard()
    {
        constexpr float normalColors[][4] = {
            { 0.10f, 0.20f, 0.55f, 1.0f },
            { 0.10f, 0.55f, 0.28f, 1.0f },
            { 0.55f, 0.18f, 0.16f, 1.0f },
            { 0.55f, 0.45f, 0.10f, 1.0f }
        };

        constexpr float purple[4] = {
            0.55f,
            0.12f,
            0.55f,
            1.0f
        };

        constexpr float black[4] = {
            0.0f,
            0.0f,
            0.0f,
            1.0f
        };

        const int renderWidth =
            OpenLRR::Renderer::GetRenderWidth();

        const int renderHeight =
            OpenLRR::Renderer::GetRenderHeight();

        const bool shiftDown =
            OpenLRR::Platform::IsKeyDown(
                OpenLRR::Platform::Key::LeftShift) ||
            OpenLRR::Platform::IsKeyDown(
                OpenLRR::Platform::Key::RightShift);

        const bool middleDown =
            OpenLRR::Platform::IsMouseButtonDown(
                OpenLRR::Platform::MouseButton::Middle);

        const bool purpleRedTiles =
			gPurpleOverride ||
			(shiftDown && middleDown);

		const int tileSize =
			GetTileSize();

		for (int y = 0;
			 y < renderHeight;
			 y += tileSize)
		{
			for (int x = 0;
				 x < renderWidth;
				 x += tileSize)
			{
				const int width =
					std::min(
						tileSize,
						renderWidth - x
					);

				const int height =
					std::min(
						tileSize,
						renderHeight - y
					);

				const int tileX =
					x / tileSize;

				const int tileY =
					y / tileSize;

                const int colorIndex =
                    (tileX + (tileY * 3)) % 4;

                const float* color =
					normalColors[colorIndex];

				if (purpleRedTiles &&
					colorIndex == 2)
				{
					color = purple;
				}

				if (tileX == gBlackTileX &&
					tileY == gBlackTileY)
				{
					color = black;
				}

				float red =
					color[0];

				float green =
					color[1];

				float blue =
					color[2];

				const float alpha =
					color[3];

				if (gInvertColors) {
					red =
						1.0f - red;

					green =
						1.0f - green;

					blue =
						1.0f - blue;
				}

				OpenLRR::Renderer::DrawFilledRect(
					x,
					y,
					width,
					height,
					red,
					green,
					blue,
					alpha
				);
            }
        }
    }
	
	void ToggleInvertColors()
	{
		gInvertColors =
			!gInvertColors;
	}

	void TogglePurpleOverride()
	{
		gPurpleOverride =
			!gPurpleOverride;
	}

	void ToggleTileSize()
	{
		gAlternateTileSize =
			!gAlternateTileSize;

		gBlackTileX = -1;
		gBlackTileY = -1;
		gBlackTileUntil = Clock::time_point{};
	}

	void Reset()
	{
		gInvertColors = false;
		gPurpleOverride = false;
		gAlternateTileSize = false;

		gBlackTileX = -1;
		gBlackTileY = -1;
		gBlackTileUntil = Clock::time_point{};
	}

	bool IsInvertColorsEnabled()
	{
		return gInvertColors;
	}

	bool IsPurpleOverrideEnabled()
	{
		return gPurpleOverride;
	}

	bool IsAlternateTileSizeEnabled()
	{
		return gAlternateTileSize;
	}
	
}
