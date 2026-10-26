void managerMenu(int connection_fd);
int loginManager(int connection_fd, int mngID, char *password);
void changeStatus(int connection_fd);
void assignLoanApplication(int connection_fd);
void readFeedBack(int connection_fd);
int changeMNGPassword(int connection_fd, int mngID);

void managerMenu(int connection_fd)
{
    int mngID, response = 0;
    char password[20];
    int choice;

start:
    bzero(write_buf, sizeof(write_buf));
    strcpy(write_buf, "\nEnter Manager ID: ");
    write(connection_fd, write_buf, sizeof(write_buf));

    bzero(readBuffer, sizeof(readBuffer));
    read(connection_fd, readBuffer, sizeof(readBuffer));
    mngID = atoi(readBuffer);

    bzero(write_buf, sizeof(write_buf));
    strcpy(write_buf, "Enter password: ");
    write(connection_fd, write_buf, sizeof(write_buf));

    bzero(readBuffer, sizeof(readBuffer));
    read(connection_fd, readBuffer, sizeof(readBuffer));
    strcpy(password, readBuffer);

    if(loginManager(connection_fd, mngID, password))
    {
        bzero(write_buf, sizeof(write_buf));
        bzero(readBuffer, sizeof(readBuffer));
        strcpy(write_buf, "Login Successfully\n^");
        write(connection_fd, write_buf, sizeof(write_buf));
        read(connection_fd, readBuffer, sizeof(readBuffer));

        while(1)
        {
            bzero(write_buf, sizeof(write_buf));
            strcpy(write_buf, MNGMENU);
            write(connection_fd, write_buf, sizeof(write_buf));

            bzero(readBuffer, sizeof(readBuffer));
            read(connection_fd, readBuffer, sizeof(readBuffer));
            choice = atoi(readBuffer);

            switch(choice)
            {
                case 1:
                    changeStatus(connection_fd);
                    break;
                case 2:
                    assignLoanApplication(connection_fd);
                    break;
                case 3:
                    readFeedBack(connection_fd);
                    break;
                case 4:
                    response = changeMNGPassword(connection_fd, mngID);
                    if(!response)
                    {
                        bzero(write_buf, sizeof(write_buf));
                        strcpy(write_buf, "Unable to change password\n^");
                        write(connection_fd, write_buf, sizeof(write_buf));
                        read(connection_fd, readBuffer, sizeof(readBuffer));
                    }
                    else
                    {
                        bzero(write_buf, sizeof(write_buf));
                        strcpy(write_buf,"Password changed successfully\n^");
                        write(connection_fd, write_buf, sizeof(write_buf));
                        read(connection_fd, readBuffer, sizeof(readBuffer));
                    }
                    logout(connection_fd, mngID);                           
                    goto start;
                case 5:
                    logout(connection_fd, mngID);
                    return;
                case 6:
                    exitClient(connection_fd, mngID);
                    return;
                default:
                    bzero(readBuffer, sizeof(readBuffer));
                    write(connection_fd, "Invalid Choice for Manager menu^", sizeof("Invalid Choice for Manager menu^"));
                    read(connection_fd, readBuffer, sizeof(readBuffer));    
            }
        }
    }
    else
    {
        bzero(write_buf, sizeof(write_buf));
        bzero(readBuffer, sizeof(readBuffer));
        strcpy(write_buf, "\nInvalid ID or Password^");
        write(connection_fd, write_buf, sizeof(write_buf));
        read(connection_fd, readBuffer, sizeof(readBuffer));
        goto start;
    }
}

// Validate credentials BEFORE acquiring session semaphore
int loginManager(int connection_fd, int mngID, char *password)
{
    struct Employee mng;
    int file = open(EMPPATH, O_RDWR | O_CREAT, 0644);
    if (file == -1) return 0;

    int valid = 0;
    lseek(file, 0, SEEK_SET);
    while(read(file, &mng, sizeof(mng)) != 0)
    {
        if (mng.empID == mngID && strcmp(mng.password, crypt(password, HASHKEY)) == 0 && mng.role == 0) {
            valid = 1;
            break;
        }
    }
    close(file);

    if(!valid) return 0;

    sema = initializeSemaphore(mngID);
    setupSignalHandlers();

    if (sem_trywait(sema) == -1) {
        if (errno == EAGAIN) {
            printf("Manager %d already logged in.\n", mngID);
        }
        return 0;
    }
    return 1;
}

void changeStatus(int connection_fd)
{
    int file = open(CUSPATH, O_CREAT | O_RDWR, 0644);
    if(file == -1) return;
    struct Customer cs;

    bzero(write_buf, sizeof(write_buf));
    strcpy(write_buf, "Enter Account Number: ");
    write(connection_fd, write_buf, sizeof(write_buf));

    bzero(readBuffer, sizeof(readBuffer));
    read(connection_fd, readBuffer, sizeof(readBuffer));
    int accNo = atoi(readBuffer);

    int found = 0, offset = -1;
    lseek(file, 0, SEEK_SET);
    while(read(file, &cs, sizeof(cs)) != 0)
    {
        if(cs.accountNumber == accNo)
        {
            offset = lseek(file, -sizeof(struct Customer), SEEK_CUR);
            found = 1;
            break;
        }
    }

    if(!found)
    {
        close(file);
        bzero(write_buf, sizeof(write_buf));
        strcpy(write_buf, "Invalid account number^");
        write(connection_fd, write_buf, sizeof(write_buf));
        read(connection_fd, readBuffer, sizeof(readBuffer));
        return;
    }

    struct flock fl = {F_WRLCK, SEEK_SET, offset, sizeof(struct Customer), getpid()};
    fcntl(file, F_SETLKW, &fl);

    bzero(write_buf, sizeof(write_buf));
    strcpy(write_buf, "Enter 1 to Deactivate\nEnter 2 to Activate\nEnter your choice: ");
    write(connection_fd, write_buf, sizeof(write_buf));

    bzero(readBuffer, sizeof(readBuffer));
    read(connection_fd, readBuffer, sizeof(readBuffer));
    int choice = atoi(readBuffer);

    lseek(file, offset, SEEK_SET);
    read(file, &cs, sizeof(cs));
    if(choice == 1) cs.activeStatus = 0;
    else if(choice == 2) cs.activeStatus = 1;

    lseek(file, offset, SEEK_SET);
    write(file, &cs, sizeof(cs));

    fl.l_type = F_UNLCK;
    fcntl(file, F_SETLK, &fl);
    close(file);

    bzero(write_buf, sizeof(write_buf));
    strcpy(write_buf, "Status Changed Successfully^");
    write(connection_fd, write_buf, sizeof(write_buf));
    read(connection_fd, readBuffer, sizeof(readBuffer));
}

void readFeedBack(int connection_fd)
{
    struct FeedBack fb;
    char tempBuffer[4096];

    int file = open(FEEDPATH, O_RDONLY, 0644);
    if(file == -1) return;

    bzero(tempBuffer, sizeof(tempBuffer));
    while(read(file, &fb, sizeof(fb)) != 0)
    {
        strcat(tempBuffer, fb.feedback);
        strcat(tempBuffer, "\n");
    }
    close(file);

    bzero(write_buf, sizeof(write_buf));
    if(strlen(tempBuffer) == 0) {
        strcpy(write_buf, "No feedback available^");
    } else {
        strcat(tempBuffer, "^");
        strcpy(write_buf, tempBuffer);
    }
    write(connection_fd, write_buf, sizeof(write_buf));
    read(connection_fd, readBuffer, sizeof(readBuffer));
}

void assignLoanApplication(int connection_fd)
{
    struct LoanDetails ld;
    int file = open(LOANPATH, O_CREAT | O_RDWR, 0644);
    if(file == -1) return;

    char tempBuffer[4096];
    bzero(tempBuffer, sizeof(tempBuffer));

    while(read(file, &ld, sizeof(ld)) != 0)
    {
        if(ld.empID == -1)
        {
            char entry[256];
            sprintf(entry, "Loan ID: %d | Acc No: %d | Amount: %d\n", ld.loanID, ld.accountNumber, ld.loanAmount);
            strcat(tempBuffer, entry);
        }
    }

    if(strlen(tempBuffer) == 0) {
        close(file);
        bzero(write_buf, sizeof(write_buf));
        strcpy(write_buf, "No unassigned loan applications^");
        write(connection_fd, write_buf, sizeof(write_buf));
        read(connection_fd, readBuffer, sizeof(readBuffer));
        return;
    }

    strcat(tempBuffer, "Enter Loan ID to assign: ");
    write(connection_fd, tempBuffer, sizeof(tempBuffer));

    bzero(readBuffer, sizeof(readBuffer));
    read(connection_fd, readBuffer, sizeof(readBuffer));
    int lID = atoi(readBuffer);

    lseek(file, 0, SEEK_SET);
    int srcOffset = -1, sourceFound = 0;
    while (read(file, &ld, sizeof(ld)) != 0)
    {
        if(ld.loanID == lID)
        {
            srcOffset = lseek(file, -sizeof(struct LoanDetails), SEEK_CUR);
            sourceFound = 1;
            break;
        }
    }

    if(!sourceFound)
    {
        close(file);
        bzero(write_buf, sizeof(write_buf));
        strcpy(write_buf, "Invalid loan ID^");
        write(connection_fd, write_buf, sizeof(write_buf));
        read(connection_fd, readBuffer, sizeof(readBuffer));
        return;
    }

    struct flock fl1 = {F_WRLCK, SEEK_SET, srcOffset, sizeof(struct LoanDetails), getpid()};
    fcntl(file, F_SETLKW, &fl1);

    bzero(write_buf, sizeof(write_buf));
    strcpy(write_buf, "Enter Employee ID: ");
    write(connection_fd, write_buf, sizeof(write_buf));

    bzero(readBuffer, sizeof(readBuffer));
    read(connection_fd, readBuffer, sizeof(readBuffer));
    int eID = atoi(readBuffer);

    lseek(file, srcOffset, SEEK_SET);
    read(file, &ld, sizeof(ld));
    ld.empID = eID;
    ld.status = 1; // Pending
    lseek(file, srcOffset, SEEK_SET);
    write(file, &ld, sizeof(ld));

    fl1.l_type = F_UNLCK;
    fcntl(file, F_SETLK, &fl1);
    close(file);

    bzero(write_buf, sizeof(write_buf));
    strcpy(write_buf, "Loan assigned successfully^");
    write(connection_fd, write_buf, sizeof(write_buf));
    read(connection_fd, readBuffer, sizeof(readBuffer));
}

int changeMNGPassword(int connection_fd, int mngID)
{
    char newPassword[20];
    struct Employee m;
    int file = open(EMPPATH, O_CREAT | O_RDWR, 0644);

    lseek(file, 0, SEEK_SET);
    int srcOffset = -1, sourceFound = 0;

    while (read(file, &m, sizeof(m)) != 0)
    {
        if(m.empID == mngID)
        {
            srcOffset = lseek(file, -sizeof(struct Employee), SEEK_CUR);
            sourceFound = 1;
            break;
        }
    }

    if(!sourceFound) {
        close(file);
        return 0;
    }

    struct flock fl1 = {F_WRLCK, SEEK_SET, srcOffset, sizeof(struct Employee), getpid()};
    fcntl(file, F_SETLKW, &fl1);

    bzero(write_buf, sizeof(write_buf));
    strcpy(write_buf, "Enter new password: ");
    write(connection_fd, write_buf, sizeof(write_buf));

    bzero(readBuffer, sizeof(readBuffer));
    read(connection_fd, readBuffer, sizeof(readBuffer));
    strcpy(newPassword, readBuffer);

    strcpy(m.password, crypt(newPassword, HASHKEY));
    lseek(file, srcOffset, SEEK_SET);
    write(file, &m, sizeof(m));

    fl1.l_type = F_UNLCK;
    fcntl(file, F_SETLK, &fl1);
    close(file);
    return 1;
}
