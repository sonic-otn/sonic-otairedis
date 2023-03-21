#include "RedisClient.h"
#include "swss/dbconnector.h"
#include <string>
#include <set>
#include <iostream>
#include <memory>

int main()
{   
    auto dbAsic = std::make_shared<swss::DBConnector>("ASIC_DB", 0);
    auto dbFlexCounter = std::make_shared<swss::DBConnector>("FLEX_COUNTER_DB", 0);    
    auto client = std::make_shared<syncd::RedisClient>(dbAsic, dbFlexCounter);

    std::set<std::string> keys;

    for (const auto& key : client->getAsicStateKeys())
    {    
        keys.insert(key);
    }

    for (const auto& key : keys)
    {
        // Find all the attrid from ASIC DB, and use them to query ASIC

        auto hash = client->getAttributesFromAsicKey(key);

        std::map<std::string, std::string> kvs;

        for (auto& kv : hash)
        {
            kvs.insert(kv);
        }

        for (auto& kv : kvs)
        {
            const std::string& skey = kv.first;
            const std::string& svalue = kv.second;

            std::cout << key \
                 << " attr value " << skey \
                 << " with on " << svalue << std::endl;

        }
    }

    keys.clear();

    for (const auto& key : client->getFlexCounterKeys())
    {    
        keys.insert(key);
    }
    for (const auto& key : keys)
    {
        auto hash = client->getAttributesFromFlexCounterKey(key);

        std::map<std::string, std::string> kvs;

        for (auto& kv : hash)
        {
            kvs.insert(kv);
        }
        for (auto& kv : kvs)
        {
            const std::string& skey = kv.first;
            const std::string& svalue = kv.second;

            std::cout << key \
                 << " attr value " << skey \
                 << " with on " << svalue << std::endl;

        }
    }

    keys.clear();

    for (const auto& key : client->getFlexCounterGroupKeys())
    {    
        keys.insert(key);
    }
    for (const auto& key : keys)
    {
        auto hash = client->getAttributesFromFlexCounterGroupKey(key);

        std::map<std::string, std::string> kvs;

        for (auto& kv : hash)
        {
            kvs.insert(kv);
        }
        for (auto& kv : kvs)
        {
            const std::string& skey = kv.first;
            const std::string& svalue = kv.second;

            std::cout << key \
                 << " attr value " << skey \
                 << " with on " << svalue << std::endl;

        }
    }
    auto vidToRid = client->getVidToRidMap();
    std::map<lai_object_id_t, lai_object_id_t> v2r;
    for (auto& i : vidToRid)
    {
        v2r.insert(i);
    }
    for (auto& i : v2r)
    {
        std::cout << std::hex << i.first << " -> " << std::hex << i.second << std::endl;
    }

    auto ridToVid = client->getRidToVidMap();
    std::map<lai_object_id_t, lai_object_id_t> r2v;
    for (auto& i : ridToVid)
    {
        r2v.insert(i);
    }
    for (auto& i : r2v)
    {
        std::cout << std::hex << i.first << " -> " << std::hex << i.second << std::endl;
    }

    return 0;
}
