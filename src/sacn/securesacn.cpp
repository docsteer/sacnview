#include "securesacn.h"
#include "streamcommon.h"
#include "defpack.h"
#include "preferences.h"

#include "blake2.h"
#include <QDataStream>
#include <QThread>

PathwaySecure::Password::Password(QString password) : password(password)
{
    this->password.resize(Password::SIZE, QChar(0x00));
}

QByteArray PathwaySecure::Password::getFingerprint()
{
    QByteArray fingerprint(RootLayer::PostAmble::FINGERPRINT_SIZE, 0x00);
    blake2s(
        fingerprint.data(), fingerprint.size(), // Resultant fingerprint
        password.constData(), password.size(), // We are hashing the password
        nullptr, 0); // No Key

    return fingerprint;
}

PathwaySecure::Sequence::Sequence()
{
    QByteArray ba = Preferences::getInstance()->GetPathwaySecureSequenceMap();
    QDataStream ds(&ba, QIODevice::ReadOnly);
    ds >> last;
    qDebug() << "PathwaySecure" << QThread::currentThreadId() << ": Loaded" << last.size() << "Sequence value(s)";
}

PathwaySecure::Sequence::~Sequence()
{    
    QByteArray ba;
    QDataStream ds(&ba, QIODevice::WriteOnly);
    ds << last;
    Preferences::getInstance()->SetPathwaySecureSequenceMap(ba);
    qDebug() << "PathwaySecure" << QThread::currentThreadId() << ": Saved" << last.size() << "Sequence value(s)";
}

bool PathwaySecure::Sequence::validate(const CID &cid, type_t type, value_t value, bool expired)
{
    const value_t last_value = last[cid];
    last[cid] = value;
    qint64 diff = last_value - value;
    static_assert(std::numeric_limits<decltype(diff)>::max() > Sequence::MAXIMUM);
    static_assert(std::numeric_limits<decltype(diff)>::min() > Sequence::MINIMUM);

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
                const auto window = Preferences::getInstance()->GetPathwaySecureSequenceTimeWindow();
                if (abs(diff) > window)
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
            diff != std::clamp(diff, static_cast<qint64>(Sequence::MINIMUM), static_cast<qint64>(Sequence::MAXIMUM));

    return sequence_ok;
}

bool PathwaySecure::VerifyStreamHeader(quint8* pbuf, uint buflen, CID &source_cid,
            char* source_name, quint8 &priority,
            quint8 &start_code, quint16 &synchronization, quint8 &sequence,
            quint8 &options, quint16 &universe,
            quint16 &slot_count, quint8* &pdata)
{
  if(!pbuf)
     return false;

  /* Do a little packet validation */
  if(buflen < STREAM_HEADER_SIZE)
  {
      return false;
  }
  if(UpackBUint16(pbuf + PREAMBLE_SIZE_ADDR) != RootLayer::PREAMBLE_SIZE)
  {
      return false;
  }
  if(UpackBUint16(pbuf + POSTAMBLE_SIZE_ADDR) != RootLayer::POSTAMBLE_SIZE)
  {
      return false;
  }
  if(memcmp(pbuf
        + ACN_IDENTIFIER_ADDR, ACN_IDENTIFIER, ACN_IDENTIFIER_SIZE) != 0)
  {
      return false;
  }
  //don't have to check the root vector, since that test has already
  //been performed.
  if(UpackBUint32(pbuf + FRAMING_VECTOR_ADDR) != VECTOR_E131_DATA_PACKET)
  {
      return false;
  }
  if(UpackBUint8(pbuf + DMP_VECTOR_ADDR) != VECTOR_DMP_SET_PROPERTY)
  {
      return false;
  }
  if(UpackBUint8(pbuf + DMP_ADDRESS_AND_DATA_ADDR)
     != DMP_ADDRESS_AND_DATA_FORMAT)
  {
      return false;
  }
  if(UpackBUint16(pbuf + FIRST_PROPERTY_ADDRESS_ADDR)
     != DMP_FIRST_PROPERTY_ADDRESS_FORCE)
  {
      return false;
  }
  if(UpackBUint16(pbuf + ADDRESS_INC_ADDR) != ADDRESS_INC)
  {
      return false;
  }

  /* Init the parameters */
  source_cid.Unpack(pbuf + CID_ADDR);

  strncpy_s(source_name, SOURCE_NAME_SIZE, (char*)(pbuf + SOURCE_NAME_ADDR), SOURCE_NAME_SIZE);
  source_name[SOURCE_NAME_SIZE-1] = '\0';
  priority = UpackBUint8(pbuf + PRIORITY_ADDR);
  start_code = UpackBUint8(pbuf + START_CODE_ADDR);
  synchronization = UpackBUint16(pbuf + SYNC_ADDR);
  sequence = UpackBUint8(pbuf + SEQ_NUM_ADDR);
  options = UpackBUint8(pbuf + OPTIONS_ADDR);
  universe = UpackBUint16(pbuf + UNIVERSE_ADDR);
  slot_count = UpackBUint16(pbuf + PROP_COUNT_ADDR) - 1;  //The property value count includes the start code byte
  pdata = pbuf + STREAM_HEADER_SIZE;

  /*Do final length validation*/
  if((pdata + slot_count) > (pbuf + buflen))
    return false;

  return true;
}

bool PathwaySecure::setSourceSecureResult(sACNSource &source, security_result_t result)
{
    switch (result) {
        case security_result_ok:
            source.pathway_secure.passwordOk = true;
            source.pathway_secure.sequenceOk = true;
            source.pathway_secure.digetOk = true;
            return true;

        case security_result_invalid_password:
            source.pathway_secure.passwordOk = false;
            return false;

        case security_result_invalid_sequence:
            source.pathway_secure.sequenceOk = true;
            return false;

        case security_result_invalid_digest:
            source.pathway_secure.digetOk = false;
            return false;

        case security_result_invalid_packet:
            source.pathway_secure.passwordOk = false;
            source.pathway_secure.sequenceOk = false;
            source.pathway_secure.digetOk = false;
            return false;
    };

    return false;
}

bool PathwaySecure::VerifyStreamSecurity(const quint8* pbuf, uint buflen, password_t password, sACNSource &source)
{
    auto &instance = getInstance();

    if(!pbuf)
       return instance.setSourceSecureResult(source, security_result_invalid_packet);

    // Expected location of the post amble
    quint16 slot_count = UpackBUint16(pbuf + PROP_COUNT_ADDR) - 1;
    quint16 post_amble_addr = STREAM_HEADER_SIZE + slot_count;

    // Verify post amble size
    if (buflen != post_amble_addr + RootLayer::POSTAMBLE_SIZE)
        return instance.setSourceSecureResult(source, security_result_invalid_packet);

    // Extract Post Amble
    const quint8* post_amble_buf = pbuf + post_amble_addr;
    const QByteArray key_fingerprint = QByteArray(
                reinterpret_cast<const char*>(post_amble_buf + RootLayer::PostAmble::FINGERPRINT_ADDR),
                RootLayer::PostAmble::FINGERPRINT_SIZE);
    const Sequence::type_t sequence_type =
            static_cast<Sequence::type_t>(UpackBUint8(post_amble_buf + RootLayer::PostAmble::SEQUENCE_TYPE_ADDR));
    const Sequence::value_t sequence = UpackBUint56(post_amble_buf + RootLayer::PostAmble::SEQUENCE_ADDR);
    const QByteArray message_digest = QByteArray(
                reinterpret_cast<const char*>(post_amble_buf + RootLayer::PostAmble::MESSAGE_DIGEST_ADDR),
                RootLayer::PostAmble::MESSAGE_DIGEST_SIZE);

    // Verify Key Fingerprint
    if (password.getFingerprint() != key_fingerprint)
        return instance.setSourceSecureResult(source, security_result_invalid_password);

    // Verify Sequence
    if (!instance.sequence.validate(source.src_cid, sequence_type, sequence, source.active.Expired()))
        return instance.setSourceSecureResult(source, security_result_invalid_sequence);

    // Verify Message Digest
    QByteArray expected_message_digest(RootLayer::PostAmble::MESSAGE_DIGEST_SIZE, 0x00);
    blake2s(
        expected_message_digest.data(), expected_message_digest.size(), // Resultant Message Digest
        pbuf, buflen - RootLayer::PostAmble::MESSAGE_DIGEST_SIZE, // Payload, sans Message Digest
        password.constData(), password.size()); // Password
    if (expected_message_digest != message_digest)
        return instance.setSourceSecureResult(source, security_result_invalid_digest);

    return instance.setSourceSecureResult(source, security_result_ok);
}
