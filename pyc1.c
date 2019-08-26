#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/un.h> /* UNIX Domain Socket Addresses */
#include <sys/socket.h>
#include <sys/mman.h>
#include <pthread.h>

#define SOCK_PATH "/tmp/ud_rnd"
#define SHM_NAME "/ud_rnd_shmem"
#define URANDOM_FILE "/dev/urandom"

/* Grid size in pixels. Each pixel is 3 bytes. */
#define SIZE_X 32
#define SIZE_Y 32
#define BUF_SIZE (SIZE_X * SIZE_Y * 3)

/* This mutex is used for locking and unlocking global variables
 * shared among threads */
static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;

int n_samples = 1; 

static void* rnd_read(void *);
static void* msg_read(void *);

int main(int argc, char** argv)
{

    pthread_t rnd_th, msg_th;
    pthread_attr_t attr;
    pthread_attr_init(&attr); // default thread attributes

    /* Start acquisition and shared memory writing */
    pthread_create(&rnd_th, &attr, rnd_read, NULL);

    /* Start datagram socket for n samples acquisition */
    pthread_create(&msg_th, &attr, msg_read, NULL);

    /* Keep blocking as long as the threads are ongoing */
    pthread_join(rnd_th, NULL);
    pthread_join(msg_th, NULL);

    shm_unlink(SHM_NAME);
}

static void* msg_read(void *n)
{
    
    struct info {
        int n_samples;
    } info;

    /* UNIX socket */
    int sockfd;
    struct sockaddr_un srvaddr, clnaddr;
    int nbytes, len, n_samples_temp;

    /* Open UNIX datagram socket */
    sockfd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (sockfd == -1) {
        printf("Error while opening socket. Exiting...\n");
        exit(1);
    }
    remove(SOCK_PATH);

    /* Clear socket addr structure, assign UNIX domain address and path */
    memset(&srvaddr, 0, sizeof(struct sockaddr_un));
    srvaddr.sun_family = AF_UNIX;
    strncpy(srvaddr.sun_path, SOCK_PATH, sizeof(srvaddr.sun_path) - 1);

    /* Bind socket to address */
    if (bind(sockfd, (struct sockaddr *) &srvaddr, sizeof(struct sockaddr_un)) == -1) {
        printf("Error while binding socket to address. Exiting...\n");
        exit(1);
    }

    /* Main loop */
    while (1) {
        len = sizeof(struct sockaddr_un);
        nbytes = recvfrom(sockfd, &info, sizeof(struct info), 0, (struct sockaddr *) &clnaddr, &len);
        pthread_mutex_lock(&mtx);
        n_samples = info.n_samples;
        /* 999 is the command to close the process */
        if (info.n_samples == 999) {
            printf("Exiting C process...\n");
            exit(0);
        }
        pthread_mutex_unlock(&mtx);
    }
}

static void* rnd_read(void *n)
{
    /* Random generator */
    int rndfd;
    
    /* Shared memory */
    int shmfd;
    uint8_t *shdata;

    /* Grid bytes */
    uint8_t grid_buf[BUF_SIZE];
    int summed_buf[BUF_SIZE];

    int i, j;

    /* Open random number generator */
    rndfd = open(URANDOM_FILE, O_RDONLY);
    if (rndfd == -1) {
        printf("Error while opening the random generator file. Exiting...\n");
        exit(1);
    }
    
    /* Unlink previously created shared memory, if any */
    shm_unlink(SHM_NAME);
    /* Initialize shared memory */
    shmfd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shmfd == -1) {
        printf("Error while opening shared memory file. Exiting...\n");
        exit(1);
    }
    ftruncate(shmfd, BUF_SIZE);
    /* Map shared memory object */
    shdata = (uint8_t *)mmap(NULL, BUF_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0);
    close(shmfd);

    /* Main loop */
    while (1) {
        memset(summed_buf, 0, BUF_SIZE);
        pthread_mutex_lock(&mtx);
        for (i = 0; i < n_samples; i++) {
            read(rndfd, grid_buf, BUF_SIZE);
            for (j = 0; j < BUF_SIZE; j++)
                summed_buf[j] += grid_buf[j];
        }
        pthread_mutex_unlock(&mtx);
        for (i = 0; i < BUF_SIZE; i++)
            summed_buf[i] /= n_samples;
        /* Re-convert to 8bit */
        for (i = 0; i < BUF_SIZE; i++)
            grid_buf[i] = (uint8_t)summed_buf[i];
        /* Copy bytes to shared memory */
        memcpy(shdata, grid_buf, BUF_SIZE);

        /* Sleep for 300 ms */
        usleep(300000);
    }
}
