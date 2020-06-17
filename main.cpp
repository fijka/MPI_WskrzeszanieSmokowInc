#include "main.h"
#include "communication_thread.h"
#include "main_thread.h"
#include <pthread.h>

state_t state = mission_wait;
volatile char end = FALSE;
int size, rank;
MPI_Datatype MPI_PACKET_T;
pthread_t threadCom;

std::vector <int> missions, cooperators;
std::vector <struct packet_t> coop_mis;
packet_t missionsReq[HEAD + BODY + TAIL] = {{-1}};
packet_t desksReq[HEAD + BODY + TAIL] = {{0}};
packet_t dragonsReq[HEAD + BODY + TAIL] = {{0}};
packet_t allAck[HEAD + BODY + TAIL] = {{0}};
int deskCount = 0;
int dragonCount = 0;
int ackDesk = 0;
int ackDragon = 0;
int lamport = 0;
int first, last;
int DESKS, DRAGONS;
int currentMission = 0;
packet_t recvPacket, myPacket;

pthread_mutex_t stateMut = PTHREAD_MUTEX_INITIALIZER;

struct list {
    packet_t *first;
    void addPacket(packet_t packet);
    void deletePacket(packet_t packet);
    list();
};
list::list() {
    first = 0;
}

void list::addPacket(packet_t packet)
{
    packet_t *newPacket = new packet_t;
    newPacket->id = packet.id;
    newPacket->mission = packet.mission;
    newPacket->timeLamport = packet.timeLamport;
    newPacket->timeRequest = packet.timeRequest;
    newPacket->data = packet.data;
    newPacket->next = packet.next;
    
    if (first == 0) {
        first = newPacket;
    } else {
        packet_t *tmp = first;
        packet_t *prev = first;

        while(tmp->next) {
            if (newPacket->data < tmp->data) {
                if (tmp == first) {
                    newPacket->next = first;
                    first = newPacket;
                } else {
                    prev->next = newPacket;
                    newPacket->next = tmp;
                }
            } else if (newPacket->data == tmp->data) {
                if (newPacket->timeRequest < tmp->timeRequest) {
                    prev->next = newPacket;
                    newPacket->next = tmp;
                } else if (newPacket->timeRequest == tmp->timeRequest) {
                    if (newPacket->id < tmp->id) {
                        prev->next = newPacket;
                        newPacket->next = tmp;
                    }
                }
            }
            tmp = tmp->next;
            prev = tmp;
        }
    }
}


/* sprawdzenie działania wątków */
void check_thread_support(int provided)
{
    // printf("THREAD SUPPORT: chcemy %d. Co otrzymamy?\n", provided);
    switch (provided) {
        case MPI_THREAD_SINGLE: 
            printf("Brak wsparcia dla wątków, kończę\n");
	        fprintf(stderr, "Brak wystarczającego wsparcia dla wątków - wychodzę!\n");
            MPI_Finalize();
            exit(-1);
            break;
        case MPI_THREAD_FUNNELED: 
            printf("tylko te wątki, ktore wykonaly mpi_init_thread mogą wykonać wołania do biblioteki mpi\n");
	        break;
        case MPI_THREAD_SERIALIZED: 
            printf("tylko jeden watek naraz może wykonać wołania do biblioteki MPI\n");
	        break;
        case MPI_THREAD_MULTIPLE: //printf("Pełne wsparcie dla wątków\n");
	        break;
        default: printf("Nikt nic nie wie\n");
    }
}

/* sprawdzenie działania wątków oraz utworzenie typu MPI_PACKET_T */
void initialize(int *argc, char ***argv)
{
    int provided;
    MPI_Init_thread(argc, argv,MPI_THREAD_MULTIPLE, &provided);
    check_thread_support(provided);

    const int nitems = 6;
    int       blocklengths[6] = {1, 1, 1, 1, 1, 1};
    MPI_Datatype typy[6] = {MPI_INT, MPI_INT, MPI_INT, MPI_INT, MPI_INT, MPI_PACKET_T};

    MPI_Aint offsets[6]; 
    offsets[0] = offsetof(packet_t, id);
    offsets[1] = offsetof(packet_t, mission);
    offsets[2] = offsetof(packet_t, timeLamport);
    offsets[3] = offsetof(packet_t, timeRequest);
    offsets[4] = offsetof(packet_t, data);
    offsets[5] = offsetof(packet_t, next);

    MPI_Type_create_struct(nitems, blocklengths, offsets, typy, &MPI_PACKET_T);
    MPI_Type_commit(&MPI_PACKET_T);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    srand(rank);

    if (rank != 0) {

        // znalezienie procesów o tej samej profesji
        if (rank < HEAD + 1) {
            first = 1;
            last = HEAD;
        } else if (rank > HEAD && rank < HEAD + BODY + 1) {
            first = HEAD + 1;
            last = HEAD + BODY;
        } else {
            first = HEAD + BODY + 1;
            last = HEAD + BODY + TAIL;
        }

        pthread_create(&threadCom, NULL, startCommunicationThread, 0);
    }
    debug("Melduję się!");
}

/* zwolnienie zasobów oraz zakończenie pracy MPI */
void finalize()
{
    pthread_mutex_destroy(&stateMut);
    /* Czekamy, aż wątek potomny się zakończy */
    printf("czekam na wątek \"komunikacyjny\"\n" );
    pthread_join(threadCom, NULL);
    MPI_Type_free(&MPI_PACKET_T);
    MPI_Finalize();
}

/* wysłanie wiadomości do wskazanego odbiorcy */
void sendPacket(packet_t *pkt, int destination, int tag)
{
    int freepkt = 0;
    if (pkt == 0) {
        pkt = (packet_t*) malloc(sizeof(packet_t));
        freepkt = 1;
    }
    // pkt->src = rank; //??
    MPI_Send(pkt, 1, MPI_PACKET_T, destination, tag, MPI_COMM_WORLD);
    if (freepkt)
        free(pkt);
}

/* zmiana czasu lamporta */
void lamport_time(int time, int other)
{
	if (other > time){
		lamport = other + 1;
	} else {
		lamport = time + 1;
	}
}

/* zmiana stanu procesu */
void changeState(state_t newState)
{
    pthread_mutex_lock(&stateMut);
    state = newState;
    pthread_mutex_unlock(&stateMut);
}

int main(int argc, char **argv)
{
    DESKS = atoi(argv[1]);
    DRAGONS = atoi(argv[2]);
    initialize(&argc, &argv);
    mainLoop();

    finalize();
    return 0;
}
