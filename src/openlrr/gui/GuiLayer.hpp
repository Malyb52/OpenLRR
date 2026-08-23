#pragma once

#include <cstddef>
#include <vector>

namespace OpenLRR::Gui
{
    struct Point
    {
        float x = 0.0f;
        float y = 0.0f;
    };

    struct Rect
    {
        float x = 0.0f;
        float y = 0.0f;
        float width = 0.0f;
        float height = 0.0f;

        bool Contains(float pointX, float pointY) const;
    };

    enum class HitShapeType
    {
        Rectangle,
        Polygon
    };

    struct HitShape
    {
        HitShapeType type = HitShapeType::Rectangle;

        Rect rectangle{};

        std::vector<Point> polygon{};
    };

    struct GuiElement
    {
        int id = -1;

        int parentId = -1;

        int zOrder = 0;

        bool visible = true;

        bool blocksSceneInput = true;

        HitShape hitShape{};
    };

    struct HitResult
    {
        bool hit = false;

        int elementId = -1;
    };

    class GuiLayer
    {
    public:
        void Clear();

        void AddElement(const GuiElement& element);

        HitResult HitTest(
            float x,
            float y
        ) const;

        bool BlocksSceneInput(
            float x,
            float y
        ) const;

        const std::vector<GuiElement>& GetElements() const;

    private:
        std::vector<GuiElement> elements_{};
    };
}
