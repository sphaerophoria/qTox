
#include "experimentalchatform.h"

#include "src/widget/contentlayout.h"
#include "src/model/ichatlog.h"
#include "src/model/imessagedispatcher.h"

#include <QAbstractItemModel>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QTimer>

class ExperimentalChatLogModel : public QAbstractListModel
{
public:
    enum Roles {
        message = Qt::UserRole + 1,
        sender,
        timestamp,
        displayName,
        messagePending
    };
    ExperimentalChatLogModel(IChatLog& chatLog, QObject* parent = nullptr)
        : chatLog(chatLog)
    {
        connect(&chatLog, &IChatLog::itemUpdated, this, &ExperimentalChatLogModel::onItemUpdated);
    }

    int rowCount(const QModelIndex& parent) const override
    {
        return chatLog.getNextIdx() - chatLog.getFirstIdx();
    }

    QVariant data(const QModelIndex& index, int role) const override
    {
        if (!index.isValid())
            return QVariant();

        auto& chatLogItem = chatLog.at(chatLog.getFirstIdx() + index.row());

        switch (role)
        {
        case message:
            if (chatLogItem.getContentType() == ChatLogItem::ContentType::message)
                return chatLogItem.getContentAsMessage().message.content;
            else
                return QString("<b>Unimplmented file transfer</b>");
        case sender:
            return chatLogItem.getSender().toString();
        case displayName:
            return chatLogItem.getDisplayName();
        case timestamp:
            return chatLogItem.getTimestamp();
        case messagePending:
            if (chatLogItem.getContentType() == ChatLogItem::ContentType::message)
                return chatLogItem.getContentAsMessage().state == MessageState::pending;
            return false;
        default:
            return QVariant();
        }

        return QVariant();
    }

    QHash<int, QByteArray> roleNames() const override
    {
        return {
            { message, "message" },
            { sender, "sender" },
            { timestamp, "timestamp"},
            { displayName, "displayName"},
            { messagePending, "messagePending"},
        };
    }
private:

    void onItemUpdated(ChatLogIdx idx)
    {
        auto modelIdx = idx - chatLog.getFirstIdx();

        // Remove row in case this is an update. If this is a new message this
        // will just do nothing
        beginRemoveRows(QModelIndex(), modelIdx, modelIdx);
        endRemoveRows();

        beginInsertRows(QModelIndex(), modelIdx, modelIdx);
        endInsertRows();
    }

    IChatLog& chatLog;
    int lastRowCount = 0;
};


ExperimentalChatForm::ExperimentalChatForm(IChatLog& chatLog, IMessageDispatcher& messageDispatcher, Contact& contact, QWidget* parent)
    : QQuickWidget(parent)
    , chatLogModel(new ExperimentalChatLogModel(chatLog, parent))
    , messageDispatcher(messageDispatcher)
    , contact(contact)
{
    initializeQml();
}

void ExperimentalChatForm::show(ContentLayout* layout)
{
    // Note: We intentionally do not use the mainHead here. This allows us to
    // render more space in the QML window. If we did not do this the popup for
    // incoming calls would be clipped into the area shown by the mainHead.
    //
    // An alternative could be to draw the callConfirm widget oustside of QML
    layout->mainContent->layout()->addWidget(this);
    QQuickWidget::show();
}

void ExperimentalChatForm::onReloadUi()
{
    // Delay reload to ensure that it isn't called within the context of the
    // current qml signal
    timer.reset(new QTimer);
    timer->setSingleShot(true);
    timer->callOnTimeout([this] {
        engine()->clearComponentCache();
        setSource(QUrl());
        initializeQml();
    });
    timer->start(std::chrono::milliseconds(30));
}

void ExperimentalChatForm::initializeQml()
{
    auto rootContext = engine()->rootContext();
    rootContext->setContextProperty("chatModel", chatLogModel);
    rootContext->setContextProperty("contact", &contact);
    setSource(QUrl::fromLocalFile("../qml/FriendChatForm.qml"));
    setAttribute(Qt::WA_AlwaysStackOnTop);
    setClearColor(Qt::transparent);
    setResizeMode(QQuickWidget::SizeRootObjectToView);

    connect(rootObject(), SIGNAL(reloadUi()), this, SLOT(onReloadUi()));
    connect(rootObject(), SIGNAL(messageSent(QString)), this, SLOT(onMessageSent(QString)));
}

void ExperimentalChatForm::onMessageSent(QString message)
{
    messageDispatcher.sendMessage(false, message);
}
