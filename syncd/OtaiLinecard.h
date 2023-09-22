#pragma once

extern "C" {
#include "otai.h"
}

#include "OtaiInterface.h"
#include "VirtualOidTranslator.h"
#include "RedisClient.h"
#include "OtaiLinecardInterface.h"

#include <set>
#include <string>
#include <unordered_map>
#include <vector>
#include <map>
#include <memory>

namespace syncd
{
    class OtaiLinecard:
        public OtaiLinecardInterface
    {
        private:

            OtaiLinecard(const OtaiLinecard&);
            OtaiLinecard& operator=(const OtaiLinecard&);

        public:

            OtaiLinecard(
                    _In_ otai_object_id_t linecard_vid,
                    _In_ otai_object_id_t linecard_rid,
                    _In_ std::shared_ptr<RedisClient> client,
                    _In_ std::shared_ptr<VirtualOidTranslator> translator,
                    _In_ std::shared_ptr<otairedis::OtaiInterface> vendorOtai,
                    _In_ bool warmBoot = false);

            virtual ~OtaiLinecard() = default;

        public:

            virtual std::unordered_map<otai_object_id_t, otai_object_id_t> getVidToRidMap() const override;

            virtual std::unordered_map<otai_object_id_t, otai_object_id_t> getRidToVidMap() const override;

            /**
             * @brief Indicates whether RID was discovered on linecard init.
             *
             * During linecard operation some RIDs are removable, like vlan member.
             * If user will remove such RID, then this function will no longer
             * return true for that RID.
             *
             * If in WARM boot mode this function will also return true for objects
             * that were user created and present on the linecard during init.
             *
             * @param rid Real ID to be examined.
             *
             * @return True if RID was discovered during init.
             */
            virtual bool isDiscoveredRid(
                    _In_ otai_object_id_t rid) const override;

            /**
             * @brief Indicates whether RID was discovered on linecard init at cold boot.
             *
             * During linecard operation some RIDs are removable, like vlan member.
             * If user will remove such RID, then this function will no longer
             * return true for that RID.
             *
             * @param rid Real ID to be examined.
             *
             * @return True if RID was discovered during cold boot init.
             */
            virtual bool isColdBootDiscoveredRid(
                    _In_ otai_object_id_t rid) const override;

            /**
             * @brief Indicates whether RID is one of default linecard objects
             * like CPU port, default virtual router etc.
             *
             * @param rid Real object id to examine.
             *
             * @return True if object is default linecard object.
             */
            virtual bool isLinecardObjectDefaultRid(
                    _In_ otai_object_id_t rid) const override;

            /**
             * @brief Indicates whether object can't be removed.
             *
             * Checks whether object can be removed. All non discovered objects can
             * be removed. All objects from internal attribute can't be removed.
             *
             * Currently there are some hard coded object types that can't be
             * removed like queues, ingress PG, ports. This may not be true for
             * some vendors.
             *
             * @param rid Real object ID to be examined.
             *
             * @return True if object can't be removed from linecard.
             */
            virtual bool isNonRemovableRid(
                    _In_ otai_object_id_t rid) const override;

            /*
             * Redis Static Methods.
             */

            /**
             * @brief Gets discovered objects on the linecard.
             *
             * This set can be different from discovered objects after linecard init
             * when for example default VLAN members will be removed.
             *
             * This set can't grow, but it can be reduced.
             *
             * Also if in WARM boot mode it can contain user created objects.
             *
             * @returns Discovered objects during linecard init.
             */
            virtual std::set<otai_object_id_t> getDiscoveredRids() const override;

            /**
             * @brief Remove existing object from the linecard.
             *
             * An ASIC remove operation is performed.
             * Function throws when object can't be removed.
             *
             * @param rid Real object ID.
             */
            virtual void removeExistingObject(
                    _In_ otai_object_id_t rid) override;

            /**
             * @brief Remove existing object reference only from discovery map.
             *
             * No ASIC operation is performed.
             * Function throws when object was not found.
             *
             * @param rid Real object ID.
             */
            virtual void removeExistingObjectReference(
                    _In_ otai_object_id_t rid) override;

            /**
             * @brief Gets default value of attribute for given object.
             *
             * This applies to objects discovered after linecard init like
             * OTAI_SCHEDULER_GROUP_ATTR_SCHEDULER_PROFILE_ID.
             *
             * If object or attribute is not found, OTAI_NULL_OBJECT_ID is returned.
             */
            virtual otai_object_id_t getDefaultValueForOidAttr(
                    _In_ otai_object_id_t rid,
                    _In_ otai_attr_id_t attr_id) override;

            /**
             * @brief Get cold boot discovered VIDs.
             *
             * @return Set of cold boot discovered VIDs after cold boot.
             */
            virtual std::set<otai_object_id_t> getColdBootDiscoveredVids() const override;

            /**
             * @brief Get warm boot discovered VIDs.
             *
             * @return Set of warm boot discovered VIDs after warm boot.
             */
            virtual std::set<otai_object_id_t> getWarmBootDiscoveredVids() const override;

        private:

            void redisSetDummyAsicStateForRealObjectId(
                    _In_ otai_object_id_t rid) const;

            /**
             * @brief Put cold boot discovered VIDs to redis DB.
             *
             * This method will only be called after cold boot and it will save
             * only VIDs that are present on the linecard after linecard is initialized
             * so it will contain only discovered objects. In case of warm boot
             * this method will not be called.
             */
            void redisSaveColdBootDiscoveredVids() const;

            otai_object_id_t helperGetLinecardAttrOid(
                    _In_ otai_attr_id_t attr_id);

            /**
             * @brief Discover helper.
             *
             * Method will call otaiDiscovery and collect all discovered objects.
             */
            void helperDiscover();

            void helperSaveDiscoveredObjectsToRedis();

            void helperInternalOids();

            void redisSaveInternalOids(
                     _In_ otai_object_id_t rid) const;

            void helperLoadColdVids();

            void helperPopulateWarmBootVids();

            /*
             * Other Methods.
             */

            bool isWarmBoot() const;

            void checkWarmBootDiscoveredRids();

        private:

            /**
             * @brief Discovered objects.
             *
             * Set of object IDs discovered after calling otaiDiscovery method.
             * This set will contain all objects present on the linecard right after
             * linecard init.
             *
             * This set depending on the boot, can contain user created objects if
             * linecard was in WARM boot mode. This set can also change if user
             * decides to remove some objects like VLAN_MEMBER.
             */
            std::set<otai_object_id_t> m_discovered_rids;

            /**
             * @brief Default oid map.
             *
             * This map will contain default created objects and all their "oid"
             * attributes and it's default value. This will be needed for bringing
             * default values.
             *
             * TODO later on we need to make this for all attributes.
             *
             * Example:
             * OTAI_OBJECT_TYPE_SCHEDULER: oid:0x16
             *
             * OTAI_OBJECT_TYPE_SCHEDULER_GROUP: oid:0x17
             *     OTAI_SCHEDULER_GROUP_ATTR_SCHEDULER_PROFILE_ID: oid:0x16
             *
             * m_defaultOidMap[0x17][OTAI_SCHEDULER_GROUP_ATTR_SCHEDULER_PROFILE_ID] == 0x16
             */
            std::unordered_map<otai_object_id_t, std::unordered_map<otai_attr_id_t, otai_object_id_t>> m_defaultOidMap;

            std::shared_ptr<otairedis::OtaiInterface> m_vendorOtai;

            bool m_warmBoot;

            std::map<otai_object_id_t, std::set<otai_object_id_t>> m_portRelatedObjects;

            std::shared_ptr<VirtualOidTranslator> m_translator;

            std::shared_ptr<RedisClient> m_client;
    };
}
