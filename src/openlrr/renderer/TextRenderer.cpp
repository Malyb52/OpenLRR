#include "renderer/TextRenderer.hpp"

#include "renderer/VulkanRenderer.hpp"

#include <cstddef>

namespace
{
    constexpr int GlyphWidth = 5;
    constexpr int GlyphHeight = 7;
    constexpr int GlyphSpacing = 1;

    struct Glyph
    {
        char character;
        unsigned char rows[GlyphHeight];
    };

    constexpr Glyph Glyphs[] = {
        { ' ', { 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000 } },

        { '0', { 0b01110, 0b10001, 0b10011, 0b10101, 0b11001, 0b10001, 0b01110 } },
        { '1', { 0b00100, 0b01100, 0b00100, 0b00100, 0b00100, 0b00100, 0b01110 } },
        { '2', { 0b01110, 0b10001, 0b00001, 0b00010, 0b00100, 0b01000, 0b11111 } },
        { '3', { 0b11110, 0b00001, 0b00001, 0b01110, 0b00001, 0b00001, 0b11110 } },
        { '4', { 0b00010, 0b00110, 0b01010, 0b10010, 0b11111, 0b00010, 0b00010 } },
        { '5', { 0b11111, 0b10000, 0b10000, 0b11110, 0b00001, 0b00001, 0b11110 } },
        { '6', { 0b01110, 0b10000, 0b10000, 0b11110, 0b10001, 0b10001, 0b01110 } },
        { '7', { 0b11111, 0b00001, 0b00010, 0b00100, 0b01000, 0b01000, 0b01000 } },
        { '8', { 0b01110, 0b10001, 0b10001, 0b01110, 0b10001, 0b10001, 0b01110 } },
        { '9', { 0b01110, 0b10001, 0b10001, 0b01111, 0b00001, 0b00001, 0b01110 } },

        { 'A', { 0b01110, 0b10001, 0b10001, 0b11111, 0b10001, 0b10001, 0b10001 } },
        { 'B', { 0b11110, 0b10001, 0b10001, 0b11110, 0b10001, 0b10001, 0b11110 } },
        { 'C', { 0b01111, 0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b01111 } },
        { 'D', { 0b11110, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b11110 } },
        { 'E', { 0b11111, 0b10000, 0b10000, 0b11110, 0b10000, 0b10000, 0b11111 } },
        { 'F', { 0b11111, 0b10000, 0b10000, 0b11110, 0b10000, 0b10000, 0b10000 } },
        { 'G', { 0b01111, 0b10000, 0b10000, 0b10111, 0b10001, 0b10001, 0b01111 } },
        { 'H', { 0b10001, 0b10001, 0b10001, 0b11111, 0b10001, 0b10001, 0b10001 } },
        { 'I', { 0b11111, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b11111 } },
        { 'J', { 0b00111, 0b00010, 0b00010, 0b00010, 0b00010, 0b10010, 0b01100 } },
        { 'K', { 0b10001, 0b10010, 0b10100, 0b11000, 0b10100, 0b10010, 0b10001 } },
        { 'L', { 0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b11111 } },
        { 'M', { 0b10001, 0b11011, 0b10101, 0b10101, 0b10001, 0b10001, 0b10001 } },
        { 'N', { 0b10001, 0b11001, 0b10101, 0b10011, 0b10001, 0b10001, 0b10001 } },
        { 'O', { 0b01110, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01110 } },
        { 'P', { 0b11110, 0b10001, 0b10001, 0b11110, 0b10000, 0b10000, 0b10000 } },
        { 'Q', { 0b01110, 0b10001, 0b10001, 0b10001, 0b10101, 0b10010, 0b01101 } },
        { 'R', { 0b11110, 0b10001, 0b10001, 0b11110, 0b10100, 0b10010, 0b10001 } },
        { 'S', { 0b01111, 0b10000, 0b10000, 0b01110, 0b00001, 0b00001, 0b11110 } },
        { 'T', { 0b11111, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100 } },
        { 'U', { 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01110 } },
        { 'V', { 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01010, 0b00100 } },
        { 'W', { 0b10001, 0b10001, 0b10001, 0b10101, 0b10101, 0b10101, 0b01010 } },
        { 'X', { 0b10001, 0b10001, 0b01010, 0b00100, 0b01010, 0b10001, 0b10001 } },
        { 'Y', { 0b10001, 0b10001, 0b01010, 0b00100, 0b00100, 0b00100, 0b00100 } },
        { 'Z', { 0b11111, 0b00001, 0b00010, 0b00100, 0b01000, 0b10000, 0b11111 } },

        { ':', { 0b00000, 0b00100, 0b00100, 0b00000, 0b00100, 0b00100, 0b00000 } },
        { '.', { 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00100, 0b00100 } },
        { '-', { 0b00000, 0b00000, 0b00000, 0b11111, 0b00000, 0b00000, 0b00000 } },
        { '/', { 0b00001, 0b00010, 0b00100, 0b01000, 0b10000, 0b00000, 0b00000 } }
    };

    const Glyph* FindGlyph(
        char character
    )
    {
        if (character >= 'a' &&
            character <= 'z')
        {
            character =
                static_cast<char>(
                    character - 'a' + 'A'
                );
        }

        for (const Glyph& glyph : Glyphs)
        {
            if (glyph.character == character) {
                return &glyph;
            }
        }

        return nullptr;
    }
}

namespace OpenLRR::Renderer
{
    void DrawText(
        const char* text,
        int x,
        int y,
        int pixelSize,
        float red,
        float green,
        float blue,
        float alpha
    )
    {
        if (text == nullptr ||
            pixelSize <= 0)
        {
            return;
        }

        int cursorX =
            x;

        for (const char* character = text;
             *character != '\0';
             ++character)
        {
            const Glyph* glyph =
                FindGlyph(*character);

            if (glyph != nullptr)
            {
                for (int row = 0;
                     row < GlyphHeight;
                     ++row)
                {
                    for (int column = 0;
                         column < GlyphWidth;
                         ++column)
                    {
                        const unsigned char mask =
                            static_cast<unsigned char>(
                                1u <<
                                (GlyphWidth - 1 - column)
                            );

                        if ((glyph->rows[row] & mask) == 0) {
                            continue;
                        }

                        DrawFilledRect(
                            cursorX +
                                column * pixelSize,
                            y +
                                row * pixelSize,
                            pixelSize,
                            pixelSize,
                            red,
                            green,
                            blue,
                            alpha
                        );
                    }
                }
            }

            cursorX +=
                (GlyphWidth + GlyphSpacing) *
                pixelSize;
        }
    }

    int MeasureTextWidth(
        const char* text,
        int pixelSize
    )
    {
        if (text == nullptr ||
            pixelSize <= 0)
        {
            return 0;
        }

        std::size_t length = 0;

        while (text[length] != '\0') {
            ++length;
        }

        if (length == 0) {
            return 0;
        }

        return
            static_cast<int>(
                length *
                (GlyphWidth + GlyphSpacing) *
                pixelSize -
                GlyphSpacing * pixelSize
            );
    }

    int GetTextHeight(
        int pixelSize
    )
    {
        if (pixelSize <= 0) {
            return 0;
        }

        return
            GlyphHeight *
            pixelSize;
    }
}
