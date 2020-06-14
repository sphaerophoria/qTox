#pragma once

#include <QQuickWidget>

class ContentLayout;

class ExperimentalChatForm : public QQuickWidget
{
public:
    ExperimentalChatForm(QWidget* parent = nullptr);

    void show(ContentLayout* contentLayout);
};
