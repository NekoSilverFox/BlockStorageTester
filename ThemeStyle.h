#ifndef THEMESTYLE_H
#define THEMESTYLE_H

#include <QString>

namespace ThemeStyle
{
    const QString LABLE_RED     = "color: red;";
    const QString LABLE_ORANGE  = "color: orange;";
    const QString LABLE_GREEN   = "color: green;";
    const QString LABLE_BLUE    = "color: blue;";
    const QString LCD_NONE      = "NONE";

    const unsigned int SCATTER_MARK_SIZE    =   10;
    const unsigned int FONT_TITLE_SIZE      =   16;  // 绘图区标题字体大小
    const unsigned int FONT_POINT_DEFALUT_SIZE      =   12;  // 鼠标不悬停时候字体大小
    const unsigned int FONT_POINT_HOVER_SIZE      =   16;  // 鼠标悬停时候字体大小
    const bool         FONT_TITLE_BOLD      =   false;// 标题字体是否加粗
}

#endif // THEMESTYLE_H
