#include "main.h"
#include "main_thread.h"

/* wątek główny: praca zleceniodawcy oraz profesjonalistów */
void mainLoop()
{
    // ZLECENIODAWCA
    if (rank == 0) {
        while (TRUE) {
            // generowanie zleceń i informaowanie o nich

        }
    }
    // PROFESJONALISTA
    else {
        bool missionReqSent = false;
        bool missionHaveSent = false;
        bool deskReqSent = false;
        bool dragonReqSent = false;

        while (TRUE) {
            switch (state) {
                case mission_wait:
                    if (missions.size() >= currentMission + 1)
                        if (missions[currentMission] == 0)
                            currentMission += 1;
                            missionReqSent = false;
                        if (!missionReqSent and missions[currentMission]) {
                            for (int i = first; i < last + 1; i++) {
                                if (i != rank)
                                    myPacket.mission = currentMission;
                                    sendPacket(&myPacket, i, MISSION_REQ);
                            }
                            missionReqSent = true;
                    }
                    break;

                case mission_have:
                    if (!missionHaveSent) {
                        for (int i = 1; i < size + 1; i++) {
                            if (i != rank) {
                                myPacket.mission = currentMission;
                                missions[currentMission] = 0;
                                myPacket.data = deskCount;
                                sendPacket(&myPacket, i, MISSION_HAVE);
                            }
                        }
                        missionHaveSent = true;
                    }
                    break;
                
                case desk_wait:
                    missionHaveSent = false;
                    if (!deskReqSent)
                        for (int i = 1; i < size + 1; i ++)
                            if (rank != i)
                                sendPacket(&myPacket, i, DESK_REQ);
                    deskReqSent = true;
                    break;
                
                case desk_have:
                    deskReqSent = false;
                    // sleep
                    changeState(dragon_wait);
                    break;

                case cooperator_wait:
                    missionHaveSent = false;
                    break;
                
                case dragon_wait:
                    if (!dragonReqSent)
                        for (int i = 1; i < size + 1; i++)
                            if (rank != i)
                                sendPacket(&myPacket, i, DRAGON_REQ);
                    dragonReqSent = true;
                    break;

                case dragon_have:
                    // wysłanie do współpracowników DRAGON_KILL
                    // sleep
                    // wysłanie do współpracowników DRAGON_READY
                    break;

                default:
                    break;
            }

        }
    }
}
