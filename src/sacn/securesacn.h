#ifndef SECURESACN_H
#define SECURESACN_H

#include "CID.h"
#include "src/sacn/streamingacn.h"
#include <QString>

#define VECTOR_ROOT_E131_DATA_PATHWAY_SECURE 0x50430001

/**
 * @brief Pathway Connectivity Secure DMX Protocol
 */
class PathwaySecure
{
    public:
        PathwaySecure() {}
        ~PathwaySecure() {}
        PathwaySecure(PathwaySecure const&) = delete;
        void operator=(PathwaySecure const&)  = delete;

        typedef struct Password {
            public:
                Password(QString password);

                /**
                 * @brief Get Fingerprint (Hash) of password
                 * @return Resultant fingerprint
                 */
                QByteArray getFingerprint();

                /**
                 * @brief Returns a pointer to the data stored in the Password
                 */
                inline const QChar *constData() const { return password.constData(); }

                /**
                 * @brief Size of data
                 */
                inline qsizetype size() const { return password.size(); }

            private:
                /**
                 * @brief Fixed size of password
                 */
                static const size_t SIZE = 32;

                QString password;
        } password_t;

        /**
         * @brief Verify and dissect packet header
         * @param[in] pbuf Packet to validate and dissect
         * @param buflen Size of pbuf
         * @param[out] source_cid Source CID
         * @param[out] source_name Source Name
         * @param[out] priority Priority of source
         * @param[out] start_code DMX Data Start Code
         * @param[out] synchronization Has the synchronization option been set
         * @param[out] sequence Sequence number
         * @param[out] options Source options
         * @param[out] universe Universe number
         * @param[out] slot_count Number of DMX Slots
         * @param[out] pdata DMX Data
         * @return True if header ok
         */
        static bool VerifyStreamHeader(quint8* pbuf, uint buflen, CID &source_cid,
                    char* source_name, quint8 &priority,
                    quint8 &start_code, quint16 &synchronization, quint8 &sequence,
                    quint8 &options, quint16 &universe,
                    quint16 &slot_count, quint8* &pdata);

        /**
         * @brief Verify the security features of packet
         * @details Sets the appropite flags in the sACNSource, based on the result
         * @param[in] pbuf Packet to verify
         * @param buflen Size of pbuf
         * @param password Password to use to verify packet
         * @param[in,out] source sACNSource to apply flags to
         * @return True if packet passes security validation
         */
        static bool VerifyStreamSecurity(
                const quint8* pbuf, uint buflen,
                password_t password,
                sACNSource &source);

    private:
        static PathwaySecure& getInstance()
        {
            static PathwaySecure instance;
            return instance;
        }

        struct RootLayer {
            static const size_t PREAMBLE_SIZE = RLP_PREAMBLE_SIZE;
            static const size_t POSTAMBLE_SIZE = 28;

            static const quint32 VECTOR = VECTOR_ROOT_E131_DATA_PATHWAY_SECURE;

            struct PostAmble {
                static const quint8 FINGERPRINT_ADDR = 0;
                static const size_t FINGERPRINT_SIZE = 4;

                static const quint8 SEQUENCE_TYPE_ADDR = FINGERPRINT_ADDR + FINGERPRINT_SIZE;
                static const size_t SEQUENCE_TYPE_SIZE = 1;

                static const quint8 SEQUENCE_ADDR = SEQUENCE_TYPE_ADDR + SEQUENCE_TYPE_SIZE;
                static const size_t SEQUENCE_SIZE = 7;

                static const quint8 MESSAGE_DIGEST_ADDR = SEQUENCE_ADDR + SEQUENCE_SIZE;
                static const size_t MESSAGE_DIGEST_SIZE = 16;
            };
        };

        struct Sequence {
            public:
                Sequence();
                ~Sequence();
                Sequence(PathwaySecure const&) = delete;
                void operator=(Sequence const&)  = delete;

                typedef quint64 value_t;
                typedef enum : quint8 {
                    type_time = 0, // Time Based Sequence Type
                    type_volatile = 1, // Volatile Sequence Type
                    type_nonvolatile = 2 // Non-Volatile Sequence Type
                } type_t;

                /**
                 * @brief Validate Sequence number
                 * @param cid Component IDentifer of source
                 * @param type Sequence type
                 * @param value Sequence number
                 * @param expired Had source previously expired/timed out?
                 * @return True if the sequence is correct
                 */
                bool validate(const CID &cid, type_t type, value_t value, bool expired = false);

            private:
                /**
                 * @brief Minimum value of sequence field
                 */
                static const value_t MINIMUM = 0x00000000000000; // 7 Bytes
                /**
                 * @brief Maximum value of sequence field
                 */
                static const value_t MAXIMUM = 0xFFFFFFFFFFFFFF; // 7 Bytes

                /**
                 * @brief Map of a CIDs last sequence number
                 */
                typedef QMap<CID, Sequence::value_t> lastMap_t;
                lastMap_t last;
        };
        Sequence sequence;

        typedef enum {
            security_result_ok, // No problems found
            security_result_invalid_password, // Invliad password
            security_result_invalid_sequence, // Out of sequence
            security_result_invalid_digest, // Message diget incorrect, altered on route?
            security_result_invalid_packet // General failure of packet, i.e. size
        } security_result_t;
        /**
         * @brief Helper function to set correct sACNSource flags after a security verification
         * @param source[in,out] sACNSource to apply flags to
         * @param result Result of security verification
         * @return
         */
        static bool setSourceSecureResult(sACNSource &source, security_result_t result);
};

#endif // SECURESACN_H
