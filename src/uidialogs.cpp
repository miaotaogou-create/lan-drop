#include "uidialogs.h"

#include <QDialog>
#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
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

static QString cardDialogStyle()
{
    return QStringLiteral(
        "#appDlg { background: transparent; }"
        "#appDlgRoot { background: #ffffff; border: 1px solid #e2e8f0; border-radius: 16px; }"
        "#appDlgTitle { color: #0f172a; font-size: 14px; font-weight: 700; background: transparent; }"
        "#appDlgBody { color: #475569; font-size: 13px; background: transparent; }"
        "#appDlgPrimary { background: #2563eb; border: none; border-radius: 8px; color: #ffffff;"
        " padding: 8px 16px; font-weight: 600; }"
        "#appDlgPrimary:hover { background: #1d4ed8; }"
        "#appDlgSecondary { background: #ffffff; border: 1px solid #e2e8f0; border-radius: 8px;"
        " color: #334155; padding: 8px 14px; font-weight: 600; }"
        "#appDlgSecondary:hover { background: #f8fafc; }"
        "#appDlgGhost { background: #f1f5f9; border: 1px solid #e2e8f0; border-radius: 8px;"
        " color: #64748b; padding: 8px 14px; font-weight: 600; }"
        "#appDlgGhost:hover { background: #e2e8f0; color: #334155; }");
}

static int runCardDialog(QWidget *parent, const QString &text, const QStringList &labels,
                         int defaultIndex, bool firstIsPrimary)
{
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
    // 按钮从左到右：确认、取消；默认焦点在取消（安全）
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
