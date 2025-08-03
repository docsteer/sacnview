#include "securesacn.h"
#include "streamcommon.h"
#include "defpack.h"
#include "preferences.h"

#include "blake2.h"
#include <QDataStream>
#include <QThread>
#include <string.h>
#include <QDateTime>

PathwaySecure::Password::Password(QString password) :
    password(password),
    fingerprint(RootLayer::PostAmble::FINGERPRINT_SIZE, 0x00)
{
    // Trim/Pass password as required
    this->password.resize(Password::SIZE, QChar(0x00));

    // Generate fingerprint
    blake2s(
        this->fingerprint.data(), this->fingerprint.size(), // Resultant fingerprint
        this->password.toUtf8().constData(), this->password.size(), // We are hashing the password
        nullptr, 0); // No Key
}

bool PathwaySecure::RootLayer::PostAmble::GetBuffer(quint8 *inbuf, size_t buflen, quint8 **outbuf)
{
    *outbuf = nullptr;

    if(!inbuf)
       return false;

    if (buflen < STREAM_HEADER_SIZE)
        return false;

    const quint16 slot_count = UpackBUint16(inbuf + PROP_COUNT_ADDR) - 1;
    const quint16 addr = STREAM_HEADER_SIZE + slot_count;
    if (buflen != addr + RootLayer::POSTAMBLE_SIZE)
        return false;

    *outbuf = inbuf + addr;
    return true;
}

PathwaySecure::Sequence::Sequence()
{
    // Restore non volatile sequence numbers
    QByteArray ba = Preferences::Instance().GetPathwaySecureSequenceMap();
    QDataStream ds(&ba, QIODevice::ReadOnly);
    ds >> last[type_nonvolatile];
    qDebug() << "PathwaySecure" << QThread::currentThreadId() << ": Loaded" << last[type_nonvolatile].size() << "Non-volatile sequence value(s)";

    // Load boot count, and increase
    bootCount = Preferences::Instance().GetPathwaySecureRxSequenceBootCount() + 1;
    qDebug() << "PathwaySecure" << QThread::currentThreadId() << ": Using boot count" << bootCount;
}

PathwaySecure::Sequence::~Sequence()
{    
    // Store non volatile sequence numbers
    QByteArray ba;
    QDataStream ds(&ba, QIODevice::WriteOnly);
    ds << last[type_nonvolatile];
    Preferences::Instance().SetPathwaySecureSequenceMap(ba);
    qDebug() << "PathwaySecure" << QThread::currentThreadId() << ": Saved" << last[type_nonvolatile].size() << "Non-volatile sequence value(s)";

    // Save boot count
    Preferences::Instance().SetPathwaySecureRxSequenceBootCount(bootCount);
    qDebug() << "PathwaySecure" << QThread::currentThreadId() << ": Saved boot count" << bootCount;

    // Save to disk now
    Preferences::Instance().savePreferences();
}

bool PathwaySecure::Sequence::validate(const CID &cid, type_t type, value_t value, bool expired)
{
    const value_t last_value = last[type][cid];
    last[type][cid] = value;
    qint64 diff = last_value - value;

    /* The value didn't increase
     * but was that because we've just send this?
     * Check if the sender is local, and if so account for this
     */
    if (diff == 0) {
        const auto localSenders = sACNManager::Instance().getSenderList();
        if (localSenders.count(cid))
            --diff;
    }

    switch (type)
    {
        case type_time:
            /*
             * A sender that wishes to use time as the basis for sequencing packets shall set the Sequence Type field to
             * 0. The sequence field shall be set to the current time in milliseconds elapsed since January 1, 1970,
             * 00:00.0 UTC. Implementers of transmitters are advised to consider how they will prevent this sequence
             * from going “backward” as this may cause receivers to reject their packets.
             */
            {
                const auto window = Preferences::Instance().GetPathwaySecureRxSequenceTimeWindow();
                const decltype(diff) time_diff = QDateTime::currentMSecsSinceEpoch() - value;
                if (abs(time_diff) > window)
                    return false;
            }
        break;

        case type_volatile:
            /*
             * A sender that wishes to use a simple incrementing sequence shall set the Sequence Type field to 1 and
             * initialize the sequence field to 1 before beginning transmission of a stream. For each successive packet
             * the sender shall increment the sequence field.
             */
            if (expired)
                diff = 0 - value; // Reset sequence for expired sources
            break;

        case type_nonvolatile:
            /*
             * A sender that wishes to use a non-volatile sequence type shall set Sequence Type field to 2. Senders
             * using a non volatile sequence type must guarantee that they will not send a lower value than they have
             * sent before. This may be accomplished using a boot count value that is maintained by the device that is
             * shifted into the upper range of this sequence field. The exact mechanism used by the sender to create
             * the non-volatile sequence is beyond the scope of this document.
             */
            break;

        default:
            return false;
    }

    // Sequence order ok?
    const bool sequence_ok =
            diff != std::clamp(diff, static_cast<qint64>(Sequence::value_t::MINIMUM), static_cast<qint64>(Sequence::value_t::MAXIMUM));

    return sequence_ok;
}

PathwaySecure::Sequence::value_t PathwaySecure::Sequence::next(const CID &cid, type_t type)
{
    value_t value = 0;
    if (!last[type].contains(cid))
        last[type][cid] = 0;

    switch (type)
    {
        case type_time:
            /*
             * A sender that wishes to use time as the basis for sequencing packets shall set the Sequence Type field to
             * 0. The sequence field shall be set to the current time in milliseconds elapsed since January 1, 1970,
             * 00:00.0 UTC. Implementers of transmitters are advised to consider how they will prevent this sequence
             * from going “backward” as this may cause receivers to reject their packets.
             */
            value = QDateTime::currentMSecsSinceEpoch();
            break;

        case type_volatile:
            /*
             * A sender that wishes to use a simple incrementing sequence shall set the Sequence Type field to 1 and
             * initialize the sequence field to 1 before beginning transmission of a stream. For each successive packet
             * the sender shall increment the sequence field.
             */
            value = last[type][cid] + 1;
            break;

        case type_nonvolatile:
            /*
             * A sender that wishes to use a non-volatile sequence type shall set Sequence Type field to 2. Senders
             * using a non volatile sequence type must guarantee that they will not send a lower value than they have
             * sent before. This may be accomplished using a boot count value that is maintained by the device that is
             * shifted into the upper range of this sequence field. The exact mechanism used by the sender to create
             * the non-volatile sequence is beyond the scope of this document.
             */
            value.setUpper(bootCount);
            value.setLower(last[type][cid] + 1);
            break;
    }

    last[type][cid] = value;
    return value;
}

bool PathwaySecure::VerifyStreamHeader(
        const quint8* pbuf, size_t buflen,
        CID &source_cid, char* source_name,
        quint8 &priority, quint8 &start_code,
        quint16 &synchronization, quint8 &sequence,
        quint8 &options, quint16 &universe,
        quint16 &slot_count, const quint8* &pdata)
{
    if(!pbuf)
        return false;

    /* Check amble size and root vector */
    if (buflen < ROOT_VECTOR_ADDR + sizeof(RootLayer::RootLayer::VECTOR))
        return false;
    if (UpackBUint16(pbuf + PREAMBLE_SIZE_ADDR) != RootLayer::PREAMBLE_SIZE)
        return false;
    if (UpackBUint16(pbuf + POSTAMBLE_SIZE_ADDR) != RootLayer::POSTAMBLE_SIZE)
        return false;
    if (UpackBUint32(pbuf + ROOT_VECTOR_ADDR) != RootLayer::VECTOR)
        return false;

    /* Verify header as if release version. The stock VerifyStreamHeader assumes root vector is VECTOR_ROOT_E131_DATA */
    return ::VerifyStreamHeader(
                pbuf, buflen,
                source_cid, source_name,
                priority, start_code,
                synchronization, sequence,
                options, universe,
                slot_count, pdata);
}

void PathwaySecure::InitStreamHeader(
                quint8* pbuf, const CID &source_cid,
                const char* source_name, quint8 priority,
                quint16 synchronization, quint8 options,
                quint8 start_code, quint16 universe,
                quint16 slot_count)
{
    // Init header for release version
    ::InitStreamHeader(
                pbuf, source_cid,
                source_name, priority,
                synchronization, options,
                start_code, universe,
                slot_count);

    // Update root layer
    PackBUint16(pbuf + PREAMBLE_SIZE_ADDR, RootLayer::PREAMBLE_SIZE);
    PackBUint16(pbuf + POSTAMBLE_SIZE_ADDR, RootLayer::POSTAMBLE_SIZE);
    PackBUint32(pbuf + ROOT_VECTOR_ADDR,  RootLayer::VECTOR);
}

bool PathwaySecure::VerifyStreamSecurity(const quint8* pbuf, size_t buflen, password_t password, sACNSource &source)
{
    auto &instance = getInstance();
    if(!pbuf)
       return false;

    source.pathway_secure.passwordOk = false;
    source.pathway_secure.digestOk = false;
    source.pathway_secure.sequenceOk = false;

    const quint8* post_amble_buf = nullptr;
    if (!RootLayer::PostAmble::GetBuffer(
                const_cast<quint8*>(pbuf), buflen, const_cast<quint8**>(&post_amble_buf)))
        return false;

    if (!post_amble_buf)
        return false;

    const QByteArray key_fingerprint =
            Upack(post_amble_buf + RootLayer::PostAmble::FINGERPRINT_ADDR, RootLayer::PostAmble::FINGERPRINT_SIZE);
    const Sequence::type_t sequence_type =
            static_cast<Sequence::type_t>(UpackBUint8(post_amble_buf + RootLayer::PostAmble::SEQUENCE_TYPE_ADDR));
    const Sequence::value_t sequence =
            UpackBN<Sequence::value_t>(post_amble_buf + RootLayer::PostAmble::SEQUENCE_ADDR, RootLayer::PostAmble::SEQUENCE_SIZE);
    const QByteArray message_digest =
            Upack(post_amble_buf + RootLayer::PostAmble::MESSAGE_DIGEST_ADDR, RootLayer::PostAmble::MESSAGE_DIGEST_SIZE);

    // Verify Key Fingerprint
    source.pathway_secure.passwordOk = password.getFingerprint() == key_fingerprint;
    if (!source.pathway_secure.passwordOk)
        return false;

    // Verify Sequence
    source.pathway_secure.sequenceOk = instance.sequence.validate(source.src_cid, sequence_type, sequence, source.active.Expired());
    if (!source.pathway_secure.sequenceOk)
        return false;

    // Verify Message Digest
    QByteArray expected_message_digest(RootLayer::PostAmble::MESSAGE_DIGEST_SIZE, 0x00);
    blake2s(
        expected_message_digest.data(), expected_message_digest.size(), // Resultant Message Digest
        pbuf, buflen - RootLayer::PostAmble::MESSAGE_DIGEST_SIZE, // Payload, sans Message Digest
        password.getPassword().toUtf8().constData(), password.getPassword().size()); // Password
    source.pathway_secure.digestOk = (expected_message_digest == message_digest);
    if (!source.pathway_secure.digestOk)
        return false;

    return source.pathway_secure.isSecure();
}

bool PathwaySecure::ApplyStreamSecurity(quint8* pbuf, size_t buflen, const CID &cid, const password_t &password)
{
    auto &instance = getInstance();

    quint8* post_amble_buf = nullptr;
    if (!RootLayer::PostAmble::GetBuffer(pbuf, buflen, &post_amble_buf))
        return false;

    if (!post_amble_buf)
        return false;

    // Key Fingerprint
    const auto key_fingerprint = password.getFingerprint();
    if (key_fingerprint.size() !=  RootLayer::PostAmble::FINGERPRINT_SIZE)
        return false;
    Pack(post_amble_buf + RootLayer::PostAmble::FINGERPRINT_ADDR, RootLayer::PostAmble::FINGERPRINT_SIZE, key_fingerprint);

    // Sequence Type
    const Sequence::type_t sequence_type = static_cast<Sequence::type_t>(Preferences::Instance().GetPathwaySecureTxSequenceType());
    PackBUint8(post_amble_buf + RootLayer::PostAmble::SEQUENCE_TYPE_ADDR, sequence_type);

    // Sequence Value
    const Sequence::value_t sequence = instance.sequence.next(cid, sequence_type);
    PackBN(post_amble_buf + RootLayer::PostAmble::SEQUENCE_ADDR, RootLayer::PostAmble::SEQUENCE_SIZE, sequence);

    // Message Digest
    QByteArray message_digest(RootLayer::PostAmble::MESSAGE_DIGEST_SIZE, 0x00);
    blake2s(
        message_digest.data(), message_digest.size(), // Resultant Message Digest
        pbuf, buflen - RootLayer::PostAmble::MESSAGE_DIGEST_SIZE, // Payload, sans Message Digest
        password.getPassword().toUtf8().constData(), password.getPassword().size()); // Password
    Pack(post_amble_buf + RootLayer::PostAmble::MESSAGE_DIGEST_ADDR, RootLayer::PostAmble::MESSAGE_DIGEST_SIZE, message_digest);

    return true;
}
