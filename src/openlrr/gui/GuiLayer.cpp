#include "gui/GuiLayer.hpp"

namespace
{
    bool PointInPolygon(
        const std::vector<OpenLRR::Gui::Point>& polygon,
        float x,
        float y
    )
    {
        if (polygon.size() < 3) {
            return false;
        }

        bool inside = false;

        std::size_t previous =
            polygon.size() - 1;

        for (std::size_t current = 0;
             current < polygon.size();
             ++current)
        {
            const auto& a =
                polygon[current];

            const auto& b =
                polygon[previous];

            const bool crosses =
                ((a.y > y) != (b.y > y));

            if (crosses) {
                const float intersectX =
                    (b.x - a.x) *
                    (y - a.y) /
                    (b.y - a.y) +
                    a.x;

                if (x < intersectX) {
                    inside =
                        !inside;
                }
            }

            previous =
                current;
        }

        return inside;
    }

    bool ShapeContains(
        const OpenLRR::Gui::HitShape& shape,
        float x,
        float y
    )
    {
        switch (shape.type)
        {
        case OpenLRR::Gui::HitShapeType::Rectangle:
            return shape.rectangle.Contains(
                x,
                y
            );

        case OpenLRR::Gui::HitShapeType::Polygon:
            return PointInPolygon(
                shape.polygon,
                x,
                y
            );
        }

        return false;
    }
}

namespace OpenLRR::Gui
{
    bool Rect::Contains(
        float pointX,
        float pointY
    ) const
    {
        return
            pointX >= x &&
            pointY >= y &&
            pointX < x + width &&
            pointY < y + height;
    }

    void GuiLayer::Clear()
    {
        elements_.clear();
    }

    void GuiLayer::AddElement(
        const GuiElement& element
    )
    {
        elements_.push_back(
            element
        );
    }

    HitResult GuiLayer::HitTest(
        float x,
        float y
    ) const
    {
        const GuiElement* bestElement =
            nullptr;

        for (const GuiElement& element :
             elements_)
        {
            if (!element.visible) {
                continue;
            }

            if (!ShapeContains(
                    element.hitShape,
                    x,
                    y))
            {
                continue;
            }

            if (bestElement == nullptr ||
                element.zOrder >
                    bestElement->zOrder)
            {
                bestElement =
                    &element;
            }
        }

        if (bestElement == nullptr) {
            return {};
        }

        return {
            true,
            bestElement->id
        };
    }

    bool GuiLayer::BlocksSceneInput(
        float x,
        float y
    ) const
    {
        const GuiElement* bestElement =
            nullptr;

        for (const GuiElement& element :
             elements_)
        {
            if (!element.visible) {
                continue;
            }

            if (!ShapeContains(
                    element.hitShape,
                    x,
                    y))
            {
                continue;
            }

            if (bestElement == nullptr ||
                element.zOrder >
                    bestElement->zOrder)
            {
                bestElement =
                    &element;
            }
        }

        if (bestElement == nullptr) {
            return false;
        }

        return bestElement->
            blocksSceneInput;
    }

    const std::vector<GuiElement>&
    GuiLayer::GetElements() const
    {
        return elements_;
    }
}
