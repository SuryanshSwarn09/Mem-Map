#pragma once

#include <QWidget>
#include <QHBoxLayout>
#include <QPushButton>
#include <vector>
#include "DiskNode.h"

class BreadcrumbWidget : public QWidget {
    Q_OBJECT

public:
    explicit BreadcrumbWidget(QWidget* parent = nullptr);

    void setRootAndCurrent(DiskNode* scanRoot, DiskNode* currentFolder);

signals:
    void folderSelected(DiskNode* folderNode);
    void upRequested();
    void resetRequested();

private:
    QHBoxLayout* m_layout{nullptr};
    QPushButton* m_btnUp{nullptr};
    QPushButton* m_btnRoot{nullptr};
    DiskNode* m_scanRoot{nullptr};
    DiskNode* m_currentFolder{nullptr};
};
