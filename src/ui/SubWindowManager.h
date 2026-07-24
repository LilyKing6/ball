#ifndef SUBWINDOWMANAGER_H
#define SUBWINDOWMANAGER_H

#include <QWidget>
#include <QStack>

class SubWindow;

class SubWindowManager : public QWidget {
    Q_OBJECT
public:
    explicit SubWindowManager(QWidget* parent = nullptr);

    void showWindow(SubWindow* window);
    void closeTopWindow();
    void clearAll();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    QStack<SubWindow*> m_stack;
};

#endif // SUBWINDOWMANAGER_H
