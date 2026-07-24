#ifndef RECORDSWINDOW_H
#define RECORDSWINDOW_H

#include "SubWindow.h"
#include <QListWidget>

class RecordsWindow : public SubWindow {
    Q_OBJECT
public:
    explicit RecordsWindow(QWidget* parent = nullptr);

    void loadRecords();

private:
    QListWidget* m_list;
};

#endif // RECORDSWINDOW_H
