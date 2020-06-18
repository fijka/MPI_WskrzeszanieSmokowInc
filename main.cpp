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

int deskCount = 0;
int dragonCount = 0;
int ackDesk = 0;
int ackDragon = 0;
int lamport = 0;
int first, last;
int DESKS, DRAGONS;
int currentMission = 0;
int timeRequest =10000000;
int ackMission = 0;
packet_t recvPacket, myPacket;

pthread_mutex_t stateMut = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t requestMut = PTHREAD_MUTEX_INITIALIZER;

list::list() {
    first = 0;
}


list *allAck = new list;
list *missionsReq = new list;
list *desksReq = new list;
list *dragonsReq = new list;

void list::addPacket(packet_t packet)
{
    packet_t *newPacket = new packet_t;
    newPacket->id = packet.id;
    newPacket->mission = packet.mission;
    newPacket->timeLamport = packet.timeLamport;
    newPacket->timeRequest = packet.timeRequest;
    newPacket->data = packet.data;
    newPacket->next = 0;
    
    if (first == 0) {
        first = newPacket;
    } else {
        packet_t *tmp = first;
        packet_t *prev = first;

        do {
            if (newPacket->data < tmp->data) {
                if (tmp == first) {
                    newPacket->next = first;
                    first = newPacket;
                    break;
                } else {
                    prev->next = newPacket;
                    newPacket->next = tmp;
                    break;
                }
            } else if (newPacket->data == tmp->data) {
                if (newPacket->timeRequest < tmp->timeRequest) {
                    prev->next = newPacket;
                    newPacket->next = tmp;
                    break;
                } else if (newPacket->timeRequest == tmp->timeRequest) {
                    if (newPacket->id < tmp->id) {
                        prev->next = newPacket;
                        newPacket->next = tmp;
                        break;
                    }
                }
            }
            prev = tmp;
            tmp = tmp->next;
        } while(tmp->next);
	if(tmp ==0){
	  prev->next = newPacket;
	}
    }
}

void list::deletePacket(packet_t packet)
{
    packet_t *tmp = first;
    packet_t *prev = first;

    do {
        if (packet.id == tmp->id and packet.timeRequest == tmp->timeRequest) {
            if (tmp == first) {
                first = tmp->next;
                break;
            } else {
                prev->next = tmp->next;
                break;
            }
        }
        prev = tmp;
        tmp = tmp->next;
    } while(tmp->next);
}

void list::deleteFirstPacket()
{
    packet_t *tmp= first;
    if(first ->next){
      first= tmp->next;
    }else{
        ;
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
    pthread_mutex_destroy(&requestMut);
    /* Czekamy, aż wątek potomny się zakończy */
    printf("czekam na wątek \"komunikacyjny\"\n" );
    pthread_join(threadCom, NULL);
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
    MPI_Send(pkt, sizeof(packet_t), MPI_BYTE, destination, tag, MPI_COMM_WORLD);
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
