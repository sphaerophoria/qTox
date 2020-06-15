#pragma once

#include <QQuickWidget>

class ContentLayout;
class IChatLog;

class ExperimentalChatForm : public QQuickWidget
{
public:
    ExperimentalChatForm(IChatLog& chatLog, QWidget* parent = nullptr);

    void show(ContentLayout* contentLayout);
private:

};
