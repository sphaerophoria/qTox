#include "coreext.h"
#include "toxstring.h"

#include <QDateTime>
#include <QTimeZone>
#include <QtCore>

#include <memory>

extern "C" {
#include <toxext/toxext.h>
#include <tox_extension_messages.h>
#include <tox_extension_sender_timestamp.h>
}

std::unique_ptr<CoreExt> CoreExt::makeCoreExt(Tox* core) {
    auto toxExtPtr = toxext_init(core);
    if (!toxExtPtr) {
        return nullptr;
    }

    auto toxExt = ExtensionPtr<ToxExt>(toxExtPtr, toxext_free);
    return std::unique_ptr<CoreExt>(new CoreExt(std::move(toxExt)));
}

CoreExt::CoreExt(ExtensionPtr<ToxExt> toxExt_)
    : toxExt(std::move(toxExt_))
    , toxExtMessages(nullptr, nullptr)
    , toxExtSenderTimestamp(nullptr, nullptr)
{
    toxExtMessages = ExtensionPtr<ToxExtensionMessages>(
        tox_extension_messages_register(
            toxExt.get(),
            CoreExt::onExtendedMessageReceived,
            CoreExt::onExtendedMessageReceipt,
            CoreExt::onExtendedMessageNegotiation,
            this),
        tox_extension_messages_free);

    toxExtSenderTimestamp = ExtensionPtr<ToxExtensionSenderTimestamp>(
        tox_extension_sender_timestamp_register(
            toxExt.get(),
            CoreExt::onSenderTimestampReceived,
            CoreExt::onSenderTimestampNegotiation,
            this),
        tox_extension_sender_timestamp_free);
}

void CoreExt::process()
{
    toxext_iterate(toxExt.get());
}

void CoreExt::onLosslessPacket(uint32_t friendId, const uint8_t* data, size_t length)
{
    if (is_toxext_packet(data, length)) {
        toxext_handle_lossless_custom_packet(toxExt.get(), friendId, data, length);
    }
}

CoreExt::Packet::Packet(
    ToxExtPacketList* packetList,
    ToxExtensionMessages* toxExtMessages,
    ToxExtensionSenderTimestamp* toxExtSenderTimestamp,
    PacketPassKey)
    : toxExtMessages(toxExtMessages)
    , toxExtSenderTimestamp(toxExtSenderTimestamp)
    , packetList(packetList)
{}

std::unique_ptr<ICoreExtPacket> CoreExt::getPacket(uint32_t friendId)
{
    return std::unique_ptr<Packet>(new Packet(
        toxext_packet_list_create(toxExt.get(), friendId),
        toxExtMessages.get(),
        toxExtSenderTimestamp.get(),
        PacketPassKey{}));
}

void CoreExt::Packet::addSenderTimestamp(QDateTime now)
{
    if (hasBeenSent) {
        assert(false);
        qWarning() << "Invalid use of CoreExt::Packet";
        return;
    }

    tox_extension_sender_timestamp_append(
        toxExtSenderTimestamp,
        packetList,
        now.toUTC().toSecsSinceEpoch());
}

uint64_t CoreExt::Packet::addExtendedMessage(QString message)
{
    if (hasBeenSent) {
        assert(false);
        qWarning() << "Invalid use of CoreExt::Packet";
        return UINT64_MAX;
    }

    ToxString toxString(message);
    return tox_extension_messages_append(
        toxExtMessages,
        packetList,
        toxString.data(),
        toxString.size());
}

bool CoreExt::Packet::send()
{
    auto ret = toxext_send(packetList);
    if (ret != TOXEXT_SUCCESS) {
        qWarning() << "Failed to send packet";
    }
    hasBeenSent = true;
    return ret == TOXEXT_SUCCESS;
}

void CoreExt::onFriendStatusChanged(uint32_t friendId, Status::Status status)
{
    if (status != Status::Status::Offline) {
        tox_extension_messages_negotiate(toxExtMessages.get(), friendId);
        tox_extension_sender_timestamp_negotiate(toxExtSenderTimestamp.get(), friendId);
    }
    else {
        emit extendedMessageSupport(friendId, false);
        emit senderTimestampSupport(friendId, false);
    }
}

void CoreExt::onExtendedMessageReceived(uint32_t friendId, const uint8_t* data, size_t size, void* userData)
{
    QString msg = ToxString(data, size).getQString();
    emit static_cast<CoreExt*>(userData)->extendedMessageReceived(friendId, msg);
}

void CoreExt::onExtendedMessageReceipt(uint32_t friendId, uint64_t receiptId, void* userData)
{
    emit static_cast<CoreExt*>(userData)->extendedReceiptReceived(friendId, receiptId);
}

void CoreExt::onExtendedMessageNegotiation(uint32_t friendId, bool compatible, void* userData)
{
    auto coreExt = static_cast<CoreExt*>(userData);
    emit coreExt->extendedMessageSupport(friendId, compatible);
}

void CoreExt::onSenderTimestampReceived(uint32_t friendId, uint64_t timestamp, void* userData)
{
    // parse and emit something here
    auto senderTime = QDateTime::fromSecsSinceEpoch(timestamp, QTimeZone::utc()).toLocalTime();
    emit static_cast<CoreExt*>(userData)->senderTimestampReceived(friendId, senderTime);
}

void CoreExt::onSenderTimestampNegotiation(uint32_t friendId, bool compatible, void* userData)
{
    auto coreExt = static_cast<CoreExt*>(userData);
    emit coreExt->senderTimestampSupport(friendId, compatible);
}
