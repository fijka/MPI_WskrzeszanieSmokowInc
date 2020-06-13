#include "main.h"
#include "communication_thread.h"

/* wątek komunikacyjny: odbieranie wiadomości i reagowanie na nie */
void *startCommunicationThread(void *ptr)
{
    MPI_Status status;
    int is_message = FALSE;
    packet_t packet;

    // ZLECENIODAWCA: reakcja na odebrane wiadomości
    if (rank == 0):
    {
        while (TRUE) {
            debug("[%d] czekam na wiadomość", rank);
            MPI_Recv(&packet, 1, MPI_PACKAGE_T, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

            switch (status.MPI_TAG) {
                // ...
                default:
                    break;
            }
        }
    }
    // PROFESJONALISTA: reakcja na odebrane wiadomości
    else {
        state = mission_wait;
        while (TRUE) {
            debug("[%d] czekam na wiadomość", rank);
            MPI_Recv( &packet, 1, MPI_PACKAGE_T, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

            switch (status.MPI_TAG) {
                // ...
                default:
                    break;
            }
        }
    }
}
