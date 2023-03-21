#pragma once

extern "C" {
#include "lai.h"
#include "laimetadata.h"
}

#include <set>
#include <unordered_map>
#include <map>

namespace syncd
{
    class LaiLinecardInterface
    {
        private:

            LaiLinecardInterface(const LaiLinecardInterface&);
            LaiLinecardInterface& operator=(const LaiLinecardInterface&);

        public:

            LaiLinecardInterface(
                    _In_ lai_object_id_t linecardVid,
                    _In_ lai_object_id_t linecardRid);

            virtual ~LaiLinecardInterface() = default;

        public:

            lai_object_id_t getVid() const;
            lai_object_id_t getRid() const;

        public:

            virtual std::unordered_map<lai_object_id_t, lai_object_id_t> getVidToRidMap() const = 0;

            virtual std::unordered_map<lai_object_id_t, lai_object_id_t> getRidToVidMap() const = 0;

            virtual bool isDiscoveredRid(
                    _In_ lai_object_id_t rid) const = 0;

            virtual bool isColdBootDiscoveredRid(
                    _In_ lai_object_id_t rid) const = 0;

            virtual bool isLinecardObjectDefaultRid(
                    _In_ lai_object_id_t rid) const = 0;

            virtual bool isNonRemovableRid(
                    _In_ lai_object_id_t rid) const = 0;

            virtual std::set<lai_object_id_t> getDiscoveredRids() const = 0;

            /**
             * @brief Gets default object based on linecard attribute.
             *
             * NOTE: This method will throw exception if invalid attribute is
             * specified, since attribute queried by this method are explicitly
             * declared in LaiLinecard constructor.
             *
             * @param attr_id Linecard attribute to query.
             *
             * @return Valid RID or specified linecard attribute received from
             * linecard.  This value can be also LAI_NULL_OBJECT_ID if linecard don't
             * support this attribute.
             */
            virtual lai_object_id_t getLinecardDefaultAttrOid(
                    _In_ lai_attr_id_t attr_id) const;


            virtual void removeExistingObject(
                    _In_ lai_object_id_t rid) = 0;

            virtual void removeExistingObjectReference(
                    _In_ lai_object_id_t rid) = 0;

            virtual lai_object_id_t getDefaultValueForOidAttr(
                    _In_ lai_object_id_t rid,
                    _In_ lai_attr_id_t attr_id) = 0;

            virtual std::set<lai_object_id_t> getColdBootDiscoveredVids() const = 0;

            virtual std::set<lai_object_id_t> getWarmBootDiscoveredVids() const = 0;

            virtual std::set<lai_object_id_t> getWarmBootNewDiscoveredVids();

        protected:

            /**
             * @brief Linecard virtual ID assigned by syncd.
             */
            lai_object_id_t m_linecard_vid;

            /**
             * @brief Linecard real ID assigned by LAI SDK.
             */
            lai_object_id_t m_linecard_rid;

            /**
             * @brief Map of default RIDs retrieved from Linecard object.
             *
             * It will contain RIDs like CPU port, default virtual router, default
             * trap group. etc. Those objects here should be considered non
             * removable.
             */
            std::map<lai_attr_id_t,lai_object_id_t> m_default_rid_map;

            std::set<lai_object_id_t> m_coldBootDiscoveredVids;

            std::set<lai_object_id_t> m_warmBootDiscoveredVids;

            std::set<lai_object_id_t> m_warmBootNewDiscoveredVids;
    };
}
