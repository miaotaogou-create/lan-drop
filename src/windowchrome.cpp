#include "windowchrome.h"

#include <QApplication>
#include <QCursor>
#include <QEvent>
#include <QMouseEvent>
#include <QWidget>

#ifdef Q_OS_WIN
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <windows.h>
#endif

WindowChrome::WindowChrome(QWidget *window, QObject *parent)
    : QObject(parent)
    , m_window(window)
{
    if (!m_window)
        return;
    // 子控件会吃掉边缘事件：应用级过滤；Windows 另用 WM_NCHITTEST 交给系统拉边
    qApp->installEventFilter(this);
}

int WindowChrome::hitEdges(const QPoint &globalPos) const
{
    if (!m_window || m_window->isMaximized())
        return EdgeNone;
    const QRect g = m_window->frameGeometry();
    if (!g.contains(globalPos))
        return EdgeNone;
    int e = EdgeNone;
    if (globalPos.x() <= g.left() + m_border)
        e |= EdgeLeft;
    if (globalPos.x() >= g.right() - m_border)
        e |= EdgeRight;
    if (globalPos.y() <= g.top() + m_border)
        e |= EdgeTop;
    if (globalPos.y() >= g.bottom() - m_border)
        e |= EdgeBottom;
    return e;
}

void WindowChrome::applyCursor(int edges)
{
    if (!m_window)
        return;
    if (edges == (EdgeTop | EdgeLeft) || edges == (EdgeBottom | EdgeRight))
        m_window->setCursor(Qt::SizeFDiagCursor);
    else if (edges == (EdgeTop | EdgeRight) || edges == (EdgeBottom | EdgeLeft))
        m_window->setCursor(Qt::SizeBDiagCursor);
    else if (edges & (EdgeLeft | EdgeRight))
        m_window->setCursor(Qt::SizeHorCursor);
    else if (edges & (EdgeTop | EdgeBottom))
        m_window->setCursor(Qt::SizeVerCursor);
    else if (m_hoverEdges != EdgeNone)
        m_window->unsetCursor();
    m_hoverEdges = edges;
}

void WindowChrome::applyResize(const QPoint &globalPos)
{
    if (!m_window || m_edges == EdgeNone)
        return;
    const QPoint d = globalPos - m_pressGlobal;
    QRect geo = m_pressGeo;
    const int minW = m_window->minimumWidth();
    const int minH = m_window->minimumHeight();
    if (m_edges & EdgeLeft) {
        const int x = m_pressGeo.x() + d.x();
        const int w = m_pressGeo.right() - x + 1;
        if (w >= minW) {
            geo.setX(x);
            geo.setWidth(w);
        }
    }
    if (m_edges & EdgeRight)
        geo.setWidth(qMax(minW, m_pressGeo.width() + d.x()));
    if (m_edges & EdgeTop) {
        const int y = m_pressGeo.y() + d.y();
        const int h = m_pressGeo.bottom() - y + 1;
        if (h >= minH) {
            geo.setY(y);
            geo.setHeight(h);
        }
    }
    if (m_edges & EdgeBottom)
        geo.setHeight(qMax(minH, m_pressGeo.height() + d.y()));
    m_window->setGeometry(geo);
}

bool WindowChrome::handleNativeEvent(const QByteArray &eventType, void *message, long *result)
{
#ifdef Q_OS_WIN
    if (!m_window || eventType != "windows_generic_MSG" || !message || !result)
        return false;
    MSG *msg = static_cast<MSG *>(message);
    if (msg->message != WM_NCHITTEST || m_window->isMaximized())
        return false;
    // 仅在真正贴边时交给系统；避免盖住标题栏按钮点击
    const SHORT x = static_cast<SHORT>(LOWORD(msg->lParam));
    const SHORT y = static_cast<SHORT>(HIWORD(msg->lParam));
    const int e = hitEdges(QPoint(x, y));
    if (e == EdgeNone)
        return false;
    if (e == (EdgeTop | EdgeLeft))
        *result = HTTOPLEFT;
    else if (e == (EdgeTop | EdgeRight))
        *result = HTTOPRIGHT;
    else if (e == (EdgeBottom | EdgeLeft))
        *result = HTBOTTOMLEFT;
    else if (e == (EdgeBottom | EdgeRight))
        *result = HTBOTTOMRIGHT;
    else if (e & EdgeLeft)
        *result = HTLEFT;
    else if (e & EdgeRight)
        *result = HTRIGHT;
    else if (e & EdgeTop)
        *result = HTTOP;
    else if (e & EdgeBottom)
        *result = HTBOTTOM;
    else
        return false;
    return true;
#else
    Q_UNUSED(eventType);
    Q_UNUSED(message);
    Q_UNUSED(result);
    return false;
#endif
}

bool WindowChrome::eventFilter(QObject *watched, QEvent *event)
{
    Q_UNUSED(watched);
    if (!m_window || !m_window->isVisible())
        return false;
    switch (event->type()) {
    case QEvent::MouseMove: {
        QMouseEvent *me = static_cast<QMouseEvent *>(event);
        if (m_resizing) {
            applyResize(me->globalPos());
            return true;
        }
        applyCursor(hitEdges(me->globalPos()));
        break;
    }
    case QEvent::MouseButtonPress: {
        QMouseEvent *me = static_cast<QMouseEvent *>(event);
        if (me->button() != Qt::LeftButton || m_window->isMaximized())
            break;
        const int e = hitEdges(me->globalPos());
        if (e == EdgeNone)
            break;
        m_resizing = true;
        m_edges = e;
        m_pressGlobal = me->globalPos();
        m_pressGeo = m_window->geometry();
        applyCursor(e);
        return true;
    }
    case QEvent::MouseButtonRelease: {
        if (!m_resizing)
            break;
        m_resizing = false;
        m_edges = EdgeNone;
        applyCursor(EdgeNone);
        break;
    }
    default:
        break;
    }
    return false;
}
