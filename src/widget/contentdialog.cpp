/*
    Copyright © 2015-2019 by The qTox Project Contributors

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

#include "contentdialog.h"
#include "splitterrestorer.h"

#include <QBoxLayout>
#include <QCloseEvent>
#include <QDragEnterEvent>
#include <QGuiApplication>
#include <QMimeData>
#include <QShortcut>
#include <QSplitter>

#include "src/core/core.h"
#include "src/friendlist.h"
#include "src/grouplist.h"
#include "src/model/friend.h"
#include "src/model/group.h"
#include "src/model/status.h"
#include "src/persistence/settings.h"
#include "src/widget/contentlayout.h"
#include "src/widget/form/chatform.h"
#include "src/widget/friendlistwidget.h"
#include "src/widget/style.h"
#include "src/widget/tool/adjustingscrollarea.h"
#include "src/widget/translator.h"
#include "src/widget/widget.h"

static const int minWidget = 220;
static const int minHeight = 220;
static const QSize minSize(minHeight, minWidget);
static const QSize defaultSize(720, 400);

ContentDialog::ContentDialog(QWidget* parent)
    : ActivateDialog(parent, Qt::Window)
    , splitter{new QSplitter(this)}
    , friendList(new FriendListWidget(this))
    , activeContact(nullptr)
    , videoSurfaceSize(QSize())
    , videoCount(0)
{
    const Settings& s = Settings::getInstance();
    setStyleSheet(Style::getStylesheet("contentDialog/contentDialog.css"));

    QWidget* contentWidget = new QWidget(this);
    contentWidget->setAutoFillBackground(true);

    contentLayout = new ContentLayout(contentWidget);
    contentLayout->setMargin(0);
    contentLayout->setSpacing(0);

    splitter->addWidget(friendList);
    splitter->addWidget(contentWidget);
    splitter->setStretchFactor(1, 1);
    splitter->setCollapsible(1, false);

    QVBoxLayout* boxLayout = new QVBoxLayout(this);
    boxLayout->setMargin(0);
    boxLayout->setSpacing(0);
    boxLayout->addWidget(splitter);

    setMinimumSize(minSize);
    setAttribute(Qt::WA_DeleteOnClose);
    setObjectName("detached");

    QByteArray geometry = s.getDialogGeometry();

    if (!geometry.isNull()) {
        restoreGeometry(geometry);
    } else {
        resize(defaultSize);
    }

    SplitterRestorer restorer(splitter);
    restorer.restore(s.getDialogSplitterState(), size());

    username = Core::getInstance()->getUsername();

    setAcceptDrops(true);

    new QShortcut(Qt::CTRL + Qt::Key_Q, this, SLOT(close()));
    new QShortcut(Qt::CTRL + Qt::SHIFT + Qt::Key_Tab, this, SLOT(previousContact()));
    new QShortcut(Qt::CTRL + Qt::Key_Tab, this, SLOT(nextContact()));
    new QShortcut(Qt::CTRL + Qt::Key_PageUp, this, SLOT(previousContact()));
    new QShortcut(Qt::CTRL + Qt::Key_PageDown, this, SLOT(nextContact()));

    connect(splitter, &QSplitter::splitterMoved, this, &ContentDialog::saveSplitterState);

    Translator::registerHandler(std::bind(&ContentDialog::retranslateUi, this), this);
}

ContentDialog::~ContentDialog()
{
    Translator::unregister(this);
}

void ContentDialog::closeEvent(QCloseEvent* event)
{
    emit willClose();
    event->accept();
}

void ContentDialog::addFriend(Friend* frnd, GenericChatForm* form)
{
    const auto compact = Settings::getInstance().getCompactLayout();
    const auto& friendPk = frnd->getPublicKey();
    contactChatForms[friendPk] = form;

    friendList->addFriendWidget(frnd, frnd->getStatus(), 0);

    // FIXME: emit should be removed
    emit friendList->friendSelected(frnd);

}

void ContentDialog::addGroup(Group* g, GenericChatForm* form)
{
    const auto& groupId = g->getPersistentId();
    const auto compact = Settings::getInstance().getCompactLayout();
    contactChatForms[groupId] = form;

    friendList->addGroupWidget(g);

    // FIXME: emit should be removed
    emit friendList->groupSelected(g);
}

void ContentDialog::removeFriend(const ToxPk& friendPk)
{
    const auto f = FriendList::findFriend(friendPk);
    if (!f)
        return;

    friendList->removeFriendWidget(f);

    if (chatroomCount() == 0) {
        contentLayout->clear();
        activeContact = nullptr;
        deleteLater();
    } else {
        update();
    }

    contactChatForms.remove(friendPk);
    closeIfEmpty();
}

void ContentDialog::removeGroup(const GroupId& groupId)
{
    const auto group = GroupList::findGroup(groupId);

    friendList->removeGroupWidget(group);

    if (chatroomCount() == 0) {
        contentLayout->clear();
        activeContact = nullptr;
        deleteLater();
    } else {
        update();
    }

    contactChatForms.remove(groupId);
    closeIfEmpty();
}

void ContentDialog::closeIfEmpty()
{
    if (chatroomCount() == 0) {
        close();
    }
}

int ContentDialog::chatroomCount() const
{
    return contactChatForms.size();
}

void ContentDialog::ensureSplitterVisible()
{
    if (splitter->sizes().at(0) == 0) {
        splitter->setSizes({1, 1});
    }

    update();
}

void ContentDialog::onVideoShow(QSize size)
{
    ++videoCount;
    if (videoCount > 1) {
        return;
    }

    videoSurfaceSize = size;
    QSize minSize = minimumSize();
    setMinimumSize(minSize + videoSurfaceSize);
}

void ContentDialog::onVideoHide()
{
    videoCount--;
    if (videoCount > 0) {
        return;
    }

    QSize minSize = minimumSize();
    setMinimumSize(minSize - videoSurfaceSize);
    videoSurfaceSize = QSize();
}


/**
 * @brief Update window title and icon.
 */
void ContentDialog::updateTitleAndStatusIcon()
{
    if (!activeContact) {
        setWindowTitle(username);
        return;
    }

    setWindowTitle(activeContact->getDisplayedName() + QStringLiteral(" - ") + username);

    bool isGroupchat = qobject_cast<Group*>(activeContact) != nullptr;
    if (isGroupchat) {
        setWindowIcon(QIcon(":/img/group.svg"));
        return;
    }

    // FIXME: removed status icon
}

/**
 * @brief Update layouts order according to settings.
 * @param groupOnTop If true move groupchat layout on the top. Move under online otherwise.
 */
void ContentDialog::reorderLayouts(bool newGroupOnTop)
{
    bool oldGroupOnTop = layouts.first() == groupLayout.getLayout();
    if (newGroupOnTop != oldGroupOnTop) {
        // Kriby: Maintain backwards compatibility
#if (QT_VERSION >= QT_VERSION_CHECK(5, 13, 0))
        layouts.swapItemsAt(0, 1);
#else
        layouts.swap(0, 1);
#endif
    }
}

void ContentDialog::previousContact()
{
    // FIXME: implement
}

/**
 * @brief Enable next contact.
 */
void ContentDialog::nextContact()
{
    // FIXME: implement
}

/**
 * @brief Update username to show in the title.
 * @param newName New name to display.
 */
void ContentDialog::setUsername(const QString& newName)
{
    username = newName;
    updateTitleAndStatusIcon();
}

bool ContentDialog::event(QEvent* event)
{
    switch (event->type()) {
    case QEvent::WindowActivate:
        if (activeContact) {
            updateTitleAndStatusIcon();

            Friend* frnd = qobject_cast<Friend*>(activeContact);
            Group* group = qobject_cast<Group*>(activeContact);

            if (frnd) {
                emit friendDialogShown(frnd);
            } else if (group) {
                emit groupDialogShown(group);
            }
        }

        emit activated();
    default:
        break;
    }

    return ActivateDialog::event(event);
}

void ContentDialog::dragEnterEvent(QDragEnterEvent* event)
{
    // FIXME: implement
    //QObject* o = event->source();
    //FriendWidget* frnd = qobject_cast<FriendWidget*>(o);
    //GroupWidget* group = qobject_cast<GroupWidget*>(o);
    //if (frnd) {
    //    assert(event->mimeData()->hasFormat("toxPk"));
    //    ToxPk toxPk{event->mimeData()->data("toxPk")};
    //    Friend* contact = FriendList::findFriend(toxPk);
    //    if (!contact) {
    //        return;
    //    }

    //    ToxPk friendId = contact->getPublicKey();

    //    // If friend is already in a dialog then you can't drop friend where it already is.
    //    if (!hasContact(friendId)) {
    //        event->acceptProposedAction();
    //    }
    //} else if (group) {
    //    assert(event->mimeData()->hasFormat("groupId"));
    //    GroupId groupId = GroupId{event->mimeData()->data("groupId")};
    //    Group* contact = GroupList::findGroup(groupId);
    //    if (!contact) {
    //        return;
    //    }

    //    if (!hasContact(groupId)) {
    //        event->acceptProposedAction();
    //    }
    //}
}

void ContentDialog::dropEvent(QDropEvent* event)
{
    // FIXME: implement
    //QObject* o = event->source();
    //FriendWidget* frnd = qobject_cast<FriendWidget*>(o);
    //GroupWidget* group = qobject_cast<GroupWidget*>(o);
    //if (frnd) {
    //    assert(event->mimeData()->hasFormat("toxPk"));
    //    const ToxPk toxId(event->mimeData()->data("toxPk"));
    //    Friend* contact = FriendList::findFriend(toxId);
    //    if (!contact) {
    //        return;
    //    }

    //    emit addFriendDialog(contact, this);
    //    ensureSplitterVisible();
    //} else if (group) {
    //    assert(event->mimeData()->hasFormat("groupId"));
    //    const GroupId groupId(event->mimeData()->data("groupId"));
    //    Group* contact = GroupList::findGroup(groupId);
    //    if (!contact) {
    //        return;
    //    }

    //    emit addGroupDialog(contact, this);
    //    ensureSplitterVisible();
    //}
}

void ContentDialog::changeEvent(QEvent* event)
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::ActivationChange) {
        if (isActiveWindow()) {
            emit activated();
        }
    }
}

void ContentDialog::resizeEvent(QResizeEvent* event)
{
    saveDialogGeometry();
    QDialog::resizeEvent(event);
}

void ContentDialog::moveEvent(QMoveEvent* event)
{
    saveDialogGeometry();
    QDialog::moveEvent(event);
}

void ContentDialog::keyPressEvent(QKeyEvent* event)
{
    // Ignore escape keyboard shortcut.
    if (event->key() != Qt::Key_Escape) {
        QDialog::keyPressEvent(event);
    }
}

void ContentDialog::focusContact(const ContactId& contactId)
{
    // FIXME: Implement programatically selecting a contact
}

bool ContentDialog::isContactActive(const ContactId& contactId) const
{
    // FIXME: Implement isContactActive
    return false;
}

/**
 * @brief Retranslate all elements in the form.
 */
void ContentDialog::retranslateUi()
{
    updateTitleAndStatusIcon();
}

/**
 * @brief Save size of dialog window.
 */
void ContentDialog::saveDialogGeometry()
{
    Settings::getInstance().setDialogGeometry(saveGeometry());
}

/**
 * @brief Save state of splitter between dialog and dialog list.
 */
void ContentDialog::saveSplitterState()
{
    Settings::getInstance().setDialogSplitterState(splitter->saveState());
}

bool ContentDialog::hasContact(const ContactId& contactId) const
{
    return friendList->hasContact(contactId);
}

/**
 * @brief Find the next or previous layout in layout list.
 * @param layout Current layout.
 * @param forward If true, move forward, backward othwerwise.
 * @return Next/previous layout.
 */
QLayout* ContentDialog::nextLayout(QLayout* layout, bool forward) const
{
    int index = layouts.indexOf(layout);
    if (index == -1) {
        return nullptr;
    }

    int next = forward ? index + 1 : index - 1;
    size_t size = layouts.size();
    next = (next + size) % size;

    return layouts[next];
}
