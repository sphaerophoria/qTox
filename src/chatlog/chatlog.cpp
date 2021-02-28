/*
    Copyright © 2014-2019 by The qTox Project Contributors

    This file is part of qTox, a Qt-based graphical interface for Tox.

    qTox is libre software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    qTox is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with qTox.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "chatlog.h"
#include "chatlinecontent.h"
#include "chatlinecontentproxy.h"
#include "chatmessage.h"
#include "content/filetransferwidget.h"
#include "content/text.h"
#include "src/widget/gui.h"
#include "src/widget/translator.h"
#include "src/widget/style.h"
#include "src/persistence/settings.h"
#include "src/chatlog/chatlinestorage.h"

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QDebug>
#include <QMouseEvent>
#include <QScrollBar>
#include <QShortcut>
#include <QTimer>

#include <algorithm>
#include <cassert>
#include <set>


namespace
{

// Maximum number of rendered messages at any given time
static size_t constexpr maxWindowSize = 300;
// Amount of messages to purge when removing messages
static size_t constexpr windowChunkSize = 100;

template <class T>
T clamp(T x, T min, T max)
{
    if (x > max)
        return max;
    if (x < min)
        return min;
    return x;
}

ChatMessage::Ptr createDateMessage(QDateTime timestamp)
{
    const auto& s = Settings::getInstance();
    const auto date = timestamp.date();
    auto dateText = date.toString(s.getDateFormat());
    return ChatMessage::createChatInfoMessage(dateText, ChatMessage::INFO, QDateTime());
}

ChatMessage::Ptr createMessage(const QString& displayName, bool isSelf, bool colorizeNames,
                               const ChatLogMessage& chatLogMessage)
{
    auto messageType = chatLogMessage.message.isAction ? ChatMessage::MessageType::ACTION
                                                       : ChatMessage::MessageType::NORMAL;

    const bool bSelfMentioned =
        std::any_of(chatLogMessage.message.metadata.begin(), chatLogMessage.message.metadata.end(),
                    [](const MessageMetadata& metadata) {
                        return metadata.type == MessageMetadataType::selfMention;
                    });

    if (bSelfMentioned) {
        messageType = ChatMessage::MessageType::ALERT;
    }

    const auto timestamp = chatLogMessage.message.timestamp;
    return ChatMessage::createChatMessage(displayName, chatLogMessage.message.content, messageType,
                                          isSelf, chatLogMessage.state, timestamp, colorizeNames);
}

void renderMessageRaw(const QString& displayName, bool isSelf, bool colorizeNames,
                   const ChatLogMessage& chatLogMessage, ChatLine::Ptr& chatLine)
{
    // HACK: This is kind of gross, but there's not an easy way to fit this into
    // the existing architecture. This shouldn't ever fail since we should only
    // correlate ChatMessages created here, however a logic bug could turn into
    // a crash due to this dangerous cast. The alternative would be to make
    // ChatLine a QObject which I didn't think was worth it.
    auto chatMessage = static_cast<ChatMessage*>(chatLine.get());

    if (chatMessage) {
        if (chatLogMessage.state == MessageState::complete) {
            chatMessage->markAsDelivered(chatLogMessage.message.timestamp);
        } else if (chatLogMessage.state == MessageState::broken) {
            chatMessage->markAsBroken();
        }
    } else {
        chatLine = createMessage(displayName, isSelf, colorizeNames, chatLogMessage);
    }
}

/**
 * @return Chat message message type (info/warning) for the given system message
 * @param[in] systemMessage
 */
ChatMessage::SystemMessageType getChatMessageType(const SystemMessage& systemMessage)
{
    switch (systemMessage.messageType)
    {
    case SystemMessageType::fileSendFailed:
    case SystemMessageType::messageSendFailed:
    case SystemMessageType::unexpectedCallEnd:
        return ChatMessage::ERROR;
    case SystemMessageType::userJoinedGroup:
    case SystemMessageType::userLeftGroup:
    case SystemMessageType::peerNameChanged:
    case SystemMessageType::peerStateChange:
    case SystemMessageType::titleChanged:
    case SystemMessageType::cleared:
    case SystemMessageType::outgoingCall:
    case SystemMessageType::incomingCall:
    case SystemMessageType::callEnd:
        return ChatMessage::INFO;
    }

    return ChatMessage::INFO;
}

ChatLogIdx firstItemAfterDate(QDate date, const IChatLog& chatLog)
{
    auto idxs = chatLog.getDateIdxs(date, 1);
    if (idxs.size()) {
        return idxs[0].idx;
    } else {
        return chatLog.getNextIdx();
    }
}

template <typename Fn>
void forEachLineIn(ChatLine::Ptr first, ChatLine::Ptr last, ChatLineStorage& storage, Fn f)
{
    auto endIt = storage.find(last);
    if (endIt != storage.end()) {
        endIt++;
    }

    for (auto it = storage.find(first); it != endIt; ++it) {
        f(*it);
    }
}

/**
 * @brief Helper function to add an offset ot a ChatLogIdx without going
 * outside the bounds of the associated chatlog
 */
ChatLogIdx clampedAdd(ChatLogIdx idx, int val, IChatLog& chatLog)
{
    if (val < 0) {
        auto distToEnd = idx - chatLog.getFirstIdx();
        if (static_cast<size_t>(std::abs(val)) > distToEnd) {
            return chatLog.getFirstIdx();
        }

        return idx - std::abs(val);
    } else {
        auto distToEnd = chatLog.getNextIdx() - idx;
        if (static_cast<size_t>(val) > distToEnd) {
            return chatLog.getNextIdx();
        }

        return idx + val;
    }
}

} // namespace


ChatLog::ChatLog(IChatLog& chatLog, const Core& core, QWidget* parent)
    : QGraphicsView(parent)
    , chatLog(chatLog)
    , core(core)
    , renderedLineStorage(new ChatLineStorage())
{
    // Create the scene
    busyScene = new QGraphicsScene(this);
    scene = new QGraphicsScene(this);
    scene->setItemIndexMethod(QGraphicsScene::BspTreeIndex);
    setScene(scene);

    busyNotification = ChatMessage::createBusyNotification();
    busyNotification->addToScene(busyScene);
    busyNotification->visibilityChanged(true);

    // Cfg.
    setInteractive(true);
    setAcceptDrops(false);
    setAlignment(Qt::AlignTop | Qt::AlignLeft);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setDragMode(QGraphicsView::NoDrag);
    setViewportUpdateMode(MinimalViewportUpdate);
    setContextMenuPolicy(Qt::CustomContextMenu);
    setBackgroundBrush(QBrush(Style::getColor(Style::GroundBase), Qt::SolidPattern));

    // The selection rect for multi-line selection
    selGraphItem = scene->addRect(0, 0, 0, 0, selectionRectColor.darker(120), selectionRectColor);
    selGraphItem->setZValue(-1.0); // behind all other items

    // copy action (ie. Ctrl+C)
    copyAction = new QAction(this);
    copyAction->setIcon(QIcon::fromTheme("edit-copy"));
    copyAction->setShortcut(QKeySequence::Copy);
    copyAction->setEnabled(false);
    connect(copyAction, &QAction::triggered, this, [this]() { copySelectedText(); });
    addAction(copyAction);

    // Ctrl+Insert shortcut
    QShortcut* copyCtrlInsShortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Insert), this);
    connect(copyCtrlInsShortcut, &QShortcut::activated, this, [this]() { copySelectedText(); });

    // select all action (ie. Ctrl+A)
    selectAllAction = new QAction(this);
    selectAllAction->setIcon(QIcon::fromTheme("edit-select-all"));
    selectAllAction->setShortcut(QKeySequence::SelectAll);
    connect(selectAllAction, &QAction::triggered, this, [this]() { selectAll(); });
    addAction(selectAllAction);

    // This timer is used to scroll the view while the user is
    // moving the mouse past the top/bottom edge of the widget while selecting.
    selectionTimer = new QTimer(this);
    selectionTimer->setInterval(1000 / 30);
    selectionTimer->setSingleShot(false);
    selectionTimer->start();
    connect(selectionTimer, &QTimer::timeout, this, &ChatLog::onSelectionTimerTimeout);

    // Background worker
    // Updates the layout of all chat-lines after a resize
    workerTimer = new QTimer(this);
    workerTimer->setSingleShot(false);
    workerTimer->setInterval(5);
    connect(workerTimer, &QTimer::timeout, this, &ChatLog::onWorkerTimeout);

    // This timer is used to detect multiple clicks
    multiClickTimer = new QTimer(this);
    multiClickTimer->setSingleShot(true);
    multiClickTimer->setInterval(QApplication::doubleClickInterval());
    connect(multiClickTimer, &QTimer::timeout, this, &ChatLog::onMultiClickTimeout);

    // selection
    connect(this, &ChatLog::selectionChanged, this, [this]() {
        copyAction->setEnabled(hasTextToBeCopied());
        copySelectedText(true);
    });

    connect(&GUI::getInstance(), &GUI::themeReload, this, &ChatLog::reloadTheme);

    reloadTheme();
    retranslateUi();
    Translator::registerHandler(std::bind(&ChatLog::retranslateUi, this), this);

    connect(this, &ChatLog::renderFinished, this, &ChatLog::onRenderFinished);
    connect(&chatLog, &IChatLog::itemUpdated, this, &ChatLog::onMessageUpdated);
    connect(verticalScrollBar(), &QScrollBar::valueChanged, this, &ChatLog::onScrollValueChanged);

    auto firstChatLogIdx = clampedAdd(chatLog.getNextIdx(), -100, chatLog);
    renderMessages(firstChatLogIdx, chatLog.getNextIdx());
}

ChatLog::~ChatLog()
{
    Translator::unregister(this);

    // Remove chatlines from scene
    for (ChatLine::Ptr l : *renderedLineStorage)
        l->removeFromScene();

    if (busyNotification)
        busyNotification->removeFromScene();

    if (typingNotification)
        typingNotification->removeFromScene();
}

void ChatLog::clearSelection()
{
    if (selectionMode == SelectionMode::None)
        return;

    forEachLineIn(selFirstRow, selLastRow, *renderedLineStorage, [&] (ChatLine::Ptr& line) {
        line->selectionCleared();
    });

    selFirstRow.reset();
    selLastRow.reset();
    selClickedCol = -1;
    selClickedRow.reset();

    selectionMode = SelectionMode::None;
    emit selectionChanged();

    updateMultiSelectionRect();
}

QRect ChatLog::getVisibleRect() const
{
    return mapToScene(viewport()->rect()).boundingRect().toRect();
}

void ChatLog::updateSceneRect()
{
    setSceneRect(calculateSceneRect());
}

void ChatLog::layout(int start, int end, qreal width)
{
    if (renderedLineStorage->empty())
        return;

    qreal h = 0.0;

    // Line at start-1 is considered to have the correct position. All following lines are
    // positioned in respect to this line.
    if (start - 1 >= 0)
        h = (*renderedLineStorage)[start - 1]->sceneBoundingRect().bottom() + lineSpacing;

    start = clamp<int>(start, 0, renderedLineStorage->size());
    end = clamp<int>(end + 1, 0, renderedLineStorage->size());

    for (int i = start; i < end; ++i) {
        ChatLine* l = (*renderedLineStorage)[i].get();

        l->layout(width, QPointF(0.0, h));
        h += l->sceneBoundingRect().height() + lineSpacing;
    }
}

void ChatLog::mousePressEvent(QMouseEvent* ev)
{
    QGraphicsView::mousePressEvent(ev);

    if (ev->button() == Qt::LeftButton) {
        clickPos = ev->pos();
        clearSelection();
    }

    if (lastClickButton == ev->button()) {
        // Counts only single clicks and first click of doule click
        clickCount++;
    }
    else {
        clickCount = 1; // restarting counter
        lastClickButton = ev->button();
    }
    lastClickPos = ev->pos();

    // Triggers on odd click counts
    handleMultiClickEvent();
}

void ChatLog::mouseReleaseEvent(QMouseEvent* ev)
{
    QGraphicsView::mouseReleaseEvent(ev);

    selectionScrollDir = AutoScrollDirection::NoDirection;

    multiClickTimer->start();
}

void ChatLog::mouseMoveEvent(QMouseEvent* ev)
{
    QGraphicsView::mouseMoveEvent(ev);

    QPointF scenePos = mapToScene(ev->pos());

    if (ev->buttons() & Qt::LeftButton) {
        // autoscroll
        if (ev->pos().y() < 0)
            selectionScrollDir = AutoScrollDirection::Up;
        else if (ev->pos().y() > height())
            selectionScrollDir = AutoScrollDirection::Down;
        else
            selectionScrollDir = AutoScrollDirection::NoDirection;

        // select
        if (selectionMode == SelectionMode::None
            && (clickPos - ev->pos()).manhattanLength() > QApplication::startDragDistance()) {
            QPointF sceneClickPos = mapToScene(clickPos.toPoint());
            ChatLine::Ptr line = findLineByPosY(scenePos.y());

            ChatLineContent* content = getContentFromPos(sceneClickPos);
            if (content) {
                selClickedRow = line;
                selClickedCol = content->getColumn();
                selFirstRow = line;
                selLastRow = line;

                content->selectionStarted(sceneClickPos);

                selectionMode = SelectionMode::Precise;

                // ungrab mouse grabber
                if (scene->mouseGrabberItem())
                    scene->mouseGrabberItem()->ungrabMouse();
            } else if (line.get()) {
                selClickedRow = line;
                selFirstRow = selClickedRow;
                selLastRow = selClickedRow;

                selectionMode = SelectionMode::Multi;
            }
        }

        if (selectionMode != SelectionMode::None) {
            ChatLineContent* content = getContentFromPos(scenePos);
            ChatLine::Ptr line = findLineByPosY(scenePos.y());

            if (content) {
                int col = content->getColumn();

                if (line == selClickedRow && col == selClickedCol) {
                    selectionMode = SelectionMode::Precise;

                    content->selectionMouseMove(scenePos);
                    selGraphItem->hide();
                } else if (col != selClickedCol) {
                    selectionMode = SelectionMode::Multi;

                    line->selectionCleared();
                }
            } else if (line.get()) {
                if (line != selClickedRow) {
                    selectionMode = SelectionMode::Multi;
                    line->selectionCleared();
                }
            } else {
                return;
            }

            auto selClickedIt = renderedLineStorage->find(selClickedRow);
            auto lineIt = renderedLineStorage->find(line);
            if (lineIt > selClickedIt)
                selLastRow = line;

            if (lineIt <= selClickedIt)
                selFirstRow = line;

            updateMultiSelectionRect();
        }

        emit selectionChanged();
    }
}

// Much faster than QGraphicsScene::itemAt()!
ChatLineContent* ChatLog::getContentFromPos(QPointF scenePos) const
{
    if (renderedLineStorage->empty())
        return nullptr;

    auto itr =
        std::lower_bound(renderedLineStorage->begin(), renderedLineStorage->end(), scenePos.y(), ChatLine::lessThanBSRectBottom);

    // find content
    if (itr != renderedLineStorage->end() && (*itr)->sceneBoundingRect().contains(scenePos))
        return (*itr)->getContent(scenePos);

    return nullptr;
}

bool ChatLog::isOverSelection(QPointF scenePos) const
{
    if (selectionMode == SelectionMode::Precise) {
        ChatLineContent* content = getContentFromPos(scenePos);

        if (content)
            return content->isOverSelection(scenePos);
    } else if (selectionMode == SelectionMode::Multi) {
        if (selGraphItem->rect().contains(scenePos))
            return true;
    }

    return false;
}

qreal ChatLog::useableWidth() const
{
    return width() - verticalScrollBar()->sizeHint().width() - margins.right() - margins.left();
}

void ChatLog::insertChatlines(std::map<ChatLogIdx, ChatLine::Ptr> chatLines)
{
    if (chatLines.empty())
        return;

    bool allLinesAtEnd = !renderedLineStorage->hasIndexedMessage() || chatLines.begin()->first > renderedLineStorage->lastIdx();
    auto startLineSize = renderedLineStorage->size();

    QGraphicsScene::ItemIndexMethod oldIndexMeth = scene->itemIndexMethod();
    scene->setItemIndexMethod(QGraphicsScene::NoIndex);

    for (auto const& chatLine : chatLines) {
        auto idx = chatLine.first;
        auto const& l = chatLine.second;

        auto insertedMessageIt = renderedLineStorage->insertChatMessage(idx, chatLog.at(idx).getTimestamp(), l);

        auto date = chatLog.at(idx).getTimestamp().date().startOfDay();

        auto insertionIdx = std::distance(renderedLineStorage->begin(), insertedMessageIt);

        if (!renderedLineStorage->contains(date)) {
            // If there is no dateline for the given date we need to insert it
            // above the line we'd like to insert.
            auto dateLine = createDateMessage(date);
            renderedLineStorage->insertDateLine(date, dateLine);
            dateLine->addToScene(scene);
            dateLine->visibilityChanged(false);
        }

        l->addToScene(scene);

        // Optimization copied from previous implementation of upwards
        // rendering. This will be changed when we call updateVisibility
        // later
        l->visibilityChanged(false);
    }

    scene->setItemIndexMethod(oldIndexMeth);

    // If all insertions are at the bottom we can get away with only rendering
    // the updated lines, otherwise we need to go through the resize workflow to
    // re-layout everything asynchronously.
    //
    // NOTE: This can make flow from the callers a little frustrating as you
    // have to rely on the onRenderFinished callback to continue doing any work,
    // even if all rendering is done synchronously
    if (allLinesAtEnd) {
        bool stickToBtm = stickToBottom();

        // partial refresh
        layout(startLineSize, renderedLineStorage->size(), useableWidth());
        updateSceneRect();

        if (stickToBtm)
            scrollToBottom();

        checkVisibility();
        updateTypingNotification();
        updateMultiSelectionRect();

        emit renderFinished();
    } else {
        startResizeWorker();
    }
}

bool ChatLog::stickToBottom() const
{
    return verticalScrollBar()->value() == verticalScrollBar()->maximum();
}

void ChatLog::scrollToBottom()
{
    updateSceneRect();
    verticalScrollBar()->setValue(verticalScrollBar()->maximum());
}

void ChatLog::startResizeWorker()
{
    if (renderedLineStorage->empty())
        return;

    // (re)start the worker
    if (!workerTimer->isActive()) {
        // these values must not be reevaluated while the worker is running
        workerStb = stickToBottom();

        if (!visibleLines.empty())
            workerAnchorLine = visibleLines.first();
    }

    // switch to busy scene displaying the busy notification if there is a lot
    // of text to be resized
    int txt = 0;
    for (ChatLine::Ptr line : *renderedLineStorage) {
        if (txt > 500000)
            break;
        for (ChatLineContent* content : line->content)
            txt += content->getText().size();
    }
    if (txt > 500000)
        setScene(busyScene);

    workerLastIndex = 0;
    workerTimer->start();

    verticalScrollBar()->hide();
}

void ChatLog::mouseDoubleClickEvent(QMouseEvent* ev)
{
    QPointF scenePos = mapToScene(ev->pos());
    ChatLineContent* content = getContentFromPos(scenePos);
    ChatLine::Ptr line = findLineByPosY(scenePos.y());

    if (content) {
        content->selectionDoubleClick(scenePos);
        selClickedCol = content->getColumn();
        selClickedRow = line;
        selFirstRow = line;
        selLastRow = line;
        selectionMode = SelectionMode::Precise;

        emit selectionChanged();
    }

    if (lastClickButton == ev->button()) {
        // Counts the second click of double click
        clickCount++;
    }
    else {
        clickCount = 1; // restarting counter
        lastClickButton = ev->button();
    }
    lastClickPos = ev->pos();

    // Triggers on even click counts
    handleMultiClickEvent();
}

QString ChatLog::getSelectedText() const
{
    if (selectionMode == SelectionMode::Precise) {
        return selClickedRow->content[selClickedCol]->getSelectedText();
    } else if (selectionMode == SelectionMode::Multi) {
        // build a nicely formatted message
        QString out;

        forEachLineIn(selFirstRow, selLastRow, *renderedLineStorage, [&] (ChatLine::Ptr& line) {
            if (line->content[1]->getText().isEmpty())
                return;

            QString timestamp = line->content[2]->getText().isEmpty()
                                    ? tr("pending")
                                    : line->content[2]->getText();
            QString author = line->content[0]->getText();
            QString msg = line->content[1]->getText();

            out +=
                QString(out.isEmpty() ? "[%2] %1: %3" : "\n[%2] %1: %3").arg(author, timestamp, msg);
        });

        return out;
    }

    return QString();
}

bool ChatLog::isEmpty() const
{
    return renderedLineStorage->empty();
}

bool ChatLog::hasTextToBeCopied() const
{
    return selectionMode != SelectionMode::None;
}

/**
 * @brief Finds the chat line object at a position on screen
 * @param pos Position on screen in global coordinates
 * @sa getContentFromPos()
 */
ChatLineContent* ChatLog::getContentFromGlobalPos(QPoint pos) const
{
    return getContentFromPos(mapToScene(mapFromGlobal(pos)));
}

void ChatLog::clear()
{
    clearSelection();

    QVector<ChatLine::Ptr> savedLines;

    for (auto it = renderedLineStorage->begin(); it != renderedLineStorage->end();) {
        if (!isActiveFileTransfer(*it)) {
            (*it)->removeFromScene();
            it = renderedLineStorage->erase(it);
        } else {
            it++;
        }
    }

    visibleLines.clear();

    checkVisibility();
    updateSceneRect();
}

void ChatLog::copySelectedText(bool toSelectionBuffer) const
{
    QString text = getSelectedText();
    QClipboard* clipboard = QApplication::clipboard();

    if (clipboard && !text.isNull())
        clipboard->setText(text, toSelectionBuffer ? QClipboard::Selection : QClipboard::Clipboard);
}

void ChatLog::setTypingNotificationVisible(bool visible)
{
    if (typingNotification.get()) {
        typingNotification->setVisible(visible);
        updateTypingNotification();
    }
}

void ChatLog::setTypingNotificationName(const QString& displayName)
{
    if (!typingNotification.get()) {
        setTypingNotification();
    }

    Text* text = static_cast<Text*>(typingNotification->getContent(1));
    QString typingDiv = "<div class=typing>%1</div>";
    text->setText(typingDiv.arg(tr("%1 is typing").arg(displayName)));

    updateTypingNotification();
}

void ChatLog::scrollToLine(ChatLine::Ptr line)
{
    if (!line.get())
        return;

    updateSceneRect();
    verticalScrollBar()->setValue(line->sceneBoundingRect().top());
}

void ChatLog::selectAll()
{
    if (renderedLineStorage->empty())
        return;

    clearSelection();

    selectionMode = SelectionMode::Multi;
    selFirstRow = renderedLineStorage->front();;
    selLastRow = renderedLineStorage->back();

    emit selectionChanged();
    updateMultiSelectionRect();
}

void ChatLog::fontChanged(const QFont& font)
{
    for (ChatLine::Ptr l : *renderedLineStorage) {
        l->fontChanged(font);
    }
}

void ChatLog::reloadTheme()
{
    setStyleSheet(Style::getStylesheet("chatArea/chatArea.css"));
    setBackgroundBrush(QBrush(Style::getColor(Style::GroundBase), Qt::SolidPattern));
    selectionRectColor = Style::getColor(Style::SelectText);
    selGraphItem->setBrush(QBrush(selectionRectColor));
    selGraphItem->setPen(QPen(selectionRectColor.darker(120)));
    setTypingNotification();

    for (ChatLine::Ptr l : *renderedLineStorage) {
        l->reloadTheme();
    }
}

void ChatLog::startSearch(const QString& phrase, const ParameterSearch& parameter)
{
    disableSearchText();

    bool bForwardSearch = false;
    switch (parameter.period) {
    case PeriodSearch::WithTheFirst: {
        bForwardSearch = true;
        searchPos.logIdx = chatLog.getFirstIdx();
        searchPos.numMatches = 0;
        break;
    }
    case PeriodSearch::WithTheEnd:
    case PeriodSearch::None: {
        bForwardSearch = false;
        searchPos.logIdx = chatLog.getNextIdx();
        searchPos.numMatches = 0;
        break;
    }
    case PeriodSearch::AfterDate: {
        bForwardSearch = true;
        searchPos.logIdx = firstItemAfterDate(parameter.date, chatLog);
        searchPos.numMatches = 0;
        break;
    }
    case PeriodSearch::BeforeDate: {
        bForwardSearch = false;
        searchPos.logIdx = firstItemAfterDate(parameter.date, chatLog);
        searchPos.numMatches = 0;
        break;
    }
    }

    if (bForwardSearch) {
        onSearchDown(phrase, parameter);
    } else {
        onSearchUp(phrase, parameter);
    }
}

void ChatLog::onSearchUp(const QString& phrase, const ParameterSearch& parameter)
{
    auto result = chatLog.searchBackward(searchPos, phrase, parameter);
    handleSearchResult(result, SearchDirection::Up);
}

void ChatLog::onSearchDown(const QString& phrase, const ParameterSearch& parameter)
{
    auto result = chatLog.searchForward(searchPos, phrase, parameter);
    handleSearchResult(result, SearchDirection::Down);
}

void ChatLog::handleSearchResult(SearchResult result, SearchDirection direction)
{
    if (!result.found) {
        emit messageNotFoundShow(direction);
        return;
    }

    disableSearchText();

    searchPos = result.pos;

    auto const firstRenderedIdx = (renderedLineStorage->hasIndexedMessage()) ? renderedLineStorage->firstIdx() : chatLog.getNextIdx();

    auto selectText = [this, result] {
        // With fast changes our callback could become invalid, ensure that the
        // index we want to view is still actually visible
        if (!renderedLineStorage->contains(searchPos.logIdx))
            return;

        auto msg = (*renderedLineStorage)[searchPos.logIdx];
        scrollToLine(msg);

        auto text = qobject_cast<Text*>(msg->getContent(1));
        text->selectText(result.exp, std::make_pair(result.start, result.len));
    };

    // If the requested element is visible the render completion callback will
    // not be called, we need to figure out which path we're going to take
    // before we take it.
    if (renderedLineStorage->contains(searchPos.logIdx)) {
        jumpToIdx(searchPos.logIdx);
        selectText();
    } else {
        renderCompletionFns.push_back(selectText);
        jumpToIdx(searchPos.logIdx);
    }

}

void ChatLog::forceRelayout()
{
    startResizeWorker();
}

void ChatLog::checkVisibility()
{
    if (renderedLineStorage->empty())
        return;

    // find first visible line
    auto lowerBound = std::lower_bound(renderedLineStorage->begin(), renderedLineStorage->end(), getVisibleRect().top(),
                                       ChatLine::lessThanBSRectBottom);

    // find last visible line
    auto upperBound = std::lower_bound(lowerBound, renderedLineStorage->end(), getVisibleRect().bottom(),
                                       ChatLine::lessThanBSRectTop);

    const ChatLine::Ptr lastLineBeforeVisible = lowerBound == renderedLineStorage->begin()
        ? ChatLine::Ptr()
        : *std::prev(lowerBound);

    // set visibilty
    QList<ChatLine::Ptr> newVisibleLines;
    for (auto itr = lowerBound; itr != upperBound; ++itr) {
        newVisibleLines.append(*itr);

        if (!visibleLines.contains(*itr))
            (*itr)->visibilityChanged(true);

        visibleLines.removeOne(*itr);
    }

    // these lines are no longer visible
    for (ChatLine::Ptr line : visibleLines)
        line->visibilityChanged(false);

    visibleLines = newVisibleLines;

    if (!visibleLines.isEmpty()) {
        emit firstVisibleLineChanged(lastLineBeforeVisible, visibleLines.at(0));
    }
}

void ChatLog::scrollContentsBy(int dx, int dy)
{
    QGraphicsView::scrollContentsBy(dx, dy);
    checkVisibility();
}

void ChatLog::resizeEvent(QResizeEvent* ev)
{
    bool stb = stickToBottom();

    if (ev->size().width() != ev->oldSize().width()) {
        startResizeWorker();
        stb = false; // let the resize worker handle it
    }

    QGraphicsView::resizeEvent(ev);

    if (stb)
        scrollToBottom();

    updateBusyNotification();
}

void ChatLog::updateMultiSelectionRect()
{
    if (selectionMode == SelectionMode::Multi && selFirstRow && selLastRow) {
        QRectF selBBox;
        selBBox = selBBox.united(selFirstRow->sceneBoundingRect());
        selBBox = selBBox.united(selLastRow->sceneBoundingRect());

        if (selGraphItem->rect() != selBBox)
            scene->invalidate(selGraphItem->rect());

        selGraphItem->setRect(selBBox);
        selGraphItem->show();
    } else {
        selGraphItem->hide();
    }
}

void ChatLog::updateTypingNotification()
{
    ChatLine* notification = typingNotification.get();
    if (!notification)
        return;

    qreal posY = 0.0;

    if (!renderedLineStorage->empty())
        posY = renderedLineStorage->back()->sceneBoundingRect().bottom() + lineSpacing;

    notification->layout(useableWidth(), QPointF(0.0, posY));
}

void ChatLog::updateBusyNotification()
{
    // repoisition the busy notification (centered)
    busyNotification->layout(useableWidth(), getVisibleRect().topLeft()
                                                    + QPointF(0, getVisibleRect().height() / 2.0));
}

ChatLine::Ptr ChatLog::findLineByPosY(qreal yPos) const
{
    auto itr = std::lower_bound(renderedLineStorage->begin(), renderedLineStorage->end(), yPos, ChatLine::lessThanBSRectBottom);

    if (itr != renderedLineStorage->end())
        return *itr;

    return ChatLine::Ptr();
}

void ChatLog::removeLines(ChatLogIdx begin, ChatLogIdx end)
{
    if (!renderedLineStorage->hasIndexedMessage()) {
        // No indexed lines to remove
        return;
    }

    begin = clamp<ChatLogIdx>(begin, renderedLineStorage->firstIdx(), renderedLineStorage->lastIdx());
    end = clamp<ChatLogIdx>(end, renderedLineStorage->firstIdx(), renderedLineStorage->lastIdx()) + 1;

    // NOTE: Optimization potential if this find proves to be too expensive.
    // Batching all our erases into one call would be more efficient
    for (auto it = renderedLineStorage->find(begin); it != renderedLineStorage->find(end);) {
        (*it)->removeFromScene();
        it = renderedLineStorage->erase(it);
    }

    // We need to re-layout anything that is after any line we removed. We could
    // probably be smarter and try to only re-render anything under what we
    // removed, but with the sliding window there doesn't seem to be much need
    if (renderedLineStorage->hasIndexedMessage() && begin <= renderedLineStorage->lastIdx()) {
        layout(0, renderedLineStorage->size(), useableWidth());
    }
}

QRectF ChatLog::calculateSceneRect() const
{
    qreal bottom = (renderedLineStorage->empty() ? 0.0 : renderedLineStorage->back()->sceneBoundingRect().bottom());

    if (typingNotification.get() != nullptr)
        bottom += typingNotification->sceneBoundingRect().height() + lineSpacing;

    return QRectF(-margins.left(), -margins.top(), useableWidth(),
                  bottom + margins.bottom() + margins.top());
}

void ChatLog::onSelectionTimerTimeout()
{
    const int scrollSpeed = 10;

    switch (selectionScrollDir) {
    case AutoScrollDirection::Up:
        verticalScrollBar()->setValue(verticalScrollBar()->value() - scrollSpeed);
        break;
    case AutoScrollDirection::Down:
        verticalScrollBar()->setValue(verticalScrollBar()->value() + scrollSpeed);
        break;
    default:
        break;
    }
}

void ChatLog::onWorkerTimeout()
{
    // Fairly arbitrary but
    // large values will make the UI unresponsive
    const int stepSize = 50;

    layout(workerLastIndex, workerLastIndex + stepSize, useableWidth());
    workerLastIndex += stepSize;

    // done?
    if (workerLastIndex >= renderedLineStorage->size()) {
        workerTimer->stop();

        // switch back to the scene containing the chat messages
        setScene(scene);

        // make sure everything gets updated
        updateSceneRect();
        checkVisibility();
        updateTypingNotification();
        updateMultiSelectionRect();

        // scroll
        if (workerStb)
            scrollToBottom();
        else
            scrollToLine(workerAnchorLine);

        // don't keep a Ptr to the anchor line
        workerAnchorLine = ChatLine::Ptr();

        // hidden during busy screen
        verticalScrollBar()->show();

        emit renderFinished();
    }
}

void ChatLog::onMultiClickTimeout()
{
    clickCount = 0;
}

void ChatLog::onMessageUpdated(ChatLogIdx idx)
{
    if (shouldRenderMessage(idx)) {
        renderMessage(idx);
    }

    if (stickToBottom()) {
        // If stuck to bottom, purge old messages. This is a simple way of
        // ensuring that if we receive messages while looking at the top of the
        // window we do not evict messages we are currently looking at.
        //
        // This can cause us to go over our max window size, however when we
        // scroll back down the window size should be clipped anyways

        if (renderedLineStorage->size() > maxWindowSize) {
            removeLines(renderedLineStorage->firstIdx(), renderedLineStorage->firstIdx() + windowChunkSize);
            startResizeWorker();
        }
    }
}

void ChatLog::renderMessage(ChatLogIdx idx)
{
    renderMessages(idx, idx + 1);
}

void ChatLog::renderMessages(ChatLogIdx begin, ChatLogIdx end)
{
    auto linesToRender = std::map<ChatLogIdx, ChatLine::Ptr>();

    for (auto i = begin; i < end; ++i) {
        bool alreadyRendered = renderedLineStorage->contains(i);
        bool prevIdxRendered = i != begin || renderedLineStorage->contains(i - 1);

        auto chatMessage = alreadyRendered ? (*renderedLineStorage)[i] : ChatLine::Ptr();
        renderItem(chatLog.at(i), needsToHideName(i, prevIdxRendered), colorizeNames, chatMessage);

        if (!alreadyRendered) {
            linesToRender.insert({i, chatMessage});
        }
    }

    insertChatlines(linesToRender);
}

void ChatLog::setRenderedWindowStart(ChatLogIdx begin)
{
    // End of the window is pre-determined as a hardcoded window size relative
    // to the start
    auto end = clampedAdd(begin, maxWindowSize, chatLog);

    // Use invalid + equal ChatLogIdx to force a full re-render if we do not
    // have an indexed message to compare to
    ChatLogIdx currentStart = ChatLogIdx(-1);
    ChatLogIdx currentEnd = ChatLogIdx(-1);

    if (renderedLineStorage->hasIndexedMessage()) {
        currentStart = renderedLineStorage->firstIdx();
        currentEnd = renderedLineStorage->lastIdx() + 1;
    }

    // If the window is already where we have no work to do
    if (currentStart == begin) {
        return;
    }

    // NOTE: This is more than an optimization, this is important for
    // selection consistency. If we re-create lines that are already rendered
    // the selXXXRow members will now be pointing to the wrong ChatLine::Ptr!
    // Please be sure to test selection logic when scrolling around loading
    // boundaries if changing this logic.
    if (begin < currentEnd && begin > currentStart) {
        // Remove leading lines
        removeLines(currentStart, begin);
        renderMessages(currentEnd, end);
    }
    else if (end <= currentEnd && end > currentStart) {
        // Remove trailing lines
        removeLines(end, currentEnd);
        renderMessages(begin, currentStart);
    }
    else {
        removeLines(currentStart, currentEnd);
        renderMessages(begin, end);
    }
}

void ChatLog::setRenderedWindowEnd(ChatLogIdx end)
{
    // Off by 1 since the maxWindowSize is not inclusive
    auto start = clampedAdd(end, -static_cast<int>(maxWindowSize) + 1, chatLog);

    setRenderedWindowStart(start);
}

void ChatLog::onRenderFinished()
{
    // We have to back these up before we run them, because people might queue
    // on _more_ items on top of the ones we want. If they do this while we're
    // iterating we can hit some memory corruption issues
    auto renderCompletionFnsLocal = renderCompletionFns;
    renderCompletionFns.clear();

    while (renderCompletionFnsLocal.size()) {
        renderCompletionFnsLocal.back()();
        renderCompletionFnsLocal.pop_back();
    }

    // NOTE: this is a regression from previous behavior. We used to be able to
    // load an infinite amount of chat and copy paste it out. Now we limit the
    // user to 300 elements and any time the elements change our selection gets
    // invalidated. This could be improved in the future but for now I  do not
    // believe this is a serious usage impediment. Chats can be exported if a
    // user really needs more than 300 messages to be copied
    if (renderedLineStorage->find(selFirstRow) == renderedLineStorage->end() ||
        renderedLineStorage->find(selLastRow) == renderedLineStorage->end() ||
        renderedLineStorage->find(selClickedRow) == renderedLineStorage->end())
    {
        clearSelection();
    }
}

void ChatLog::onScrollValueChanged(int value)
{
    if (!renderedLineStorage->hasIndexedMessage()) {
        // This could be a little better. On a cleared screen we should probably
        // be able to scroll, but this makes the rest of this function easier
        return;
    }

    if (value == verticalScrollBar()->minimum())
    {
        auto idx = clampedAdd(renderedLineStorage->firstIdx(), -static_cast<int>(windowChunkSize), chatLog);

        if (idx != renderedLineStorage->firstIdx()) {
            auto currentTop = (*renderedLineStorage)[renderedLineStorage->firstIdx()];

            renderCompletionFns.push_back([this, currentTop] {
                scrollToLine(currentTop);
            });

            setRenderedWindowStart(idx);
        }

    }
    else if (value == verticalScrollBar()->maximum())
    {
        auto idx = clampedAdd(renderedLineStorage->lastIdx(), static_cast<int>(windowChunkSize), chatLog);

        if (idx != renderedLineStorage->lastIdx() + 1) {
            auto currentBottomIdx = renderedLineStorage->lastIdx();

            renderCompletionFns.push_back([this, currentBottomIdx] {
                auto it = renderedLineStorage->find(currentBottomIdx);
                if (it != renderedLineStorage->end()) {
                    scrollToLine(*it);
                }
            });

            setRenderedWindowEnd(idx);
        }
    }
}

void ChatLog::handleMultiClickEvent()
{
    // Ignore single or double clicks
    if (clickCount < 2)
        return;

    switch (clickCount) {
    case 3:
        QPointF scenePos = mapToScene(lastClickPos);
        ChatLineContent* content = getContentFromPos(scenePos);
        ChatLine::Ptr line = findLineByPosY(scenePos.y());

        if (content) {
            content->selectionTripleClick(scenePos);
            selClickedCol = content->getColumn();
            selClickedRow = line;
            selFirstRow = line;
            selLastRow = line;
            selectionMode = SelectionMode::Precise;

            emit selectionChanged();
        }
        break;
    }
}

void ChatLog::showEvent(QShowEvent*)
{
    // Empty.
    // The default implementation calls centerOn - for some reason - causing
    // the scrollbar to move.
}

void ChatLog::focusInEvent(QFocusEvent* ev)
{
    QGraphicsView::focusInEvent(ev);

    if (selectionMode != SelectionMode::None) {
        selGraphItem->setBrush(QBrush(selectionRectColor));

        auto endIt = renderedLineStorage->find(selLastRow);
        // Increase by one since this selLastRow is inclusive, not exclusive
        // like our loop expects
        if (endIt != renderedLineStorage->end()) {
            endIt++;
        }

        for (auto it = renderedLineStorage->begin(); it != renderedLineStorage->end() && it != endIt; ++it)
            (*it)->selectionFocusChanged(true);
    }
}

void ChatLog::focusOutEvent(QFocusEvent* ev)
{
    QGraphicsView::focusOutEvent(ev);

    if (selectionMode != SelectionMode::None) {
        selGraphItem->setBrush(QBrush(selectionRectColor.lighter(120)));

        auto endIt = renderedLineStorage->find(selLastRow);
        // Increase by one since this selLastRow is inclusive, not exclusive
        // like our loop expects
        if (endIt != renderedLineStorage->end()) {
            endIt++;
        }

        for (auto it = renderedLineStorage->begin(); it != renderedLineStorage->end() && it != endIt; ++it)
            (*it)->selectionFocusChanged(false);
    }
}

void ChatLog::wheelEvent(QWheelEvent *event)
{
    QGraphicsView::wheelEvent(event);
    checkVisibility();
}

void ChatLog::retranslateUi()
{
    copyAction->setText(tr("Copy"));
    selectAllAction->setText(tr("Select all"));
}

bool ChatLog::isActiveFileTransfer(ChatLine::Ptr l)
{
    int count = l->getColumnCount();
    for (int i = 0; i < count; ++i) {
        ChatLineContent* content = l->getContent(i);
        ChatLineContentProxy* proxy = qobject_cast<ChatLineContentProxy*>(content);
        if (!proxy)
            continue;

        QWidget* widget = proxy->getWidget();
        FileTransferWidget* transferWidget = qobject_cast<FileTransferWidget*>(widget);
        if (transferWidget && transferWidget->isActive())
            return true;
    }

    return false;
}

void ChatLog::setTypingNotification()
{
    typingNotification = ChatMessage::createTypingNotification();
    typingNotification->visibilityChanged(true);
    typingNotification->setVisible(false);
    typingNotification->addToScene(scene);
    updateTypingNotification();
}


void ChatLog::renderItem(const ChatLogItem& item, bool hideName, bool colorizeNames, ChatLine::Ptr& chatMessage)
{
    const auto& sender = item.getSender();

    bool isSelf = sender == core.getSelfId().getPublicKey();

    switch (item.getContentType()) {
    case ChatLogItem::ContentType::message: {
        const auto& chatLogMessage = item.getContentAsMessage();

        renderMessageRaw(item.getDisplayName(), isSelf, colorizeNames, chatLogMessage, chatMessage);

        break;
    }
    case ChatLogItem::ContentType::fileTransfer: {
        const auto& file = item.getContentAsFile();
        renderFile(item.getDisplayName(), file.file, isSelf, item.getTimestamp(), chatMessage);
        break;
    }
    case ChatLogItem::ContentType::systemMessage: {
        const auto& systemMessage = item.getContentAsSystemMessage();

        auto chatMessageType = getChatMessageType(systemMessage);
        chatMessage = ChatMessage::createChatInfoMessage(systemMessage.toString(), chatMessageType, QDateTime::currentDateTime());
        // Ignore caller's decision to hide the name. We show the icon in the
        // slot of the sender's name so we always want it visible
        hideName = false;
        break;
    }
    }

    if (hideName) {
        chatMessage->getContent(0)->hide();
    }
}

void ChatLog::renderFile(QString displayName, ToxFile file, bool isSelf, QDateTime timestamp,
                ChatLine::Ptr& chatMessage)
{
    if (!chatMessage) {
        CoreFile* coreFile = core.getCoreFile();
        assert(coreFile);
        chatMessage = ChatMessage::createFileTransferMessage(displayName, *coreFile, file, isSelf, timestamp);
    } else {
        auto proxy = static_cast<ChatLineContentProxy*>(chatMessage->getContent(1));
        assert(proxy->getWidgetType() == ChatLineContentProxy::FileTransferWidgetType);
        auto ftWidget = static_cast<FileTransferWidget*>(proxy->getWidget());
        ftWidget->onFileTransferUpdate(file);
    }
}

/**
 * @brief Determine if the name at the given idx needs to be hidden
 * @param idx ChatLogIdx of the message
 * @param prevIdxRendered Hint if the previous index is going to be rendered at
 * all. If the previous line is not rendered we always show the name
 * @return True if the name should be hidden, false otherwise
 */
bool ChatLog::needsToHideName(ChatLogIdx idx, bool prevIdxRendered) const
{
    // If the previous message is not rendered we should show the name
    // regardless of other constraints

    if (!prevIdxRendered) {
        return false;
    }

    const auto& prevItem = chatLog.at(idx - 1);
    const auto& currentItem = chatLog.at(idx);

    // Always show the * in the name field for action messages
    if (currentItem.getContentType() == ChatLogItem::ContentType::message
        && currentItem.getContentAsMessage().message.isAction) {
        return false;
    }

    qint64 messagesTimeDiff = prevItem.getTimestamp().secsTo(currentItem.getTimestamp());
    return currentItem.getSender() == prevItem.getSender()
           && messagesTimeDiff < repNameAfter;

    return false;
}

bool ChatLog::shouldRenderMessage(ChatLogIdx idx) const
{
    return renderedLineStorage->contains(idx) ||
        (
            renderedLineStorage->contains(idx - 1) && idx + 1 == chatLog.getNextIdx()
        ) || renderedLineStorage->empty();
}

void ChatLog::disableSearchText()
{
    if (!renderedLineStorage->contains(searchPos.logIdx)) {
        return;
    }

    auto line = (*renderedLineStorage)[searchPos.logIdx];
    auto text = qobject_cast<Text*>(line->getContent(1));
    text->deselectText();
}

void ChatLog::removeSearchPhrase()
{
    disableSearchText();
}

void ChatLog::jumpToDate(QDate date) {
    auto idx = firstItemAfterDate(date, chatLog);
    jumpToIdx(idx);
}

void ChatLog::jumpToIdx(ChatLogIdx idx)
{
    if (idx == chatLog.getNextIdx()) {
        idx = chatLog.getNextIdx() - 1;
    }

    if (renderedLineStorage->contains(idx)) {
        scrollToLine((*renderedLineStorage)[idx]);
        return;
    }

    // If the requested idx is not currently rendered we need to request a
    // render and jump to the requested line after the render completes
    renderCompletionFns.push_back([this, idx] {
        if (renderedLineStorage->contains(idx)) {
            scrollToLine((*renderedLineStorage)[idx]);
        }
    });

    // If the chatlog is empty it's likely the user has just cleared. In this
    // case it makes more sense to present the jump as if we're coming from the
    // bottom
    if (renderedLineStorage->hasIndexedMessage() && idx > renderedLineStorage->lastIdx()) {
        setRenderedWindowEnd(idx);
    }
    else {
        setRenderedWindowStart(idx);
    }
}
