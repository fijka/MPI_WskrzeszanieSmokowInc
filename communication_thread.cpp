#include "main.h"
#include "communication_thread.h"

/* wątek komunikacyjny: odbiór i segregacja otrzymanych przez profesjonalistów wiadomości*/
void *startCommunicationThread(void *ptr)
{
    MPI_Status status;
    // myPacket.ts = lamport;
    int ready = 0;

    // PROFESJONALISTA: odbiór i segregacja otrzymanych wiadomości
    while (TRUE) {
        MPI_Recv(&recvPacket, 1, MPI_PACKET_T, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
	    //lamport_time(lamport, recvPacket.ts);
    	//myPacket.ts = lamport;
	
        switch (status.MPI_TAG) {

            // ---------- MISSION ----------

            // informacja o nowym zleceniu
            case MISSION_AD:
                missions.push_back(recvPacket.mission);
                break;
            
            // prośba o dostęp do zlecenia
            case MISSION_REQ:
                missionsReq[status.MPI_SOURCE - 1] = recvPacket;
                break;

            // zgoda na otrzymanie zlecenia
            case MISSION_ACK:
                allAck[status.MPI_SOURCE - 1] = recvPacket;
                break;

            // informacja o dostępie do zlecenia innego profesjonalisty
            case MISSION_HAVE:
                cooperators.push_back(status.MPI_SOURCE);
                coop_mis.push_back(recvPacket);
                if (status.MPI_SOURCE >= first and status.MPI_SOURCE <= last)
                    missions[recvPacket.mission] = -1; // powinno być currentMission!
                break;
            
            // ---------- DESK ----------

            // prośba o dostęp do biurka
            case DESK_REQ:
                desksReq[status.MPI_SOURCE - 1] = recvPacket;
                break;
            
            // zgoda na dostęp do biurka
            case DESK_ACK:
                allAck[status.MPI_SOURCE - 1] = recvPacket;
                break;

            // zwolnienie biurka
            case DESK_RELEASE:
                desksReq[status.MPI_SOURCE - 1] = {-1, -1, -1};
                break;
            
            // ---------- DRAGON ----------

            // prośba o dostęp do szkieletu
            case DRAGON_REQ:
                dragonsReq[status.MPI_SOURCE - 1] = recvPacket;
                break;

            // zgoda na dostęp do szkieletu
            case DRAGON_ACK:
                allAck[status.MPI_SOURCE - 1] = recvPacket;
                break;

            // zgoda na dostęp do szieletu dla współpracowników
            case DRAGON_KILL:
                changeState(dragon_have);
                break;
            
            // zakończenie wskrzeszania
            case DRAGON_READY:
                ready += 1;
                if (ready == 2) {
                    debug("    [%d] Smok wskrzeszony! Dobra robota!", recvPacket.mission);
                    ready = 0;
                    dragonCount += 1;
                    changeState(mission_wait);
                }
                break;

            // zwolnienie szkieletu
            case DRAGON_RELEASE:
                dragonsReq[status.MPI_SOURCE - 1] = {-1, -1, -1};
                break;

            default:
                break;
        }
    }
}
