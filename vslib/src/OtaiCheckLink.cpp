#include "Otai.h"
#include "OtaiInternal.h"

#include "otaivs.h"
#include "meta/otai_serialize.h"

#include "swss/logger.h"
#include "swss/select.h"

using namespace otaivs;

void Otai::startCheckLinkThread()
{
    SWSS_LOG_ENTER();

    m_checkLinkRun = true;

    m_checkLinkThread = std::make_shared<std::thread>(std::thread(&Otai::checkLinkThreadProc, this));
}

void Otai::stopCheckLinkThread()
{
    SWSS_LOG_ENTER();

    SWSS_LOG_NOTICE("begin");

    if (m_checkLinkRun)
    {
        m_checkLinkRun = false;

        m_checkLinkThread->join();
    }

    m_isLinkUp = false;

    SWSS_LOG_NOTICE("end");
}

#include <fstream>
#include <unistd.h>

extern int g_linecard_state_change;
extern otai_oper_status_t g_linecard_state;

extern int g_alarm_change;
extern bool g_alarm_occur;

extern int g_event_change;
extern bool g_event_occur;

void Otai::checkLinkThreadProc()
{
    SWSS_LOG_ENTER();

    SWSS_LOG_NOTICE("enter VS check link thread");

    while (m_checkLinkRun)
    {
        std::ifstream ifs("/etc/sonic/link.txt");
        std::string line;

        if (ifs.is_open()) {
            std::getline(ifs, line);
            if (line == "1" && m_isLinkUp == false) {
                m_isLinkUp = true;
                g_linecard_state_change = 1;
                g_linecard_state = OTAI_OPER_STATUS_ACTIVE;
            } else if (line == "0" && m_isLinkUp == true) {
                m_isLinkUp = false;
                g_linecard_state_change = 1;
                g_linecard_state = OTAI_OPER_STATUS_INACTIVE;
            }
        }
        ifs.close();

        ifs.open("/etc/sonic/alarm.txt");
        if (ifs.is_open()) {
            std::getline(ifs, line);
             if (line == "1" && m_isAlarm == false) {
                 m_isAlarm = true;
                 g_alarm_change = 1;
                 g_alarm_occur = true;
             } else if (line == "0" && m_isAlarm == true) {
                 m_isAlarm = false;
                 g_alarm_change = 1;
                 g_alarm_occur = false;
             }
        }
        ifs.close();

        ifs.open("/etc/sonic/event.txt");
        if (ifs.is_open()) {
            std::getline(ifs, line);
             if (line == "1" && m_isEvent == false) {
                 m_isEvent = true;
                 g_event_change = 1;
                 g_event_occur = true;
             } else if (line == "0" && m_isEvent == true) {
                 m_isEvent = false;
                 g_event_change = 1;
                 g_event_occur = false;
             }
        }
        ifs.close();

        //SWSS_LOG_NOTICE("m_isLinkUp=%d", m_isLinkUp);
        usleep(500000);
    }

    SWSS_LOG_NOTICE("exit VS check link thread");
}

