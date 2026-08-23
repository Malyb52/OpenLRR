#include "dev/GuiDiagnostics.hpp"
#include "gui/GuiLayer.hpp"
#include "renderer/VulkanRenderer.hpp"
#include "dev/RendererDiagnostics.hpp"
#include "platform/Platform.hpp"
#include "input/RenderCursor.hpp"

namespace
{
    OpenLRR::Gui::GuiLayer gGuiLayer{};

    int gLayoutWidth = -1;
    int gLayoutHeight = -1;

    enum ElementId
    {
        Minimap = 1,

        PanelLeft,
        PanelMiddle,
        PanelRight,

        ButtonInvert,
        ButtonPurple,
        ButtonTileSize,
        ButtonReset
    };

    void AddRectangle(
        int id,
        int zOrder,
        float x,
        float y,
        float width,
        float height,
        bool blocksSceneInput
    )
    {
        OpenLRR::Gui::GuiElement element{};

        element.id =
            id;

        element.zOrder =
            zOrder;

        element.blocksSceneInput =
            blocksSceneInput;

        element.hitShape.type =
            OpenLRR::Gui::HitShapeType::Rectangle;

        element.hitShape.rectangle = {
            x,
            y,
            width,
            height
        };

        gGuiLayer.AddElement(
            element
        );
    }

    void BuildLayout()
    {
        const int renderWidth =
            OpenLRR::Renderer::GetRenderWidth();

        const int renderHeight =
            OpenLRR::Renderer::GetRenderHeight();

        if (renderWidth <= 0 ||
            renderHeight <= 0)
        {
            return;
        }

        if (renderWidth == gLayoutWidth &&
            renderHeight == gLayoutHeight)
        {
            return;
        }

        gLayoutWidth =
            renderWidth;

        gLayoutHeight =
            renderHeight;

        gGuiLayer.Clear();

        // Top-left minimap placeholder.
        const float minimapWidth =
            static_cast<float>(renderWidth) *
            0.16f;

        const float minimapHeight =
            minimapWidth *
            0.75f;

        AddRectangle(
            Minimap,
            10,
            12.0f,
            12.0f,
            minimapWidth,
            minimapHeight,
            true
        );

        // Bottom-centre control panel.
        const float panelWidth =
            static_cast<float>(renderWidth) *
            0.34f;

        const float panelHeight =
            static_cast<float>(renderHeight) *
            0.12f;

        const float panelX =
            (static_cast<float>(renderWidth) -
             panelWidth) *
            0.5f;

        const float panelY =
            static_cast<float>(renderHeight) -
            panelHeight -
            12.0f;

        // Three overlapping rectangles make a deliberately
        // non-rectangular panel footprint for now.
        AddRectangle(
            PanelLeft,
            10,
            panelX,
            panelY + panelHeight * 0.18f,
            panelWidth * 0.25f,
            panelHeight * 0.64f,
            true
        );

        AddRectangle(
            PanelMiddle,
            10,
            panelX + panelWidth * 0.18f,
            panelY,
            panelWidth * 0.64f,
            panelHeight,
            true
        );

        AddRectangle(
            PanelRight,
            10,
            panelX + panelWidth * 0.75f,
            panelY + panelHeight * 0.18f,
            panelWidth * 0.25f,
            panelHeight * 0.64f,
            true
        );

        const float buttonWidth =
            panelWidth *
            0.16f;

        const float buttonHeight =
            panelHeight *
            0.42f;

        const float buttonY =
            panelY +
            (panelHeight - buttonHeight) *
            0.5f;

        const float buttonGap =
            panelWidth *
            0.035f;

        const float buttonsTotalWidth =
            buttonWidth * 4.0f +
            buttonGap * 3.0f;

        const float firstButtonX =
            panelX +
            (panelWidth - buttonsTotalWidth) *
            0.5f;

        AddRectangle(
            ButtonInvert,
            20,
            firstButtonX,
            buttonY,
            buttonWidth,
            buttonHeight,
            true
        );

        AddRectangle(
            ButtonPurple,
            20,
            firstButtonX +
                (buttonWidth + buttonGap),
            buttonY,
            buttonWidth,
            buttonHeight,
            true
        );

        AddRectangle(
            ButtonTileSize,
            20,
            firstButtonX +
                (buttonWidth + buttonGap) * 2.0f,
            buttonY,
            buttonWidth,
            buttonHeight,
            true
        );

        AddRectangle(
            ButtonReset,
            20,
            firstButtonX +
                (buttonWidth + buttonGap) * 3.0f,
            buttonY,
            buttonWidth,
            buttonHeight,
            true
        );
    }
}

namespace OpenLRR::Dev::GuiDiagnostics
{
    void Update()
	{
		BuildLayout();

		using namespace OpenLRR::Platform;

		if (!WasMouseButtonPressed(
				MouseButton::Left))
		{
			return;
		}

		OpenLRR::Input::RenderCursorPosition cursor{};

		if (!OpenLRR::Input::GetRenderCursorPosition(
				&cursor))
		{
			return;
		}

		const OpenLRR::Gui::HitResult hit =
			gGuiLayer.HitTest(
				static_cast<float>(cursor.x),
				static_cast<float>(cursor.y)
			);

		if (!hit.hit) {
			return;
		}

		switch (hit.elementId)
		{
		case ButtonInvert:
			OpenLRR::Dev::RendererDiagnostics::
				ToggleInvertColors();
			break;

		case ButtonPurple:
			OpenLRR::Dev::RendererDiagnostics::
				TogglePurpleOverride();
			break;

		case ButtonTileSize:
			OpenLRR::Dev::RendererDiagnostics::
				ToggleTileSize();
			break;

		case ButtonReset:
			OpenLRR::Dev::RendererDiagnostics::
				Reset();
			break;

		default:
			break;
		}
	}

    bool BlocksSceneInput(
        double renderX,
        double renderY
    )
    {
        BuildLayout();

        return gGuiLayer.BlocksSceneInput(
            static_cast<float>(renderX),
            static_cast<float>(renderY)
        );
    }

    void Render()
    {
        BuildLayout();

        const auto& elements =
            gGuiLayer.GetElements();

        for (const auto& element :
             elements)
        {
            if (!element.visible) {
                continue;
            }

            if (element.hitShape.type !=
                OpenLRR::Gui::HitShapeType::Rectangle)
            {
                continue;
            }

            const auto& rect =
                element.hitShape.rectangle;

            float red = 0.18f;
            float green = 0.18f;
            float blue = 0.20f;

            if (element.id == Minimap) {
				red = 0.08f;
				green = 0.22f;
				blue = 0.12f;
			}
			else if (element.id == ButtonInvert) {
				red = 0.70f;
				green = 0.58f;
				blue = 0.08f;

				if (OpenLRR::Dev::RendererDiagnostics::
						IsInvertColorsEnabled())
				{
					red = 0.95f;
					green = 0.90f;
					blue = 0.25f;
				}
			}
			else if (element.id == ButtonPurple) {
				red = 0.42f;
				green = 0.10f;
				blue = 0.48f;

				if (OpenLRR::Dev::RendererDiagnostics::
						IsPurpleOverrideEnabled())
				{
					red = 0.72f;
					green = 0.20f;
					blue = 0.80f;
				}
			}
			else if (element.id == ButtonTileSize) {
				red = 0.10f;
				green = 0.28f;
				blue = 0.62f;

				if (OpenLRR::Dev::RendererDiagnostics::
						IsAlternateTileSizeEnabled())
				{
					red = 0.18f;
					green = 0.52f;
					blue = 0.92f;
				}
			}
			else if (element.id == ButtonReset) {
				red = 0.62f;
				green = 0.10f;
				blue = 0.10f;
			}
			else {
				// Panel pieces.
				red = 0.18f;
				green = 0.18f;
				blue = 0.20f;
			}

            OpenLRR::Renderer::DrawFilledRect(
                static_cast<int>(rect.x),
                static_cast<int>(rect.y),
                static_cast<int>(rect.width),
                static_cast<int>(rect.height),
                red,
                green,
                blue,
                1.0f
            );
        }
    }
}
