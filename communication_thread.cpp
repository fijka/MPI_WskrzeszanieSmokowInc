#include "main.h"
#include "communication_thread.h"

/* wątek komunikacyjny: odbieranie wiadomości i reagowanie na nie */
void *startCommunicationThread(void *ptr)
{
    MPI_Status status;
    myPacket.ts = lamport;
    packet_t recvPacket, sendedPacket;

    state = mission_wait;

    int ackDesk = 0;
    int ackDragon = 0;
    int acceptedProf[2] = {0, 0};
    int ready = 0;

    packet_t coop1, coop2;

    // PROFESJONALISTA: reakcja na odebrane wiadomości
    while (TRUE) {
        MPI_Recv(&recvPacket, 1, MPI_PACKET_T, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
	    lamport_time(lamport, recvPacket.ts);
    	myPacket.ts = lamport;
	
        switch (status.MPI_TAG) {

            // informacja o nowym zleceniu
            case MISSION_AD:
                missions.push_back(recvPacket.mission);
                break;


            // prośba o dostęp do zlecenia
            case MISSION_REQ:
	        if (recvPacket.mission != missions[currentMission]) {
                sendedPacket.mission = recvPacket.mission;
                lamport += 1;
                sendedPacket.ts = lamport;
		//                debug("wysylam ack [%d] do %d, warunek 1 -----", sendedPacket.mission, status.MPI_SOURCE);
                sendPacket(&sendedPacket, status.MPI_SOURCE, MISSION_ACK);
            } else if (state != mission_wait and recvPacket.mission != missions[currentMission]) {
                sendedPacket.mission = recvPacket.mission;
                lamport += 1;
                sendedPacket.ts = lamport;
                //debug("wysylam ack [%d] do %d, warunek 2 -----", sendedPacket.mission, status.MPI_SOURCE);
                sendPacket(&sendedPacket, status.MPI_SOURCE, MISSION_ACK);
            } else if (recvPacket.data < dragonCount) {
                sendedPacket.mission = recvPacket.mission;
                lamport += 1;
                sendedPacket.ts = lamport;
		// debug("wysylam ack [%d] do %d, warunek 3 -----", sendedPacket.mission, status.MPI_SOURCE);
                sendPacket(&sendedPacket, status.MPI_SOURCE, MISSION_ACK);
            } else if (recvPacket.time < requestTime and recvPacket.data ==  dragonCount) {
                sendedPacket.mission = recvPacket.mission;
                lamport += 1;
                sendedPacket.ts = lamport;
                //debug("wysylam ack [%d] do %d, warunek 4 -----", sendedPacket.mission, status.MPI_SOURCE);
                sendPacket(&sendedPacket, status.MPI_SOURCE, MISSION_ACK);
            } else if (recvPacket.data == dragonCount and recvPacket.time == requestTime and rank > status.MPI_SOURCE) {
                sendedPacket.mission = recvPacket.mission;
                lamport += 1;
                sendedPacket.ts = lamport;
                //debug("wysylam ack [%d] do %d,  warunek 5 -----", sendedPacket.mission, status.MPI_SOURCE);
                sendPacket(&sendedPacket, status.MPI_SOURCE, MISSION_ACK);
                } else {
                    reqTab[status.MPI_SOURCE] = recvPacket;
                }
                break;

            // zgoda na otrzymanie zlecenia
            case MISSION_ACK:
                if (state == mission_wait and recvPacket.mission == missions[currentMission]) {
                    pthread_mutex_lock(&ackMut);
                    ackMission += 1;
		    //  debug("[%d] ack od %d", recvPacket.mission, status.MPI_SOURCE)
                    if (ackMission == last - first) {
                        changeState(mission_have);
                    }
                    pthread_mutex_unlock(&ackMut);
                }
                break;

            // informacja o dostępie do zlecenia innego profesjonalisty
            case MISSION_HAVE:
                cooperators.push_back(status.MPI_SOURCE);
                coop_mis.push_back(recvPacket);
                break;

            // prośba o dostęp do biurka
            case DESK_REQ:
                if ((state != desk_have and state != desk_wait)
			            or (state == desk_wait and deskCount < recvPacket.data)
                        or (state == desk_wait and lamport < recvPacket.ts and deskCount < recvPacket.data)
                        or (state == desk_wait and deskCount < recvPacket.data and recvPacket.ts == lamport and rank > status.MPI_SOURCE)) {
		            lamport += 1;
		            myPacket.ts = lamport;
                    sendPacket(&myPacket, status.MPI_SOURCE, DESK_ACK);
                }
                break;

            // zgoda na dostęp do biurka
            case DESK_ACK:
                if (state = desk_wait) {
                    ackDesk += 1;
                    if (ackDesk >= size - 1 - DESKS) {
                        ackDesk = 0;
                        deskCount += 1;
                        changeState(desk_have);
                    }
                }
                break;

            // prośba o dostęp do szkieletu
            case DRAGON_REQ:
                if ((state != dragon_have and state != dragon_wait)
                        or (state == dragon_wait and lamport < recvPacket.ts)
                        or (state == dragon_wait and recvPacket.ts == lamport and rank > status.MPI_SOURCE)) {
		            lamport += 1;
		            myPacket.ts = lamport;
                    sendPacket(&myPacket, status.MPI_SOURCE, DRAGON_ACK);
                }
                break;

            // zgoda na dostęp do szkieletu
            case DRAGON_ACK:
                if (state = dragon_wait) {
                    ackDragon += 1; 
                    if (ackDragon > size - 1 - 3 * DRAGONS) {
                        ackDragon = 0;
                        dragonCount += 1;
                        changeState(dragon_have);
                    }
                }
                break;

            // zgoda na dostęp do szieletu dla współpracowników
            case DRAGON_KILL:
                changeState(dragon_have);
                break;

            // zakończenie wskrzeszania
            case DRAGON_READY:
                ready += 1;
                if (ready == 2) {
                    debug("      [zlecenie %d czas %d] Smok wskrzeszony! Dobra robota!", recvPacket.mission, lamport);
                    ready = 0;
                    dragonCount += 1;
                    changeState(mission_wait);
                }
                break;

            default:
                break;
        }
    }
}
