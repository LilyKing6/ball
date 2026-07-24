#ifndef SUBWINDOW_H
#define SUBWINDOW_H

#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QPropertyAnimation>

class SubWindow : public QWidget {
    Q_OBJECT
    Q_PROPERTY(float windowOpacity READ windowOpacity WRITE setWindowOpacity)
public:
    explicit SubWindow(const QString& title, QWidget* parent = nullptr, int w = 500, int h = 400);

    void showAnimated();
    void hideAnimated();

    QVBoxLayout* contentLayout() const { return m_contentLayout; }

signals:
    void closed();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    QLabel* m_titleLabel;
    QPushButton* m_closeBtn;
    QVBoxLayout* m_contentLayout;
    QPropertyAnimation* m_anim;
    float m_opacity = 0.0f;
    bool m_dragging = false;
    QPoint m_dragStart;
};

#endif // SUBWINDOW_H
