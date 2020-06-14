#include "main.h"
#include "communication_thread.h"

/* wątek komunikacyjny: odbieranie wiadomości i reagowanie na nie */
void *startCommunicationThread(void *ptr)
{
    MPI_Status status;
    myPacket.ts = lamport;

    state = mission_wait;

    int ackMission = 0;
    int ackDesk = 0;
    int ackDragon = 0;
    int acceptedProf[2] = {0, 0};
    int ready = 0;

    packet_t coop1, coop2;

    // PROFESJONALISTA: reakcja na odebrane wiadomości
    while (TRUE) {
        debug("czekam na wiadomość");
        MPI_Recv(&recvPacket, 1, MPI_PACKET_T, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

        switch (status.MPI_TAG) {

            // informacja o nowym zleceniu
            case MISSION_AD:
                missions.push_back(1);
                break;

            // prośba o dostęp do zlecenia
            case MISSION_REQ:
                if (state != mission_wait or recvPacket.ts > lamport
                        or (recvPacket.ts == lamport and rank > status.MPI_SOURCE)) {
                    myPacket.mission = recvPacket.mission;
                    // POZOR! mogę przypadkiem być w stanie mission_wait i mieć wyższy priorytet,
                    // ale starać się o inne zlecenie - to też trzeba jeszcze sprawdzić
                    // i to by chyba trzeba wszędzie sprawdzać (?) - np. przy zliczaniu ack-ów (?)
                    sendPacket(&myPacket, status.MPI_SOURCE, MISSION_ACK);
                }
                break;

            // zgoda na otrzymanie zlecenia
            case MISSION_ACK:
                if (state = mission_wait) {
                    ackMission += 1;    
                    if (ackMission == last - first) {
                        ackMission = 0;
                        changeState(mission_have);
                    }
                }
                break;

            // informacja o dostępie do zlecenia innego profesjonalisty
            case MISSION_HAVE:
                if (status.MPI_SOURCE >= first and status.MPI_SOURCE <= last)
                    missions[recvPacket.mission] = 0;
                    // trzeba jeszcze wymyślić jak ich przechowywać i czy wszystkich
                    if (state == mission_have) {
                        // tutaj się porównuje tych trzech
                        // jeśli najmniej razy siedziało się przy biurku -> changeState(desk_wait)
                        // w przeciwnym razie -> changeState(cooperator_wait)
                    }
                break;

            // prośba o dostęp do biurka
            case DESK_REQ:
                if (state != desk_have or lamport < recvPacket.ts
                        or (recvPacket.ts == lamport and rank > status.MPI_SOURCE))
                    sendPacket(&myPacket, status.MPI_SOURCE, DESK_ACK);
                break;

            // zgoda na dostęp do biurka
            case DESK_ACK:
                if (state = desk_wait) {
                    ackDesk += 1;
                    if (ackDesk > size - DESKS) {
                        ackDesk = 0;
                        changeState(desk_have);
                        deskCount += 1;
                    }
                }
                break;

            // prośba o dostęp do szkieletu
            case DRAGON_REQ:
                if (state != dragon_have or lamport < recvPacket.ts or
                        (recvPacket.ts == lamport and rank > status.MPI_SOURCE)) {
                    sendPacket(&myPacket, status.MPI_SOURCE, DRAGON_ACK);
                }
                break;

            // zgoda na dostęp do szkieletu
            case DRAGON_ACK:
                if (state = dragon_wait) {
                    ackDragon += 1; 
                    if (ackDragon > size - 3 * DRAGONS) {
                        ackDragon = 0;
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
                    changeState(mission_wait);
                    ready = 0;
                    dragonCount += 1;
                }
                break;

            default:
                break;
        }
    }
}
