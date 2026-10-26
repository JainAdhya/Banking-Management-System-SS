#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>

void connectionHandler(int socketFileDescriptor);
void hide_input(char *buffer, int size);

int main()
{
    int socketFileDescriptor;
    int connectStatus;
    struct sockaddr_in address;

    socketFileDescriptor = socket(AF_INET, SOCK_STREAM, 0);
    if (socketFileDescriptor == -1)
    {
        perror("Socket creation error\n");
        exit(-1);
    }

    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_family = AF_INET;
    address.sin_port = htons(8080);

    connectStatus = connect(socketFileDescriptor, (struct sockaddr *)&address, sizeof(address));
    if(connectStatus == -1)
    {
        perror("Connection error\n");
        exit(-1);
    }

    connectionHandler(socketFileDescriptor);
    exit(0);
}

void connectionHandler(int socketFileDescriptor)
{
    char readBuffer[4096], writeBuffer[4096], tempBuffer[4096];
    int readBytes, writeBytes;

    do
    {
        bzero(readBuffer, sizeof(readBuffer));
        readBytes = read(socketFileDescriptor, readBuffer, sizeof(readBuffer));
        if(readBytes <= 0)
        {
            break;
        }

        if (strcmp(readBuffer, "Enter password: ") == 0 || strcmp(readBuffer, "Enter new password: ") == 0)
        {
            hide_input(writeBuffer, sizeof(writeBuffer));
        }
        else
        {
            bzero(writeBuffer, sizeof(writeBuffer));
            bzero(tempBuffer, sizeof(tempBuffer));
            if(strcmp(readBuffer, "Client logging out...\n") == 0)
            {
                close(socketFileDescriptor);
                return;
            }
            else if(strchr(readBuffer, '^') != NULL)
            {
                if(strlen(readBuffer) != 1)
                {
                    strncpy(tempBuffer, readBuffer, strlen(readBuffer) - 1);
                    printf("%s\n", tempBuffer);
                }
                strcpy(writeBuffer, "");
            }
            else
            {
                printf("%s\n", readBuffer);
                scanf("%s", writeBuffer);
            }
        }

        writeBytes = write(socketFileDescriptor, writeBuffer, sizeof(writeBuffer));
        if(writeBytes == -1)
        {
            break;
        }
    } while(readBytes > 0);

    close(socketFileDescriptor);
}

void hide_input(char *buffer, int size) {
    struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    printf("Enter password: ");
    scanf("%s", buffer);

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    printf("\n");
}
