#ifndef SECURESACN_H
#define SECURESACN_H

#include "CID.h"
#include "sacn/streamingacn.h"
#include <QString>

#define VECTOR_ROOT_E131_DATA_PATHWAY_SECURE 0x50430001

/**
 * @brief Pathway Connectivity Secure DMX Protocol
 */
class PathwaySecure
{
    friend class PreferencesDialog;

    public:
        PathwaySecure() {}
        ~PathwaySecure() {}
        PathwaySecure(PathwaySecure const&) = delete;
        void operator=(PathwaySecure const&)  = delete;

        /**
         * @brief RootLayer Constants
         */
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

                /**
                 * @brief Get the pointer to the start of the post amble
                 * @param[in] inbuf Packet buffer
                 * @param buflen Size of inbuf
                 * @param[out] outbuf Pointer to post amble buffer within pbuf, nullprt on failure
                 * @return True on success
                 */
                static bool GetBuffer(quint8* inbuf, uint buflen, quint8 **outbuf);
            };
        };

        /**
         * @brief Password container type
         */
        typedef struct Password {
            public:
                Password(QString password);

                /**
                 * @brief Get Fingerprint (Hash) of password
                 */
                inline const QByteArray &getFingerprint() const { return fingerprint; }

                /**
                 * @brief Get the actual password
                 */
                inline const QString &getPassword() const { return password; }

            private:
                /**
                 * @brief Fixed size of password
                 */
                static const size_t SIZE = 32;

                QString password;
                QByteArray fingerprint;
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
        static bool VerifyStreamHeader(
                const quint8* pbuf, uint buflen,
                CID &source_cid, char* source_name,
                quint8 &priority, quint8 &start_code,
                quint16 &synchronization, quint8 &sequence,
                quint8 &options, quint16 &universe,
                quint16 &slot_count, const quint8* &pdata);

        /**
         * @brief Setup stream header
         * @param[in, out] pbuf Packet buffer to setup
         * @param[in] source_cid Source CID
         * @param[in] source_name Source Name
         * @param[in] priority Priority of source
         * @param[in] synchronization Synchronization option bits
         * @param[in] options Source options
         * @param[in] start_code DMX Data Start Code
         * @param[in] universe Universe number
         * @param[in] slot_count Number of DMX Slots
         */
        static void InitStreamHeader(
                quint8* pbuf, const CID &source_cid,
                const char* source_name, quint8 priority,
                quint16 synchronization, quint8 options,
                quint8 start_code, quint16 universe,
                quint16 slot_count);

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


        /**
         * @brief Apply security features to packet
         * @param[in,out] pbuf Packet buffer
         * @param buflen Size of pbuf
         * @param cid Component IDentifer
         * @param password Password to for hash
         * @return True if security features applied
         */
        static bool ApplyStreamSecurity(quint8* pbuf, uint buflen,
                const CID &cid, const password_t &password);

    private:
        static PathwaySecure& getInstance()
        {
            static PathwaySecure instance;
            return instance;
        }

        /**
         * @brief Sequence container and validation type
         */
        struct Sequence {
            public:
                Sequence();
                ~Sequence();
                Sequence(PathwaySecure const&) = delete;
                void operator=(Sequence const&)  = delete;

                class value_t {
                private:
                    typedef quint64 type;

                public:
                    value_t() : data(0) {}
                    value_t(type value) : data(value) {
                        data = std::clamp(data, MINIMUM, MAXIMUM);
                    }

                    /**
                     * @brief Get the upper word of the value
                     * @return Upper range value
                     */
                    type getUpper()
                    {
                        return (data & UPPER_MASK) >> UPPER_SHIFT;
                    }

                    /**
                     * @brief Get the lower word of the value
                     * @return Lower range value
                     */
                    type getLower()
                    {
                        return (data & LOWER_MASK) >> LOWER_SHIFT;
                    }

                    /**
                     * @brief Set the upper word of the value
                     * @param upper Upper range to set
                     */
                    void setUpper(type upper)
                    {
                        upper = upper << UPPER_SHIFT;
                        data &= ~UPPER_MASK;
                        data |= upper;
                        data = std::clamp(data, MINIMUM, MAXIMUM);
                    }

                    /**
                     * @brief Set the lower word of the value
                     * @param upper Upper range to set
                     */
                    void setLower(type lower)
                    {
                        lower = lower << LOWER_SHIFT;
                        data &= ~LOWER_MASK;
                        data |= lower;
                        data = std::clamp(data, MINIMUM, MAXIMUM);
                    }

                    operator type() const {
                        return data;
                    }

                    friend bool operator==(const value_t& l, const value_t& r)
                    {
                        return l.data == r.data;
                    }
                    friend bool operator!=(const value_t& l, const value_t& r)
                    {
                        return !(l == r);
                    }
                    friend QDataStream& operator<<(QDataStream &l, const value_t &r) {
                        return l << r.data;
                    }
                    friend QDataStream& operator>>(QDataStream &l, value_t &r)
                    {
                        l >> r.data;
                        return l;
                    }

                    /**
                     * @brief Minimum allowed value
                     */
                    static constexpr type MINIMUM = 0x00000000000000; // 7 Bytes

                    /**
                     * @brief Maximum allowed value
                     */
                    static constexpr type MAXIMUM = 0xFFFFFFFFFFFFFF; // 7 Bytes

                private:
                    type data;

                    /**
                     * @brief Bit mask for upper range of sequence field
                     */
                    static const type UPPER_MASK = 0xFFFFFF00000000;

                    /**
                     * @brief Bit shift for upper range of sequence field
                     */
                    static const quint8 UPPER_SHIFT = 4 * 8;

                    /**
                     * @brief Bit mask for lower range of sequence field
                     */
                    static const type LOWER_MASK = 0x000000FFFFFFFF;

                    /**
                     * @brief Bit shift for lower range of sequence field
                     */
                    static const quint8 LOWER_SHIFT = 0;

                };

                /**
                 * @brief Sequence Types
                 */
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

                /**
                 * @brief Get next value in Sequence
                 * @param cid Component IDentifer of source
                 * @param type Sequence type
                 * @return Next value in Sequence
                 */
                value_t next(const CID &cid, type_t type);

            private:
                /**
                 * @brief Map of a CIDs last sequence number
                 */
                typedef QMap<CID, Sequence::value_t> lastMap_t;
                QMap<type_t, lastMap_t> last;

                /**
                 * @brief bootCount Used for non-volatile sequence upper word
                 */
                unsigned int bootCount = 0;
        };
        Sequence sequence;
};

#endif // SECURESACN_H
