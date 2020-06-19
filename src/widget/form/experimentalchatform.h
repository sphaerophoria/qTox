#pragma once

#include <QQuickWidget>

class ContentLayout;
class IChatLog;
class IMessageDispatcher;
class ExperimentalChatLogModel;

class ExperimentalChatForm : public QQuickWidget
{
    Q_OBJECT
public:
    ExperimentalChatForm(IChatLog& chatLog, IMessageDispatcher& messageDispatcher, QWidget* parent = nullptr);
    ~ExperimentalChatForm() = default;

    void show(ContentLayout* contentLayout);

private slots:
    void onReloadUi();
    void onMessageSent(QString message);

private:
    void initializeQml();
    ExperimentalChatLogModel* chatLogModel;
    IMessageDispatcher& messageDispatcher;
    std::unique_ptr<QTimer> timer;
};
