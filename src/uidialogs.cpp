#include "uidialogs.h"

#include <QColor>
#include <QDialog>
#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QPushButton>
#include <QStyle>
#include <QStyleFactory>
#include <QVBoxLayout>

void applyFloatingShadow(QWidget *w)
{
    if (!w)
        return;
    w->setAttribute(Qt::WA_StyledBackground, true);
    QGraphicsDropShadowEffect *fx = new QGraphicsDropShadowEffect(w);
    fx->setBlurRadius(20);
    fx->setOffset(0, 4);
    fx->setColor(QColor(15, 23, 42, 36));
    w->setGraphicsEffect(fx);
}

void styleAppMenu(QMenu *menu)
{
    if (!menu)
        return;
    if (QStyle *fusion = QStyleFactory::create(QStringLiteral("Fusion")))
        menu->setStyle(fusion);
    menu->setStyleSheet(QStringLiteral(
        "QMenu { background: #ffffff; border: 1px solid #e2e8f0; border-radius: 10px;"
        " padding: 6px; color: #0f172a; }"
        "QMenu::item { padding: 7px 24px 7px 14px; border-radius: 6px; background: transparent;"
        " color: #0f172a; font-size: 12px; }"
        "QMenu::item:selected { background: #eff6ff; color: #1d4ed8; }"
        "QMenu::item:disabled { color: #94a3b8; background: transparent; }"
        "QMenu::separator { height: 1px; background: #eef2f7; margin: 5px 8px; }"
        "QMenu::indicator { width: 14px; height: 14px; margin-left: 8px; }"));
}

QString formDialogChromeQss(const QString &ns, const QString &primarySuffix,
                            const QString &secondarySuffix)
{
    if (ns.isEmpty())
        return QString();
    const QString p = QLatin1Char('#') + ns;
    QString s;
    s += p + QStringLiteral("Dlg { background: transparent; }");
    s += p + QStringLiteral("Root { background: #ffffff; border: 1px solid #e2e8f0; border-radius: 16px; }");
    s += p + QStringLiteral("Head { background: #f8fafc; border-bottom: 1px solid #e2e8f0;"
                            " border-top-left-radius: 16px; border-top-right-radius: 16px; }");
    s += p + QStringLiteral("Title { color: #0f172a; font-size: 14px; font-weight: 700; }");
    s += p + QStringLiteral("Sub { color: #64748b; font-size: 11px; }");
    s += p + QStringLiteral("Close { background: transparent; border: none; border-radius: 6px; padding: 0; }");
    s += p + QStringLiteral("Close:hover { background: #e2e8f0; }");
    s += p + QStringLiteral("Label { color: #334155; font-size: 12px; font-weight: 600; }");
    s += p + QStringLiteral("Field { background: #f8fafc; border: 1px solid #e2e8f0; border-radius: 8px;"
                            " padding: 8px 10px; color: #0f172a; selection-background-color: #bfdbfe; }");
    s += p + QStringLiteral("Field:focus { background: #ffffff; border-color: #3b82f6; }");
    s += p + QStringLiteral("Combo { background: #f8fafc; border: 1px solid #e2e8f0; border-radius: 8px;"
                            " padding: 7px 32px 7px 10px; color: #0f172a; min-height: 20px; }");
    s += p + QStringLiteral("Combo:hover { border-color: #cbd5e1; }");
    s += p + QStringLiteral("Combo:on { background: #ffffff; border-color: #3b82f6; }");
    s += p + QStringLiteral("Combo::drop-down { subcontrol-origin: padding; subcontrol-position: center right;"
                            " width: 28px; border: none; background: transparent; }");
    s += p + QStringLiteral("Combo::down-arrow { image: url(:/icons/chevron-down.svg); width: 12px; height: 12px; }");
    s += p + QStringLiteral("Combo QAbstractItemView { background: #ffffff; border: 1px solid #e2e8f0;"
                            " outline: 0; padding: 4px; selection-background-color: #eff6ff;"
                            " selection-color: #1e3a8a; color: #0f172a; }");
    s += p + QStringLiteral("Foot { border-top: 1px solid #e2e8f0; background: #f8fafc;"
                            " border-bottom-left-radius: 16px; border-bottom-right-radius: 16px; }");
    if (!primarySuffix.isEmpty()) {
        s += p + primarySuffix + QStringLiteral(
            " { background: #2563eb; border: none; border-radius: 8px; color: #ffffff;"
            " padding: 8px 16px; font-weight: 600; }");
        s += p + primarySuffix + QStringLiteral(":hover { background: #1d4ed8; }");
    }
    if (!secondarySuffix.isEmpty()) {
        s += formDialogSecondaryBtnQss(QStringList() << (ns + secondarySuffix));
    }
    return s;
}

QString formDialogSecondaryBtnQss(const QStringList &objectIds)
{
    if (objectIds.isEmpty())
        return QString();
    QStringList sels;
    for (int i = 0; i < objectIds.size(); ++i) {
        QString id = objectIds.at(i).trimmed();
        if (id.isEmpty())
            continue;
        if (!id.startsWith(QLatin1Char('#')))
            id.prepend(QLatin1Char('#'));
        sels.append(id);
    }
    if (sels.isEmpty())
        return QString();
    const QString join = sels.join(QStringLiteral(", "));
    return join + QStringLiteral(
               " { background: #ffffff; border: 1px solid #e2e8f0; border-radius: 8px;"
               " color: #334155; padding: 8px 14px; font-weight: 600; }")
        + join + QStringLiteral(":hover { background: #f8fafc; }");
}

QWidget *showDialogDim(QWidget *anchor)
{
    QWidget *host = anchor ? anchor->window() : 0;
    if (!host)
        return 0;
    QWidget *dim = new QWidget(host);
    dim->setObjectName(QStringLiteral("appDlgDim"));
    dim->setStyleSheet(QStringLiteral(
        "#appDlgDim { background-color: rgba(15, 23, 42, 72); }"));
    dim->setGeometry(host->rect());
    dim->show();
    dim->raise();
    return dim;
}

static QString cardDialogStyle()
{
    return QStringLiteral(
        "#appDlg { background: transparent; }"
        "#appDlgRoot { background: #ffffff; border: 1px solid #e2e8f0; border-radius: 16px; }"
        "#appDlgTitle { color: #0f172a; font-size: 14px; font-weight: 700; background: transparent; }"
        "#appDlgBody { color: #475569; font-size: 13px; background: transparent; }")
        + formDialogSecondaryBtnQss(QStringList()
                                    << QStringLiteral("appDlgSecondary")
                                    << QStringLiteral("appDlgGhost"))
        + QStringLiteral(
              "#appDlgPrimary { background: #2563eb; border: none; border-radius: 8px; color: #ffffff;"
              " padding: 8px 16px; font-weight: 600; }"
              "#appDlgPrimary:hover { background: #1d4ed8; }");
}

static int runCardDialog(QWidget *parent, const QString &text, const QStringList &labels,
                         int defaultIndex, bool firstIsPrimary)
{
    QWidget *dim = showDialogDim(parent);
    QDialog dlg(parent);
    dlg.setObjectName(QStringLiteral("appDlg"));
    dlg.setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    dlg.setAttribute(Qt::WA_TranslucentBackground, true);
    dlg.setModal(true);
    dlg.setMinimumWidth(360);
    dlg.setMaximumWidth(440);
    dlg.setStyleSheet(cardDialogStyle());

    QWidget *root = new QWidget(&dlg);
    root->setObjectName(QStringLiteral("appDlgRoot"));
    applyFloatingShadow(root);
    QVBoxLayout *dlgLay = new QVBoxLayout(&dlg);
    dlgLay->setContentsMargins(16, 16, 16, 16);
    dlgLay->addWidget(root);

    QVBoxLayout *rootLay = new QVBoxLayout(root);
    rootLay->setContentsMargins(22, 20, 22, 18);
    rootLay->setSpacing(14);

    QLabel *title = new QLabel(QString::fromUtf8(u8"局域快传"));
    title->setObjectName(QStringLiteral("appDlgTitle"));
    QLabel *body = new QLabel(text);
    body->setObjectName(QStringLiteral("appDlgBody"));
    body->setWordWrap(true);
    body->setTextInteractionFlags(Qt::TextSelectableByMouse);

    QHBoxLayout *btnLay = new QHBoxLayout;
    btnLay->setSpacing(8);
    btnLay->addStretch(1);

    int result = -1;
    QList<QPushButton *> buttons;
    for (int i = 0; i < labels.size(); ++i) {
        QPushButton *btn = new QPushButton(labels.at(i));
        btn->setCursor(Qt::PointingHandCursor);
        const bool primary = firstIsPrimary && i == 0;
        const bool ghost = (!firstIsPrimary && i == labels.size() - 1)
            || (firstIsPrimary && i == labels.size() - 1 && labels.size() > 1);
        if (primary)
            btn->setObjectName(QStringLiteral("appDlgPrimary"));
        else if (ghost && labels.size() > 1)
            btn->setObjectName(QStringLiteral("appDlgGhost"));
        else
            btn->setObjectName(QStringLiteral("appDlgSecondary"));
        const int idx = i;
        QObject::connect(btn, &QPushButton::clicked, &dlg, [&dlg, &result, idx]() {
            result = idx;
            dlg.accept();
        });
        buttons.append(btn);
        btnLay->addWidget(btn);
    }

    rootLay->addWidget(title);
    rootLay->addWidget(body);
    rootLay->addLayout(btnLay);

    int focusIdx = defaultIndex;
    if (focusIdx < 0 || focusIdx >= buttons.size())
        focusIdx = buttons.isEmpty() ? -1 : (buttons.size() - 1);
    if (focusIdx >= 0 && focusIdx < buttons.size()) {
        buttons.at(focusIdx)->setDefault(true);
        buttons.at(focusIdx)->setFocus(Qt::OtherFocusReason);
    }

    dlg.exec();
    if (dim)
        dim->deleteLater();
    return result;
}

void appInfo(QWidget *parent, const QString &text)
{
    runCardDialog(parent, text, QStringList() << QString::fromUtf8(u8"知道了"), 0, true);
}

void appWarn(QWidget *parent, const QString &text)
{
    runCardDialog(parent, text, QStringList() << QString::fromUtf8(u8"知道了"), 0, true);
}

bool appConfirm(QWidget *parent, const QString &text, const QString &acceptText,
                const QString &cancelText, bool defaultAccept)
{
    const QString ok = acceptText.isEmpty() ? QString::fromUtf8(u8"确定") : acceptText;
    const QString cancel = cancelText.isEmpty() ? QString::fromUtf8(u8"取消") : cancelText;
    const int picked = runCardDialog(parent, text, QStringList() << ok << cancel,
                                     defaultAccept ? 0 : 1, true);
    return picked == 0;
}

int appChoice(QWidget *parent, const QString &text, const QStringList &labels, int defaultIndex)
{
    if (labels.isEmpty())
        return -1;
    return runCardDialog(parent, text, labels, defaultIndex, true);
}
