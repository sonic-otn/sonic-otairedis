#pragma once

#include "Linecard.h"

#include <memory>
#include <map>

namespace laivs
{
    class LinecardContainer
    {
        public:

            LinecardContainer() = default;

            virtual ~LinecardContainer() = default;

        public:

            /**
             * @brief Insert linecard to container.
             *
             * Throws when linecard already exists in container.
             */
            void insert(
                    _In_ std::shared_ptr<Linecard> sw);

            /**
             * @brief Remove linecard from container.
             *
             * Throws when linecard is not present in container.
             */
            void removeLinecard(
                    _In_ lai_object_id_t linecardId);

            /**
             * @brief Remove linecard from container.
             *
             * Throws when linecard is not present in container.
             */
            void removeLinecard(
                    _In_ std::shared_ptr<Linecard> sw);

            /**
             * @brief Get linecard from the container.
             *
             * If linecard is not present in container returns NULL pointer.
             */
            std::shared_ptr<Linecard> getLinecard(
                    _In_ lai_object_id_t linecardId) const;

            /**
             * @brief Removes all linecards from container.
             */
            void clear();

            /**
             * @brief Check whether linecard is in the container.
             */
            bool contains(
                    _In_ lai_object_id_t linecardId) const;

        private:

            std::map<lai_object_id_t, std::shared_ptr<Linecard>> m_linecardMap;

    };
};
