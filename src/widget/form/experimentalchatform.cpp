
#include "experimentalchatform.h"

#include "src/widget/contentlayout.h"
#include "src/model/ichatlog.h"

#include <QAbstractItemModel>
#include <QQmlContext>
#include <QQmlEngine>

class ExperimentalChatLogModel : public QAbstractListModel
{
public:
    enum Roles {
        message = Qt::UserRole + 1,
        sender,
        timestamp,
        displayName
    };
    ExperimentalChatLogModel(IChatLog& chatLog, QObject* parent = nullptr)
        : chatLog(chatLog)
    { }

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
            { displayName, "displayName"}
        };
    }
private:
    IChatLog& chatLog;
};


ExperimentalChatForm::ExperimentalChatForm(IChatLog& chatLog, QWidget* parent)
    : QQuickWidget(parent)
    , chatLogModel(new ExperimentalChatLogModel(chatLog, parent))
{

    engine()->rootContext()->setContextProperty("chatModel", chatLogModel);
    setSource(QUrl::fromLocalFile("../qml/friendchatform.qml"));
    setAttribute(Qt::WA_AlwaysStackOnTop);
    setClearColor(Qt::transparent);
    setResizeMode(QQuickWidget::SizeRootObjectToView);
}

void ExperimentalChatForm::show(ContentLayout* layout) {
    // Note: We intentionally do not use the mainHead here. This allows us to
    // render more space in the QML window. If we did not do this the popup for
    // incoming calls would be clipped into the area shown by the mainHead.
    //
    // An alternative could be to draw the callConfirm widget oustside of QML
    layout->mainContent->layout()->addWidget(this);
    QQuickWidget::show();
}
