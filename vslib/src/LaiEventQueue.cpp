#include "Lai.h"
#include "LaiInternal.h"

#include "swss/logger.h"

using namespace laivs;

void Lai::startEventQueueThread()
{
    SWSS_LOG_ENTER();

    m_eventQueueThreadRun = true;

    m_eventQueueThread = std::make_shared<std::thread>(&Lai::eventQueueThreadProc, this);
}

void Lai::stopEventQueueThread()
{
    SWSS_LOG_ENTER();

    SWSS_LOG_NOTICE("begin");

    if (m_eventQueueThreadRun)
    {
        m_eventQueueThreadRun = false;

        m_eventQueue->enqueue(std::make_shared<Event>(EventType::EVENT_TYPE_END_THREAD, nullptr));

        m_eventQueueThread->join();
    }

    SWSS_LOG_NOTICE("end");
}

void Lai::eventQueueThreadProc()
{
    SWSS_LOG_ENTER();

    while (m_eventQueueThreadRun)
    {
        m_signal->wait();

        while (auto event = m_eventQueue->dequeue())
        {
            processQueueEvent(event);
        }
    }
}

void Lai::processQueueEvent(
        _In_ std::shared_ptr<Event> event)
{
    SWSS_LOG_ENTER();

    auto type = event->getType();

    switch (type)
    {
        case EVENT_TYPE_END_THREAD:

            SWSS_LOG_NOTICE("received EVENT_TYPE_END_THREAD, will process all messages and end");
            break;

        case EVENT_TYPE_NET_LINK_MSG:
            return syncProcessEventNetLinkMsg(std::dynamic_pointer_cast<EventPayloadNetLinkMsg>(event->getPayload()));

        case EVENT_TYPE_NOTIFICATION:
            return asyncProcessEventNotification(std::dynamic_pointer_cast<EventPayloadNotification>(event->getPayload()));

        default:

            SWSS_LOG_THROW("unhandled event type: %d", type);
            break;
    }
}

void Lai::syncProcessEventNetLinkMsg(
        _In_ std::shared_ptr<EventPayloadNetLinkMsg> payload)
{
    MUTEX();

    SWSS_LOG_ENTER();

    m_vsLai->syncProcessEventNetLinkMsg(payload);
}

void Lai::asyncProcessEventNotification(
        _In_ std::shared_ptr<EventPayloadNotification> payload)
{
    SWSS_LOG_ENTER();

    auto linecardNotifications = payload->getLinecardNotifications();

    auto ntf = payload->getNotification();

    ntf->executeCallback(linecardNotifications);
}
