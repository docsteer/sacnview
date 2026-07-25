#ifndef COLOR_HELPERS_H
#define COLOR_HELPERS_H

#include <QColor>

/**
 * Calculate luminance in the sRGB color space.
 *
 * @see https://w3c.github.io/wcag/guidelines/22/#dfn-relative-luminance
 */
double calcLuminance(const QColor & color);

/**
 * Calculate contrast ratio between two colors.
 *
 * @see https://w3c.github.io/wcag/guidelines/22/#dfn-contrast-ratio
 */
double calcContrastRatio(const QColor & color1, const QColor & color2);

/**
 * Find the color in the iterator @p begin .. @p end that contrasts best with @p bgColor.
 *
 * @param bgColor Background color
 * @param begin Start QColor iterator of possible foreground colors.
 * @param end End QColor iterator.
 */
template<class FgColorIt>
QColor findBestContrastingColor(const QColor & bgColor, FgColorIt begin, FgColorIt end)
{
    using ColorContrastRatio = std::tuple<QColor, double>;
    std::vector<ColorContrastRatio> contrastRatios;

    // Calculate ratios.
    for (FgColorIt it = begin; it != end; ++it)
    {
        contrastRatios.emplace_back(*it, calcContrastRatio(bgColor, *it));
    }

    // Find the highest contrast ratio.
    ColorContrastRatio best = contrastRatios.front();
    for (const auto & contrastRatio : contrastRatios)
    {
        if (std::get<1>(best) < std::get<1>(contrastRatio))
        {
            best = contrastRatio;
        }
    }
    return std::get<0>(best);
}

/**
 * Find the which of the application palette's text colors best contrasts with @p bgColor.
 * @param bgColor Background color
 */
QColor findBestContrastingColor(const QColor & bgColor);

#endif //COLOR_HELPERS_H
