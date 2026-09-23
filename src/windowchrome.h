#ifndef WINDOWCHROME_H
#define WINDOWCHROME_H

#include <QByteArray>
#include <QObject>
#include <QPoint>
#include <QRect>

class QEvent;
class QWidget;

// 无边框主窗口：边缘命中缩放（Windows 走 WM_NCHITTEST；其它平台走鼠标回退）
class WindowChrome : public QObject
{
    Q_OBJECT
public:
    explicit WindowChrome(QWidget *window, QObject *parent = 0);

    bool handleNativeEvent(const QByteArray &eventType, void *message, long *result);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    enum {
        EdgeNone = 0,
        EdgeLeft = 1,
        EdgeRight = 2,
        EdgeTop = 4,
        EdgeBottom = 8
    };

    int hitEdges(const QPoint &globalPos) const;
    void applyCursor(int edges);
    void applyResize(const QPoint &globalPos);

    QWidget *m_window = 0;
    int m_border = 6;
    bool m_resizing = false;
    int m_edges = EdgeNone;
    QPoint m_pressGlobal;
    QRect m_pressGeo;
    int m_hoverEdges = EdgeNone;
};

#endif
