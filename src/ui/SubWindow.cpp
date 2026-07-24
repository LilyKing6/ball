#include "SubWindow.h"
#include "Style.h"
#include <QPainter>
#include <QMouseEvent>
#include <QGraphicsOpacityEffect>

SubWindow::SubWindow(const QString& title, QWidget* parent, int w, int h)
    : QWidget(parent) {
    setFixedSize(w, h);
    setWindowFlags(Qt::FramelessWindowHint | Qt::SubWindow);
    setAttribute(Qt::WA_TranslucentBackground);
    hide();

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // 标题栏
    auto* titleBar = new QWidget(this);
    titleBar->setFixedHeight(44);
    titleBar->setStyleSheet(
        "background: rgba(15, 25, 50, 0.92); "
        "border-top-left-radius: 12px; "
        "border-top-right-radius: 12px;"
    );
    auto* titleLayout = new QHBoxLayout(titleBar);
    titleLayout->setContentsMargins(18, 0, 12, 0);

    m_titleLabel = new QLabel(title, titleBar);
    m_titleLabel->setStyleSheet(Style::labelStyle(Style::accentGold(), 15, true));
    titleLayout->addWidget(m_titleLabel);
    titleLayout->addStretch();

    m_closeBtn = new QPushButton("✕", titleBar);
    m_closeBtn->setFixedSize(28, 28);
    Style::applyIconButton(m_closeBtn, "#aaa");
    m_closeBtn->setStyleSheet(R"(
        QPushButton {
            color: #aaa;
            background: transparent;
            border: none;
            font-size: 16px;
            border-radius: 14px;
        }
        QPushButton:hover {
            color: #ff4444;
            background: rgba(255,68,68,0.15);
        }
    )");
    connect(m_closeBtn, &QPushButton::clicked, this, [this]() { hideAnimated(); });
    titleLayout->addWidget(m_closeBtn);

    mainLayout->addWidget(titleBar);

    // 内容区
    auto* contentArea = new QWidget(this);
    contentArea->setStyleSheet(
        "background: rgba(22, 33, 62, 0.92); "
        "border-bottom-left-radius: 12px; "
        "border-bottom-right-radius: 12px;"
    );
    m_contentLayout = new QVBoxLayout(contentArea);
    m_contentLayout->setContentsMargins(22, 18, 22, 22);
    m_contentLayout->setSpacing(10);
    mainLayout->addWidget(contentArea, 1);

    // 动画
    auto* effect = new QGraphicsOpacityEffect(this);
    effect->setOpacity(0.0);
    setGraphicsEffect(effect);
    m_anim = new QPropertyAnimation(effect, "opacity", this);
    m_anim->setDuration(200);
}

void SubWindow::showAnimated() {
    raise();
    show();
    m_anim->stop();
    m_anim->disconnect();
    m_anim->setStartValue(0.0);
    m_anim->setEndValue(1.0);
    m_anim->start();
}

void SubWindow::hideAnimated() {
    m_anim->stop();
    m_anim->disconnect();
    m_anim->setStartValue(1.0);
    m_anim->setEndValue(0.0);
    connect(m_anim, &QPropertyAnimation::finished, this, [this]() {
        hide();
        emit closed();
    });
    m_anim->start();
}

void SubWindow::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // 绘制底层阴影
    for (int i = 0; i < 3; ++i) {
        QRect shadowRect = rect().adjusted(-2 + i, 2 - i, 2 - i, 4 + i);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(0, 0, 0, 30 - i * 8));
        p.drawRoundedRect(shadowRect, 14, 14);
    }

    // 绘制主体边框和背景
    p.setPen(QPen(QColor(255, 255, 255, 35), 1));
    p.setBrush(QColor(22, 33, 62, 245));
    p.drawRoundedRect(rect().adjusted(1, 1, -1, -1), 12, 12);
}

void SubWindow::mousePressEvent(QMouseEvent* e) {
    if (e->pos().y() < 44) {
        m_dragging = true;
        m_dragStart = e->globalPosition().toPoint() - frameGeometry().topLeft();
    }
}

void SubWindow::mouseMoveEvent(QMouseEvent* e) {
    if (m_dragging) {
        move(e->globalPosition().toPoint() - m_dragStart);
    }
}

void SubWindow::mouseReleaseEvent(QMouseEvent*) {
    m_dragging = false;
}
