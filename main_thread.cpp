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
		    float sleepTime = 100000L + (long)((1e6-1e5) * rand()/(RAND_MAX + 1.0));
         	//float sleepTime = 1000000L;
		    usleep(sleepTime);                
            info.mission = task_number;
            sleep(1);   // można wyrzucić na końcu
            for (int i = 1; i < size; i++) {
	      //debug("sended %d", i);
	      sendPacket(&info, i, MISSION_AD);
            }
            debug("Nowe zlecenie! Nowe zlecenie! [%d]", task_number);
            task_number++;
        }
    }

    // PROFESJONALISTA
    else {
	requestTime = 100000000;
        bool missionReqSent = false;
        bool missionHaveSent = false;
        bool deskReqSent = false;
        bool dragonReqSent = false;
        bool dragonHaveSent = false;
        bool dragonReadySent = false;
        float sleepTime1 = 100000L+(long)((1e6-1e5)*rand()/(RAND_MAX+1.0));
        float sleepTime2 = 100000L+(long)((1e6-1e5)*rand()/(RAND_MAX+1.0));
        int coop = 0;
        int c1, c2;
        int tmp = 0;
        bool deskBoy = false;

        while (TRUE) {
            switch (state) {
                case mission_wait:
                    dragonReadySent = false;
		    if ((int)missions.size() > currentMission) {
                        if (missions[currentMission] == -1) {
                            while ((int)missions.size() <= currentMission + 1) {}
                            currentMission += 1;
                            missionReqSent = false;
                        }
                        myPacket.mission = missions[currentMission];
                        if (!missionReqSent and myPacket.mission != -1) {
			  lamport += 1;
			  requestTime = lamport;
                            for (int i = first; i < last + 1; i++) {
                                if (i != rank) {
						lamport += 1;
		    		                myPacket.ts = lamport;
						myPacket.data = dragonCount;
						myPacket.time = requestTime;
                                    sendPacket(&myPacket, i, MISSION_REQ);
                                }
                            }
                            missionReqSent = true;
                            if (first == last) {
                                changeState(mission_have);
                            }
                        }
                    }
                    break;

                case mission_have:
                    if (!missionHaveSent) {
                        for (int i = 1; i < size; i++) {
                            if (i != rank) {
                                myPacket.mission = missions[currentMission];
                                myPacket.data = deskCount;
				                lamport += 1;
		    		            myPacket.ts = lamport;
                                sendPacket(&myPacket, i, MISSION_HAVE);
                            }
                        }
                        missionHaveSent = true;
                    }
                    while (coop != 2) {
                        if (coop_mis.size() >= tmp + 1) {
                            if (coop_mis[tmp].mission == missions[currentMission]) {
                                coop++;
                                if (coop == 1) {
                                    c1 = tmp;
                                }
                                else {
                                    c2 = tmp;
                                    debug("[zlecenie %d, czas %d] Biorę zlecenie z %d i %d", missions[currentMission], lamport, cooperators[c1], cooperators[c2]);
                                    if (deskCount < coop_mis[c1].data and deskCount < coop_mis[c2].data) {
                                        changeState(desk_wait);
                                    } else if (deskCount > coop_mis[c1].data or deskCount > coop_mis[c2].data) {
                                        changeState(cooperator_wait);
                                    } else if (deskCount == coop_mis[c1].data and deskCount == coop_mis[c2].data) {
                                        if (rank < cooperators[c1] and rank < cooperators[c2]) changeState(desk_wait);
                                        else changeState(cooperator_wait);
                                    } else if (deskCount == coop_mis[c1].data) {
                                        if (rank < cooperators[c1]) changeState(desk_wait);
                                        else changeState(cooperator_wait);
                                    } else if (deskCount == coop_mis[c2].data) {
                                        if (rank < cooperators[c2]) changeState(desk_wait);
                                        else changeState(cooperator_wait);
                                    }
                                }
                            }
                            tmp++;
                        }
                    }
                    
                    break;
                
                case desk_wait:
                    missionHaveSent = false;
                    tmp = 0;
                    deskBoy = true;
                    coop = 0;
                    if (!deskReqSent) {
                        for (int i = 1; i < size; i ++)
			            {
                            if (rank != i) {
                                lamport += 1;
                                myPacket.ts = lamport;
                                sendPacket(&myPacket, i, DESK_REQ);
                            }
			            }
                    }
                    deskReqSent = true;
                    break;
                
                case desk_have:
                    debug("[zlecenie %d czas %d] Jestem w gildii, czekajcie!", missions[currentMission], lamport);
                    deskReqSent = false;
                    // sleep
		            usleep(sleepTime1);                
                    changeState(dragon_wait);
                    break;

                case cooperator_wait:
                    missionHaveSent = false;
                    coop = 0;
                    tmp = 0;
                    break;
                
                case dragon_wait:
                    if (!dragonReqSent) {
                        for (int i = 1; i < size; i++)
			            {
                            if (rank != i)
				            lamport += 1;
		    		        myPacket.ts = lamport;
                            sendPacket(&myPacket, i, DRAGON_REQ);
			            }
                        dragonReqSent = true;
                    }
                    break;

                case dragon_have:
                    dragonReqSent = false;
                    if (!dragonHaveSent and deskBoy) {
                        dragonHaveSent = true;
                        debug("[zlecenie %d czas %d] Mam szkielet! Chodźcie!", missions[currentMission], lamport);
                        sendPacket(&myPacket, cooperators[c1], DRAGON_KILL);
                        sendPacket(&myPacket, cooperators[c2], DRAGON_KILL);
                    }
		            usleep(sleepTime2);
                    if (!dragonReadySent) {
                        dragonReadySent = true;
                        sendPacket(&myPacket, cooperators[c1], DRAGON_READY);
                        sendPacket(&myPacket, cooperators[c2], DRAGON_READY);
                        missions[currentMission] = -1;
                    }            
                    if (deskBoy) {
                        deskBoy = false;
                        dragonHaveSent = false;
                    }
                    break;

                default:
                    break;
            }

        }
    }
}
