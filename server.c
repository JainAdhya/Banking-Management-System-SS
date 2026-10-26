#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sem.h>
#include <netinet/ip.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <crypt.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <wait.h>
#include <semaphore.h>

#define EMPPATH "employees.txt"
#define CUSPATH "customers.txt"
#define LOANPATH "loanDetails.txt"
#define COUNTERPATH "loanCounter.txt"
#define HISTORYPATH "trans_hist.txt"
#define FEEDPATH "feedback.txt"
#define HASHKEY "$6$saltsalt$"

#define MAINMENU "\n--- Select Login Type ---\n1. Customer Login\n2. Employee Login\n3. Manager Login\n4. Admin Login\n5. Exit Program\nEnter option: "
#define CUSMENU "\n--- Customer Panel ---\n1. Deposit Money\n2. Withdraw Money\n3. Check Balance\n4. Apply for Loan\n5. Transfer Funds\n6. Change Account Password\n7. View Transaction History\n8. Provide Feedback\n9. Logout\n10. Exit\n11. View Applied Loans\nEnter option: "
#define ADMINMENU "\n--- Admin Console ---\n1. Register New Employee\n2. Update Customer/Employee Info\n3. Manage Roles and Permissions\n4. Add Customer Account\n5. Delete Customer Account\n6. Search Customer Account\n7. Logout\nEnter option: "
#define EMPMENU "\n--- Employee Dashboard ---\n1. Register New Customer\n2. Edit Customer Information\n3. Review/Approve Loan Requests\n4. View Assigned Loan Tasks\n5. Review Customer Transactions\n6. Change Password\n7. Logout\n8. Exit\nEnter option: "
#define MNGMENU "\n--- Manager Section ---\n1. Enable/Disable Customer Accounts\n2. Assign Loan Applications to Employees\n3. View Customer Feedback\n4. Update Your Password\n5. Logout\n6. Exit\nEnter option: "

sem_t *sema;
char semName[50];

void employeeMenu(int connection_fd);
void managerMenu(int connection_fd);
void adminMenu(int connection_fd);
void connectionHandler(int connection_fd);
void exitClient(int connection_fd, int id);
void cleanupSemaphore(int signum);
void setupSignalHandlers();
sem_t *initializeSemaphore(int accountNumber);

#include "bank_functions.h"
#include "customer_functions.h"
#include "admin_functions.h"
#include "employee_functions.h"
#include "manager_functions.h"

void exitClient(int connection_fd, int id){
    if(id != 0) {
        snprintf(semName, 50, "/sem_%d", id);
        sem_t *s = sem_open(semName, 0);
        if (s != SEM_FAILED) {
            sem_post(s);
            sem_close(s);
            sem_unlink(semName);
        }
    }

    bzero(write_buf, sizeof(write_buf));
    strcpy(write_buf, "Client logging out...\n");
    write(connection_fd, write_buf, sizeof(write_buf));
    read(connection_fd, readBuffer, sizeof(readBuffer));
}

void cleanupSemaphore(int signum) {
    if (sema != NULL) {
        sem_post(sema);
        sem_close(sema);
        sem_unlink(semName);
    }
    printf("Program interrupted. Semaphore cleaned.\n");
    _exit(signum);
}

sem_t *initializeSemaphore(int id) {
    snprintf(semName, 50, "/sem_%d", id);
    return sem_open(semName, O_CREAT, 0644, 1);
}

void setupSignalHandlers() {
    signal(SIGINT, cleanupSemaphore);
    signal(SIGTERM, cleanupSemaphore);
    signal(SIGSEGV, cleanupSemaphore);
    signal(SIGHUP, cleanupSemaphore);
    signal(SIGQUIT, cleanupSemaphore);
}

int main(){
    int socket_fd, connection_fd;
    int clientSize;
    struct sockaddr_in address, client;

    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(socket_fd == -1){
        perror("Socket creation failed");
        exit(-1);
    }

    int opt = 1;
    setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_family = AF_INET;
    address.sin_port = htons(8080);

    if (bind(socket_fd, (struct sockaddr *)&address, sizeof(address)) == -1){
        perror("Binding failed");
        exit(-1);
    }

    if (listen(socket_fd, 10) == -1){
        perror("Listen failed");
        exit(-1);
    }
    printf("Server listening for connections on port 8080...\n");

    while(1){
        clientSize = sizeof(client);
        connection_fd = accept(socket_fd, (struct sockaddr *) &client, &clientSize);

        if (connection_fd == -1)
            perror("Accept Failed!");
        else{
            if(fork() == 0){
                close(socket_fd);
                connectionHandler(connection_fd);
                exit(0);
            }
            close(connection_fd);
        }
    }
    close(socket_fd);
    return 0;
}

void connectionHandler(int connection_fd){
    char readBuffer[4096];
    int readBytes, writeBytes, choice;

    while(1){
        writeBytes = write(connection_fd, MAINMENU, sizeof(MAINMENU));
        if(writeBytes == -1) break;

        bzero(readBuffer, sizeof(readBuffer));
        readBytes = read(connection_fd, readBuffer, sizeof(readBuffer));
        if(readBytes <= 0) break;

        choice = atoi(readBuffer);
        switch (choice) {
            case 1:
                customerMenu(connection_fd);
                break;
            case 2:
                employeeMenu(connection_fd);
                break;
            case 3:
                managerMenu(connection_fd);
                break;
            case 4:
                adminMenu(connection_fd);
                break;
            case 5:
                exitClient(connection_fd, 0);
                return;
            default:
                write(connection_fd, "Invalid Choice for Login menu^", sizeof("Invalid Choice for Login menu^"));
                read(connection_fd, readBuffer, sizeof(readBuffer));
        }
    }
    close(connection_fd);
}
