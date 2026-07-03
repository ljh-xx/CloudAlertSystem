#pragma once

#include <afxwin.h>

class CStyledButton : public CButton {
public:
    CStyledButton();

    void SetColors(COLORREF fill, COLORREF text, COLORREF border);

protected:
    COLORREF m_fillColor;
    COLORREF m_textColor;
    COLORREF m_borderColor;

    void DrawItem(LPDRAWITEMSTRUCT drawItem) override;
};
