#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "StyledButton.h"

static int ClampChannel(int value) {
    return value < 0 ? 0 : value;
}

CStyledButton::CStyledButton()
    : m_fillColor(RGB(245, 247, 250)),
      m_textColor(RGB(20, 24, 32)),
      m_borderColor(RGB(160, 168, 180)) {}

void CStyledButton::SetColors(COLORREF fill, COLORREF text, COLORREF border) {
    m_fillColor = fill;
    m_textColor = text;
    m_borderColor = border;
    if (::IsWindow(GetSafeHwnd())) Invalidate();
}

void CStyledButton::DrawItem(LPDRAWITEMSTRUCT drawItem) {
    CDC dc;
    dc.Attach(drawItem->hDC);

    CRect rc(drawItem->rcItem);
    bool disabled = (drawItem->itemState & ODS_DISABLED) != 0;
    bool pressed = (drawItem->itemState & ODS_SELECTED) != 0;

    COLORREF fill = disabled ? RGB(232, 236, 241) : m_fillColor;
    COLORREF text = disabled ? RGB(140, 146, 156) : m_textColor;
    COLORREF border = disabled ? RGB(190, 196, 204) : m_borderColor;

    if (pressed && !disabled) {
        fill = RGB(
            ClampChannel(GetRValue(fill) - 18),
            ClampChannel(GetGValue(fill) - 18),
            ClampChannel(GetBValue(fill) - 18));
    }

    dc.FillSolidRect(rc, fill);
    CPen borderPen(PS_SOLID, 1, border);
    CPen* oldPen = dc.SelectObject(&borderPen);
    dc.SelectStockObject(NULL_BRUSH);
    dc.Rectangle(rc);
    dc.SelectObject(oldPen);

    CString textValue;
    GetWindowText(textValue);
    dc.SetBkMode(TRANSPARENT);
    dc.SetTextColor(text);
    if (pressed && !disabled) rc.OffsetRect(1, 1);
    dc.DrawText(textValue, rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

    if (drawItem->itemState & ODS_FOCUS) {
        CRect focus = drawItem->rcItem;
        focus.DeflateRect(4, 4);
        dc.DrawFocusRect(focus);
    }

    dc.Detach();
}
