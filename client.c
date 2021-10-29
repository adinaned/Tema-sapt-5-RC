#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>

int main() {
    
     if (access("fifoToServer", F_OK) == -1) // daca canalul fifo nu exista atunci il creeaza prin mknod
    { 
        if (mknod("fifoToServer", S_IFIFO | 0666, 0) == -1) // verificam situatia in care apare vreo eroare la crearea fisierului
        { 
            perror("Nu se poate crea fisierul fifoToServer.");
            exit(EXIT_FAILURE);
        }
    }

    int fdToServer;

    if ((fdToServer = open("fifoToServer", O_WRONLY)) == -1) 
    {
        perror("Nu se poate deschide pentru scriere fisierul fifoToServer.");
        exit(EXIT_FAILURE);
    }

    if (access("fifoToClient", F_OK) == -1) // daca canalul fifo nu exista atunci il creeaza prin mknod
    { 
        if (mknod("fifoToClient", S_IFIFO | 0666, 0) == -1) // verificam situatia in care apare vreo eroare la crearea fisierului
        { 
            perror("Nu se poate crea fisierul fifoToClient.");
            exit(EXIT_FAILURE);
        }
    }

    int fdToClient;

    if ((fdToClient = open("fifoToClient", O_RDONLY)) == -1) 
    {
        perror("Nu se poate deschide pentru citire fisierul fifoToClient.");
        exit(EXIT_FAILURE);
    }

    char buffer[256];
    buffer[0] = '\0';

    int lungimeComanda = 0;

    while(1) { 
        
        lungimeComanda=read(0, buffer, 256);
        buffer[lungimeComanda-1] = '\0'; 
        
        if(strcmp(buffer, "quit") == 0)
        {
            break;
        }
         
        write(fdToServer, buffer, strlen(buffer));

        char bufferResponse[256];
        bufferResponse[0] = '\0';

        int lungime = 0;
        lungime = read(fdToClient, bufferResponse, 256);
        bufferResponse[lungime] = '\0';

        printf("%s\n", bufferResponse);        
    }

    // close(fd);
    // close(fd2);
}