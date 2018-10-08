#ifndef I_FRIEND_SETTINGS_H
#define I_FRIEND_SETTINGS_H

#include "src/model/interface.h"
#include "src/persistence/autoaccepttypes.h"

#include <QFlag>
#include <QObject>

class ToxPk;

class IFriendSettings
{
public:
    virtual QString getContactNote(const ToxPk& pk) const = 0;
    virtual void setContactNote(const ToxPk& pk, const QString& note) = 0;

    virtual QString getAutoAcceptDir(const ToxPk& pk) const = 0;
    virtual void setAutoAcceptDir(const ToxPk& pk, const QString& dir) = 0;

    virtual bool getAutoAcceptEnable(const ToxPk& id) const = 0;
    virtual void setAutoAcceptEnable(const ToxPk& id, bool enable) = 0;

    virtual AutoAcceptCallFlags getAutoAcceptCall(const ToxPk& pk) const = 0;
    virtual void setAutoAcceptCall(const ToxPk& pk, AutoAcceptCallFlags accept) = 0;

    virtual bool getAutoGroupInvite(const ToxPk& pk) const = 0;
    virtual void setAutoGroupInvite(const ToxPk& pk, bool accept) = 0;

    virtual QString getFriendAlias(const ToxPk& pk) const = 0;
    virtual void setFriendAlias(const ToxPk& pk, const QString& alias) = 0;

    virtual int getFriendCircleID(const ToxPk& pk) const = 0;
    virtual void setFriendCircleID(const ToxPk& pk, int circleID) = 0;

    virtual QDate getFriendActivity(const ToxPk& pk) const = 0;
    virtual void setFriendActivity(const ToxPk& pk, const QDate& date) = 0;

    virtual void saveFriendSettings(const ToxPk& pk) = 0;
    virtual void removeFriendSettings(const ToxPk& pk) = 0;

signals:
    DECLARE_SIGNAL(autoAcceptCallChanged, const ToxPk& pk, AutoAcceptCallFlags accept);
    DECLARE_SIGNAL(autoGroupInviteChanged, const ToxPk& pk, bool accept);
    DECLARE_SIGNAL(autoAcceptEnableChanged, const ToxPk& pk, bool enable);
    DECLARE_SIGNAL(autoAcceptDirChanged, const ToxPk& pk, const QString& dir);
    DECLARE_SIGNAL(contactNoteChanged, const ToxPk& pk, const QString& note);
};

#endif // I_FRIEND_SETTINGS_H
