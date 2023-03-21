#include "NotificationQueue.h"
#include "lairediscommon.h"

#define NOTIFICATION_QUEUE_DROP_COUNT_INDICATOR (1000)

using namespace syncd;

#define MUTEX std::lock_guard<std::mutex> _lock(m_mutex);

NotificationQueue::NotificationQueue(
        _In_ size_t queueLimit):
    m_queueSizeLimit(queueLimit),
    m_dropCount(0)
{
    SWSS_LOG_ENTER();

    // empty;
}

NotificationQueue::~NotificationQueue()
{
    SWSS_LOG_ENTER();

    // empty
}

bool NotificationQueue::enqueue(
        _In_ const swss::KeyOpFieldsValuesTuple& item)
{
    MUTEX;

    SWSS_LOG_ENTER();

    /*
     * If the queue exceeds the limit, then drop all further FDB events This is
     * a temporary solution to handle high memory usage by syncd and the
     * notification queue keeps growing. The permanent solution would be to
     * make this stateful so that only the *latest* event is published.
     */

    m_queue.push(item);

    return true;
}

bool NotificationQueue::tryDequeue(
        _Out_ swss::KeyOpFieldsValuesTuple& item)
{
    MUTEX;

    SWSS_LOG_ENTER();

    if (m_queue.empty())
    {
        return false;
    }

    item = m_queue.front();

    m_queue.pop();

    return true;
}

size_t NotificationQueue::getQueueSize()
{
    MUTEX;

    SWSS_LOG_ENTER();

    return m_queue.size();
}
