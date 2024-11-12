#include "RequestShutdown.h"

#include "swss/logger.h"
#include "swss/notificationproducer.h"

#include <iostream>

using namespace syncd;

RequestShutdown::RequestShutdown()
{
    SWSS_LOG_ENTER();
}

RequestShutdown::~RequestShutdown()
{
    SWSS_LOG_ENTER();

    // empty
}

void RequestShutdown::send()
{
    SWSS_LOG_ENTER();

    swss::DBConnector db("ASIC_DB", 0);

    swss::NotificationProducer restartQuery(&db, SYNCD_NOTIFICATION_CHANNEL_RESTARTQUERY);

    std::vector<swss::FieldValueTuple> values;

    auto op = "COLD";

    SWSS_LOG_NOTICE("requested COLD shutdown");

    std::cerr << "requested " << op << " shutdown" << std::endl;

    restartQuery.send(op, op, values);
}
