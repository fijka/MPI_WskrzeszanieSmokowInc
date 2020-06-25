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
                pthread_mutex_lock(&curMisMut);
                missions.push_back(recvPacket.mission);
                pthread_mutex_unlock(&curMisMut);
                break;

            // prośba o dostęp do zlecenia
            case MISSION_REQ:
	        if (recvPacket.mission != missions[currentMission]) {
                pthread_mutex_lock(&curMisMut);
                missions[recvPacket.mission] = -1;
                pthread_mutex_unlock(&curMisMut);
                sendedPacket.mission = recvPacket.mission;
                lamport += 1;
                sendedPacket.ts = lamport;
                sendPacket(&sendedPacket, status.MPI_SOURCE, MISSION_ACK);
            } else if (state != mission_wait and recvPacket.mission != missions[currentMission]) {
                pthread_mutex_lock(&curMisMut);
                missions[recvPacket.mission] = -1;
                pthread_mutex_unlock(&curMisMut);
                sendedPacket.mission = recvPacket.mission;
                lamport += 1;
                sendedPacket.ts = lamport;
                sendPacket(&sendedPacket, status.MPI_SOURCE, MISSION_ACK);
            } else if (recvPacket.data < dragonCount) {
                pthread_mutex_lock(&curMisMut);
                missions[recvPacket.mission] = -1;
                pthread_mutex_unlock(&curMisMut);
                sendedPacket.mission = recvPacket.mission;
                lamport += 1;
                sendedPacket.ts = lamport;
                sendPacket(&sendedPacket, status.MPI_SOURCE, MISSION_ACK);
            } else if (recvPacket.time < requestTime and recvPacket.data ==  dragonCount) {
                pthread_mutex_lock(&curMisMut);
                missions[recvPacket.mission] = -1;
                pthread_mutex_unlock(&curMisMut);
                sendedPacket.mission = recvPacket.mission;
                lamport += 1;
                sendedPacket.ts = lamport;
                sendPacket(&sendedPacket, status.MPI_SOURCE, MISSION_ACK);
            } else if (recvPacket.data == dragonCount and recvPacket.time == requestTime and rank > status.MPI_SOURCE) {
                pthread_mutex_lock(&curMisMut);
                missions[recvPacket.mission] = -1;
                pthread_mutex_unlock(&curMisMut);
                sendedPacket.mission = recvPacket.mission;
                lamport += 1;
                sendedPacket.ts = lamport;
                sendPacket(&sendedPacket, status.MPI_SOURCE, MISSION_ACK);
                } else {
                    //reqTab[status.MPI_SOURCE] = recvPacket;
                }
                break;

            // zgoda na otrzymanie zlecenia
            case MISSION_ACK:
		        if (state == mission_wait and recvPacket.mission == missions[currentMission]) {
                    pthread_mutex_lock(&ackMut);
                    ackMission += 1;
                    if (ackMission == last - first) {
                        changeState(mission_have);
			            ackMission = 0;
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
			            or (state == desk_wait and deskCount > recvPacket.data)
                        or (state == desk_wait and requestTime > recvPacket.time and deskCount == recvPacket.data)
                        or (state == desk_wait and deskCount == recvPacket.data and recvPacket.time == requestTime and rank > status.MPI_SOURCE)) {
		            lamport += 1;
		            myPacket.ts = lamport;
                    sendPacket(&myPacket, status.MPI_SOURCE, DESK_ACK);
                }else{
                    reqTab[status.MPI_SOURCE] = recvPacket;
                }
                break;

            // zgoda na dostęp do biurka
            case DESK_ACK:
                if (state == desk_wait) {
                    ackDesk += 1;
                    if (ackDesk >= size - 1 - DESKS - 2) {
                        deskCount += 1;
                        changeState(desk_have);
                        ackDesk = 0;
                    }
                }
                break;

            // prośba o dostęp do szkieletu
            case DRAGON_REQ:
                if ((state != dragon_have and state != dragon_wait)
                        or (state == dragon_wait and dragonCount > recvPacket.data)
                        or (state == dragon_wait and recvPacket.data == dragonCount and recvPacket.time < requestTime)
                        or (state == dragon_wait and dragonCount ==recvPacket.data and recvPacket.time == requestTime and rank > status.MPI_SOURCE)) {
		            lamport += 1;
		            myPacket.ts = lamport;
                    sendPacket(&myPacket, status.MPI_SOURCE, DRAGON_ACK);
                } else {
                    dragTab[status.MPI_SOURCE] = recvPacket;
                }
                break;

            // zgoda na dostęp do szkieletu
            case DRAGON_ACK:
                if (state == dragon_wait) {
		  
                    ackDragon += 1; 
                    if (ackDragon >= size - 1 - DRAGONS - 2) {
                        
                        changeState(dragon_have);
                        ackDragon = 0;
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
                    for (int i = 1; i < size; i++) {
                        if (dragTab[i].mission != -1 and i != rank) {
                            lamport++; 
                            myPacket.ts = lamport;
                            sendPacket(&myPacket, i, DRAGON_ACK);
                        }
                    }
                }
                break;

            default:
                break;
        }
    }
}
