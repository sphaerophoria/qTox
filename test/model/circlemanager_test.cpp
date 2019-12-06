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

#include "src/core/toxpk.h"

#include "src/model/friend.h"

#include "src/friendlist.h"

#include "src/persistence/ifriendsettings.h"
#include "src/persistence/icirclesettings.h"

#include "src/model/circlemanager.h"

#include <QDateTime>
#include <QObject>
#include <QtTest/QtTest>

#include <unordered_map>

class MockFriendSettings : public QObject, public IFriendSettings
{
    Q_OBJECT
public:
    QString getContactNote(const ToxPk& pk) const override { return QString(); }
    void setContactNote(const ToxPk& pk, const QString& note) override {}

    QString getAutoAcceptDir(const ToxPk& pk) const override { return QString(); }
    void setAutoAcceptDir(const ToxPk& pk, const QString& dir) override {}

    AutoAcceptCallFlags getAutoAcceptCall(const ToxPk& pk) const override {return AutoAcceptCallFlags(); }
    void setAutoAcceptCall(const ToxPk& pk, AutoAcceptCallFlags accept) override {}

    bool getAutoGroupInvite(const ToxPk& pk) const override { return false; }
    void setAutoGroupInvite(const ToxPk& pk, bool accept) override {}

    QString getFriendAlias(const ToxPk& pk) const override { return QString(); }
    void setFriendAlias(const ToxPk& pk, const QString& alias) override {}

    int getFriendCircleID(const ToxPk& pk) const override
    {
        auto it = friendToCircleMap.find(pk);
        if (it == friendToCircleMap.end()) {
            return -1;
        }

        return it->second;
    }

    void setFriendCircleID(const ToxPk& pk, int circleID) override
    {
        friendToCircleMap[pk] = circleID;
    }

    QDateTime getFriendActivity(const ToxPk& pk) const override { return QDateTime(); }
    void setFriendActivity(const ToxPk& pk, const QDateTime& date) override {};

    void saveFriendSettings(const ToxPk& pk) override {}
    void removeFriendSettings(const ToxPk& pk) override {}

    SIGNAL_IMPL(MockFriendSettings, autoAcceptCallChanged, const ToxPk& id,
                IFriendSettings::AutoAcceptCallFlags accept)
    SIGNAL_IMPL(MockFriendSettings, autoGroupInviteChanged, const ToxPk& id, bool accept)
    SIGNAL_IMPL(MockFriendSettings, autoAcceptDirChanged, const ToxPk& id, const QString& dir)
    SIGNAL_IMPL(MockFriendSettings, contactNoteChanged, const ToxPk& id, const QString& note)

private:
    std::unordered_map<ToxPk, int> friendToCircleMap;
};

class MockCircleSettings : public ICircleSettings
{
public:
    int getCircleCount() const override
    {
        return circles.size();
    }

    int addCircle(const QString& name = QString()) override
    {
        circles.push_back(CircleData{name, false});
        return circles.size() - 1;
    }

    int removeCircle(int id) override
    {
        // Actual implementation does a swap and pop
        std::swap(circles.back(), circles[id]);
        circles.pop_back();
        return circles.size();
    }

    QString getCircleName(int id) const override
    {
        return circles[id].name;
    }

    void setCircleName(int id, const QString& name) override
    {
        circles[id].name = name;
    }

    bool getCircleExpanded(int id) const override
    {
        return circles[id].expanded;
    }

    void setCircleExpanded(int id, bool expanded) override
    {
        circles[id].expanded = expanded;
    }

private:
    struct CircleData
    {
        QString name;
        bool expanded;
    };

    // Using vector indexes as IDs seems bad since that invalidates ids when we
    // remove circles, however the real version of the class is kind of implemented
    // this way so we model our mock the same way
    std::vector<CircleData> circles;
};

class TestCircleManager : public QObject
{
    Q_OBJECT
public:
    TestCircleManager() = default;
private slots:

    void init();
    void testAddRemoveCircle();
    void testSetCircleName();
    void testSetCircleExpanded();
    void testAddRemoveFriendToCircle();
    void testFriendRemovedFromDeletedCircle();
    void testLoadCirclesFromDb();
    void testSaveCirclesToDb();

private:
    std::unique_ptr<MockFriendSettings> friendSettings;
    std::unique_ptr<MockCircleSettings> circleSettings;
    std::unique_ptr<CircleManager> circleManager;
};

void TestCircleManager::init()
{
    FriendList::clear();
    friendSettings.reset(new MockFriendSettings());
    circleSettings.reset(new MockCircleSettings());
    circleManager.reset(new CircleManager({}, *friendSettings, *circleSettings));
}

void TestCircleManager::testAddRemoveCircle()
{
    QVERIFY(circleManager->getCircles().size() == 0);

    auto circleId = circleManager->addCircle();

    QVERIFY(circleManager->getCircles().size() == 1);

    auto circleId2 = circleManager->addCircle();

    QVERIFY(circleId != circleId2);

    auto createdCircles = std::vector<CircleId>({circleId, circleId2});
    auto managerCircles = circleManager->getCircles();
    std::sort(managerCircles.begin(), managerCircles.end());
    std::sort(createdCircles.begin(), createdCircles.end());

    QVERIFY(managerCircles == createdCircles);

    // Remove the first circle to ensure that CircleIds exposed from CircleManager
    // are stable
    circleManager->removeCircle(circleId);

    QVERIFY(circleManager->getCircles() == std::vector<CircleId>({circleId2}));
}

void TestCircleManager::testSetCircleName()
{
    auto circleId = circleManager->addCircle();
    circleManager->setCircleName(circleId, "TestCircle");
    QVERIFY(circleManager->getCircleName(circleId) == "TestCircle");
}

void TestCircleManager::testSetCircleExpanded()
{
    auto circleId = circleManager->addCircle();
    circleManager->setCircleExpanded(circleId, true);
    QVERIFY(circleManager->getCircleExpanded(circleId) == true);

    circleManager->setCircleExpanded(circleId, false);
    QVERIFY(circleManager->getCircleExpanded(circleId) == false);
}


void TestCircleManager::testAddRemoveFriendToCircle()
{
    Friend f(0, ToxPk());
    auto circleId = circleManager->addCircle();
    QVERIFY(circleManager->getFriendCircle(&f) == CircleManager::none);

    circleManager->addFriendToCircle(&f, circleId);
    QVERIFY(circleManager->getFriendCircle(&f) == circleId);

    // Ensure that friend gets moved to different circle
    circleId = circleManager->addCircle();
    circleManager->addFriendToCircle(&f, circleId);
    QVERIFY(circleManager->getFriendCircle(&f) == circleId);

    circleManager->removeFriendFromCircle(&f, circleId);

    QVERIFY(circleManager->getFriendCircle(&f) == CircleManager::none);
}

void TestCircleManager::testFriendRemovedFromDeletedCircle()
{
    Friend f(0, ToxPk());
    auto circleId = circleManager->addCircle();
    QVERIFY(circleManager->getFriendCircle(&f) == CircleManager::none);

    circleManager->addFriendToCircle(&f, circleId);
    QVERIFY(circleManager->getFriendCircle(&f) == circleId);

    circleManager->removeCircle(circleId);
    QVERIFY(circleManager->getFriendCircle(&f) == CircleManager::none);
}

void TestCircleManager::testLoadCirclesFromDb()
{
    int id1 = circleSettings->addCircle();
    int id2 = circleSettings->addCircle();

    circleSettings->setCircleName(id1, "Test 1");
    circleSettings->setCircleExpanded(id1, false);

    circleSettings->setCircleName(id2, "Test 2");
    circleSettings->setCircleExpanded(id2, true);

    Friend f1(0, ToxPk(QByteArray(TOX_PUBLIC_KEY_SIZE, 'a')));
    Friend f2(1, ToxPk(QByteArray(TOX_PUBLIC_KEY_SIZE, 'b')));

    auto friendList = std::vector<const Friend*>({&f1, &f2});

    friendSettings->setFriendCircleID(f1.getPublicKey(), id1);
    friendSettings->setFriendCircleID(f2.getPublicKey(), id2);

    circleManager.reset(new CircleManager(friendList, *friendSettings, *circleSettings));
    auto circles = circleManager->getCircles();
    QVERIFY(circles.size() == 2);

    auto circleId1 = circleManager->getFriendCircle(&f1);
    auto circleId2 = circleManager->getFriendCircle(&f2);

    QVERIFY(circleManager->getCircleName(circleId1) ==  "Test 1");
    QVERIFY(circleManager->getCircleExpanded(circleId1) == false);

    QVERIFY(circleManager->getCircleName(circleId2) ==  "Test 2");
    QVERIFY(circleManager->getCircleExpanded(circleId2) == true);
}

void TestCircleManager::testSaveCirclesToDb()
{
    auto id1 = circleManager->addCircle();
    auto id2 = circleManager->addCircle();

    circleManager->setCircleName(id1, "Test 1");
    circleManager->setCircleName(id2, "Test 2");

    circleManager->setCircleExpanded(id1, true);
    circleManager->setCircleExpanded(id2, false);

    QVERIFY(circleSettings->getCircleCount() == 2);
    QVERIFY(circleSettings->getCircleName(0) == "Test 1");
    QVERIFY(circleSettings->getCircleExpanded(0) == true);
    QVERIFY(circleSettings->getCircleName(1) == "Test 2");
    QVERIFY(circleSettings->getCircleExpanded(1) == false);

    // Ensure that stable CircleIds map back to unstable settings ids
    circleManager->removeCircle(id1);
    circleManager->setCircleName(id2, "Test 3");
    QVERIFY(circleSettings->getCircleCount() == 1);
    QVERIFY(circleSettings->getCircleName(0) == "Test 3");
    QVERIFY(circleSettings->getCircleExpanded(0) == false);

    Friend f(0, ToxPk(QByteArray(TOX_PUBLIC_KEY_SIZE, 'a')));
    circleManager->addFriendToCircle(&f, id2);
    QVERIFY(friendSettings->getFriendCircleID(f.getPublicKey()) == 0);

    circleManager->removeFriendFromCircle(&f, id2);
    QVERIFY(friendSettings->getFriendCircleID(f.getPublicKey()) == -1);
}


QTEST_GUILESS_MAIN(TestCircleManager)
#include "circlemanager_test.moc"
