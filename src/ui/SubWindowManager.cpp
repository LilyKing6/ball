#include "SubWindowManager.h"
#include "SubWindow.h"
#include <QPainter>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QResizeEvent>

SubWindowManager::SubWindowManager(QWidget* parent) : QWidget(parent) {
    setAttribute(Qt::WA_TransparentForMouseEvents, false);
    hide();
}

void SubWindowManager::showWindow(SubWindow* window) {
    if (!window) return;
    if (!m_stack.isEmpty()) {
        m_stack.top()->hide();
    }
    m_stack.push(window);
    window->setParent(this);
    window->move((width() - window->width()) / 2, (height() - window->height()) / 2);
    connect(window, &SubWindow::closed, this, [this]() {
        if (!m_stack.isEmpty()) m_stack.pop();
        if (m_stack.isEmpty()) hide();
        else { m_stack.top()->showAnimated(); }
    });
    show();
    raise();
    window->showAnimated();
}

void SubWindowManager::closeTopWindow() {
    if (!m_stack.isEmpty()) {
        m_stack.top()->hideAnimated();
    }
}

void SubWindowManager::clearAll() {
    while (!m_stack.isEmpty()) {
        SubWindow* w = m_stack.pop();
        w->hide();
        w->disconnect(this);
    }
    hide();
}

void SubWindowManager::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.fillRect(rect(), QColor(0, 0, 0, 120));
}

void SubWindowManager::mousePressEvent(QMouseEvent*) {
    closeTopWindow();
}

void SubWindowManager::keyPressEvent(QKeyEvent* e) {
    if (e->key() == Qt::Key_Escape) closeTopWindow();
}

void SubWindowManager::resizeEvent(QResizeEvent* e) {
    QWidget::resizeEvent(e);
    if (!m_stack.isEmpty()) {
        auto* w = m_stack.top();
        w->move((width() - w->width()) / 2, (height() - w->height()) / 2);
    }
}
