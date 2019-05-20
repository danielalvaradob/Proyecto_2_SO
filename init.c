// reference: https://www.ibm.com/support/knowledgecenter/en/ssw_ibm_i_73/apiref/apiexusmem.htm

#include <stdio.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>

#define SEMKEYPATH "/dev/null"  /* Path used on ftok for semget key  */
#define SEMKEYID 1              /* Id used on ftok for semget key    */
#define SHMKEYPATH "/dev/null"  /* Path used on ftok for shmget key  */
#define SHMKEYID 1              /* Id used on ftok for shmget key    */

#define NUMSEMS 1               /* Num of sems in created sem set    */
#define SIZEOFSHMSEG 100        /* Size of the shared mem segment    */

#define NUMMSG 2                /* Server only doing two "receives"
                                   on shm segment                    */

int main(int argc, char *argv[])
{
    int rc, semid, shmid, i;
    key_t semkey, shmkey;
    void *shm_address;
    struct sembuf operations[2];
    struct shmid_ds shmid_struct;
    short  sarray[NUMSEMS];

    /* Generate an IPC key for the semaphore set and the shared      */
    /* memory segment.  Typically, an application specific path and  */
    /* id would be used to generate the IPC key.                     */
    semkey = ftok(SEMKEYPATH,SEMKEYID);
    if ( semkey == (key_t)-1 )
      {
        printf("main: ftok() for sem failed\n");
        return -1;
      }
    shmkey = ftok(SHMKEYPATH,SHMKEYID);
    if ( shmkey == (key_t)-1 )
      {
        printf("main: ftok() for shm failed\n");
        return -1;
      }

    /* Create a semaphore set using the IPC key.  The number of      */
    /* semaphores in the set is two.  If a semaphore set already     */
    /* exists for the key, return an error. The specified permissions*/
    /* give everyone read/write access to the semaphore set.         */
      
    semid = semget( semkey, NUMSEMS, 0666 | IPC_CREAT | IPC_EXCL );
    if ( semid == -1 )
      {
        printf("main 1: semget() failed\n");
        return -1;
      }

    /* Initialize the first semaphore in the set to 0 and the        */
    /* second semaphore in the set to 0.                             */
    /*                                                               */
    /* The first semaphore in the sem set means:                     */
    /*        '1' --  The shared memory segment is being used.       */
    /*        '0' --  The shared memory segment is freed.            */

    sarray[0] = 0;



    /* The '1' on this command is a no-op, because the SETALL command*/
    /* is used.                                                      */
    rc = semctl( semid, 1, SETALL, sarray);
    if(rc == -1) {
        printf("main: semctl() initialization failed\n");
        return -1;
    }

    /* Create a shared memory segment using the IPC key.  The        */
    /* size of the segment is a constant.  The specified permissions */
    /* give everyone read/write access to the shared memory segment. */
    /* If a shared memory segment already exists for this key,       */
    /* return an error.                                              */
    shmid = shmget(shmkey, SIZEOFSHMSEG, 0666 | IPC_CREAT | IPC_EXCL);
    if (shmid == -1) {
        printf("main2: shmget() failed\n");
        return -1;
    }

    /* Attach the shared memory segment to the server process.       */
    shm_address = shmat(shmid, NULL, 0);
    if ( shm_address== (char *) -1 ) {
        printf("main: shmat() failed\n");
        return -1;
    }
    printf("Ready for client jobs\n");

    
        /* Set the structure passed into the semop() to first wait   */
        /* for the second semval to equal 1, then decrement it to    */
        /* allow the next signal that the client writes to it.       */
        /* Set the first semaphore to equal -1, which means          */
        /* that the shared memory segment is free.                   */
    operations[0].sem_num = 0;
    operations[0].sem_op =  -1;
    operations[0].sem_flg = IPC_NOWAIT;

    rc = semop( semid, operations, 1 );
    if (rc == -1) {
        printf("main1: semop() failed\n");
        return -1;
    
    }
       

    operations[0].sem_num = 0;
                                    /* Operate on the first sem      */
    operations[0].sem_op =  0;
                                    /* Wait for the value to be=0    */
    operations[0].sem_flg = 0;
                                    /* Allow a wait to occur         */

    operations[1].sem_num = 0;
                                    /* Operate on the first sem      */
    operations[1].sem_op =  1;
                                    /* Increment the semval by one   */
    operations[1].sem_flg = 0;
                                    /* Allow a wait to occur         */

    rc = semop( semid, operations, 2 );
    if (rc == -1)
      {
        printf("main2: semop() failed\n");
        return -1;
     }

     /* Clean up the environment by removing the semid structure,     */
    /* detaching the shared memory segment, and then performing      */
    /* the delete on the shared memory segment ID.                   */

    rc = semctl( semid, 1, IPC_RMID );
    if (rc==-1)
      {
        printf("main: semctl() remove id failed\n");
        return -1;
      }

     /* Detach the shared memory segment from the current process.    */
    rc = shmdt(shm_address);
    if (rc==-1)
      {
        printf("main: shmdt() failed\n");
        return -1;
      }
    rc = shmctl(shmid, IPC_RMID, &shmid_struct);
    if (rc==-1)
      {
        printf("main: shmctl() failed\n");
        return -1;
      }



return 0;
}
