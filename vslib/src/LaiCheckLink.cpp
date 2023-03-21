#include "Lai.h"
#include "LaiInternal.h"

#include "laivs.h"
#include "meta/lai_serialize.h"

#include "swss/logger.h"
#include "swss/select.h"

using namespace laivs;

void Lai::startCheckLinkThread()
{
    SWSS_LOG_ENTER();

    m_checkLinkRun = true;

    m_checkLinkThread = std::make_shared<std::thread>(std::thread(&Lai::checkLinkThreadProc, this));
}

void Lai::stopCheckLinkThread()
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
extern lai_oper_status_t g_linecard_state;

void Lai::checkLinkThreadProc()
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
                g_linecard_state = LAI_OPER_STATUS_ACTIVE;
            } else if (line == "0" && m_isLinkUp == true) {
                m_isLinkUp = false;
                g_linecard_state_change = 1;
                g_linecard_state = LAI_OPER_STATUS_INACTIVE;
            }
        }
        ifs.close();
 
        //SWSS_LOG_NOTICE("m_isLinkUp=%d", m_isLinkUp);
        usleep(500000);
    }

    SWSS_LOG_NOTICE("exit VS check link thread");
}

