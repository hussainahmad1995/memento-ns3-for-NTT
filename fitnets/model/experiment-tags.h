/* Tags for tracking simulation info.
*/
#ifndef EXPERIMENT_TAGS_H
#define EXPERIMENT_TAGS_H

#include "ns3/core-module.h"
#include "ns3/network-module.h"

using namespace ns3;

// A timestamp tag that can be added to a packet.
class TimestampTag : public Tag
{
public:
    static TypeId GetTypeId(void)
    {
        static TypeId tid = TypeId("ns3::TimestampTag")
                                .SetParent<Tag>()
                                .AddConstructor<TimestampTag>()
                                .AddAttribute("Timestamp",
                                              "Timestamp to save in tag.",
                                              EmptyAttributeValue(),
                                              MakeTimeAccessor(&TimestampTag::timestamp),
                                              MakeTimeChecker());
        return tid;
    };
    TypeId GetInstanceTypeId(void) const { return GetTypeId(); };
    uint32_t GetSerializedSize(void) const { return sizeof(timestamp); };
    void Serialize(TagBuffer i) const
    {
        i.Write(reinterpret_cast<const uint8_t *>(&timestamp),
                sizeof(timestamp));
    };
    void Deserialize(TagBuffer i)
    {
        i.Read(reinterpret_cast<uint8_t *>(&timestamp), sizeof(timestamp));
    };
    void Print(std::ostream &os) const
    {
        os << "t=" << timestamp;
    };

    // these are our accessors to our tag structure
    void SetTime(Time time) { timestamp = time; };
    Time GetTime() { return timestamp; };

private:
    Time timestamp;
};

// A tag with simple integer value.
class IntTag : public Tag
{
public:
    static TypeId GetTypeId(void)
    {
        static TypeId tid = TypeId("ns3::IntTag")
                                .SetParent<Tag>()
                                .AddConstructor<IntTag>()
                                .AddAttribute("Int",
                                              "Int to save in tag.",
                                              EmptyAttributeValue(),
                                              MakeUintegerAccessor(&IntTag::value),
                                              MakeUintegerChecker<u_int32_t>());
        return tid;
    };
    TypeId GetInstanceTypeId(void) const { return GetTypeId(); };
    uint32_t GetSerializedSize(void) const { return sizeof(value); };
    void Serialize(TagBuffer i) const
    {
        i.Write(reinterpret_cast<const uint8_t *>(&value),
                sizeof(value));
    };
    void Deserialize(TagBuffer i)
    {
        i.Read(reinterpret_cast<uint8_t *>(&value), sizeof(value));
    };
    void Print(std::ostream &os) const
    {
        os << "i=" << value;
    };

    // these are our accessors to our tag structure
    void SetValue(u_int32_t newval) { value = newval; };
    u_int32_t GetValue() { return value; };

private:
    u_int32_t value;
};

#endif // EXPERIMENT_TAGS_H
