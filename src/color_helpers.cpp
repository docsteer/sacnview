#include "color_helpers.h"
#include <QApplication>
#include <QPalette>

double calcLuminance(const QColor & color)
{
    // https://w3c.github.io/wcag/guidelines/22/#dfn-relative-luminance
    return 0.2126 * color.redF() + 0.7152 * color.greenF() + 0.0722 * color.blueF();
}

double calcContrastRatio(const QColor & color1, const QColor & color2)
{
    const double luminance1 = calcLuminance(color1);
    const double luminance2 = calcLuminance(color2);
    const double lighter = std::max(luminance1, luminance2);
    const double darker = std::min(luminance1, luminance2);
    // https://w3c.github.io/wcag/guidelines/22/#dfn-contrast-ratio
    return (lighter + 0.05) / (darker + 0.05);
}

QColor findBestContrastingColor(const QColor & bgColor)
{
    const QPalette palette = qApp->palette();
    const std::array fgColors = {palette.color(QPalette::Text), palette.color(QPalette::BrightText)};
    return findBestContrastingColor(bgColor, fgColors.cbegin(), fgColors.cend());
}
