#pragma once

#include <QQuickWidget>

class ContentLayout;
class IChatLog;
class IMessageDispatcher;
class ExperimentalChatLogModel;

class ExperimentalChatForm : public QQuickWidget
{
public:
    ExperimentalChatForm(IChatLog& chatLog, IMessageDispatcher& messageDispatcher, QWidget* parent = nullptr);

    void show(ContentLayout* contentLayout);
private:
    ExperimentalChatLogModel* chatLogModel;
    IMessageDispatcher& messageDispatcher;
};
