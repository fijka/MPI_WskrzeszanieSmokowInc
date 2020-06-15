#include "main.h"
#include "main_thread.h"

/* wątek główny: praca zleceniodawcy oraz profesjonalistów */
void mainLoop()
{
// ZLECENIODAWCA
    if (rank == 0) {
	int task_number = 0;
	packet_t info;
	info.ts = 0;
        while (TRUE) {
            // generowanie zleceń i informaowanie o nich
		//czekanie losowy czas na wygenerowanie zlecenia 
		float sleepTime = 100000L+(long)((1e6-1e5)*rand()/(RAND_MAX+1.0));
         	//float sleepTime = 1000000L;
		usleep(sleepTime);                
		info.mission = task_number;
 		for (int i = 1; i < size; i++){
			sendPacket(&info, i, 1);
		}
		task_number++;
        }
    }
       

    // PROFESJONALISTA
    else {
        bool missionReqSent = false;
        bool missionHaveSent = false;
        bool deskReqSent = false;
        bool dragonReqSent = false;
	float sleepTime1 = 100000L+(long)((1e6-1e5)*rand()/(RAND_MAX+1.0));
	float sleepTime2 = 100000L+(long)((1e6-1e5)*rand()/(RAND_MAX+1.0));

        while (TRUE) {
            switch (state) {
                case mission_wait:
                    if (missions.size() >= currentMission + 1) {
                        if (missions[currentMission] == 0)
                            currentMission += 1;
                            missionReqSent = false;
                        if (!missionReqSent and missions[currentMission]) {
                            for (int i = first; i < last + 1; i++) {
                                if (i != rank) {
                                    myPacket.mission = currentMission;
				    lamport += 1;
		    		    myPacket.ts = lamport;
                                    sendPacket(&myPacket, i, MISSION_REQ);
                                }
                            }
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
				lamport += 1;
		    		myPacket.ts = lamport;
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
			{
                            if (rank != i)
				lamport += 1;
		    		myPacket.ts = lamport;
                                sendPacket(&myPacket, i, DESK_REQ);
			}
                    deskReqSent = true;
                    break;
                
                case desk_have:
                    deskReqSent = false;
                    // sleep
		    usleep(sleepTime1);                
                    changeState(dragon_wait);
                    break;

                case cooperator_wait:
                    missionHaveSent = false;
                    break;
                
                case dragon_wait:
                    if (!dragonReqSent)
                        for (int i = 1; i < size + 1; i++)
			{
                            if (rank != i)
				lamport += 1;
		    		myPacket.ts = lamport;
                                sendPacket(&myPacket, i, DRAGON_REQ);
			}
                    dragonReqSent = true;
                    break;

                case dragon_have:
                    // wysłanie do współpracowników DRAGON_KILL
                    // sleep
		    usleep(sleepTime2);                
                    // wysłanie do współpracowników DRAGON_READY
                    break;

                default:
                    break;
            }

        }
    }
}
