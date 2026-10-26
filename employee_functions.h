void employeeMenu(int connection_fd);
int loginEmployee(int connection_fd, int empID, char *password);
void addCustomer(int connection_fd);
void approveRejectLoan(int connection_fd, int empID);
void viewAssignedLoan(int connection_fd, int empID);
int changeEMPPassword(int connection_fd, int empID);

void employeeMenu(int connection_fd)
{
    int empID;
    int accountNumber, response = 0;
    char password[20];
    int choice;

start:
    bzero(write_buf, sizeof(write_buf));
    strcpy(write_buf, "\nEnter Employee ID: ");
    write(connection_fd, write_buf, sizeof(write_buf));

    bzero(readBuffer, sizeof(readBuffer));
    read(connection_fd, readBuffer, sizeof(readBuffer));
    empID = atoi(readBuffer);

    bzero(write_buf, sizeof(write_buf));
    strcpy(write_buf, "Enter password: ");
    write(connection_fd, write_buf, sizeof(write_buf));

    bzero(readBuffer, sizeof(readBuffer));
    read(connection_fd, readBuffer, sizeof(readBuffer));
    strcpy(password, readBuffer);

    if(loginEmployee(connection_fd, empID, password))
    {
        bzero(write_buf, sizeof(write_buf));
        bzero(readBuffer, sizeof(readBuffer));
        strcpy(write_buf, "\nLogin Successfully^");
        write(connection_fd, write_buf, sizeof(write_buf));
        read(connection_fd, readBuffer, sizeof(readBuffer));

        while(1)
        {
            bzero(write_buf, sizeof(write_buf));
            strcpy(write_buf, EMPMENU);
            write(connection_fd, write_buf, sizeof(write_buf));

            bzero(readBuffer, sizeof(readBuffer));
            read(connection_fd, readBuffer, sizeof(readBuffer));
            choice = atoi(readBuffer);

            switch(choice)
            {
                case 1:
                    addCustomer(connection_fd);
                    break;
                case 2:
                    modifyCE(connection_fd, 1);
                    break;
                case 3:
                    approveRejectLoan(connection_fd, empID);
                    break;
                case 4:
                    viewAssignedLoan(connection_fd, empID);
                    break;
                case 5:
                    bzero(write_buf, sizeof(write_buf));
                    strcpy(write_buf, "Enter Account Number: ");
                    write(connection_fd, write_buf, sizeof(write_buf));

                    bzero(readBuffer, sizeof(readBuffer));
                    read(connection_fd, readBuffer, sizeof(readBuffer));
                    accountNumber = atoi(readBuffer);

                    transactionHistory(connection_fd, accountNumber);
                    break;
                case 6:
                    response = changeEMPPassword(connection_fd, empID);
                    if(!response){
                        bzero(write_buf, sizeof(write_buf));
                        strcpy(write_buf, "Unable to change password^");
                        write(connection_fd, write_buf, sizeof(write_buf));
                        read(connection_fd, readBuffer, sizeof(readBuffer));
                    }
                    else{
                        bzero(write_buf, sizeof(write_buf));
                        strcpy(write_buf, "Password changed successfully^");
                        write(connection_fd, write_buf, sizeof(write_buf));
                        read(connection_fd, readBuffer, sizeof(readBuffer));
                    }
                    logout(connection_fd, empID);                           
                    goto start;
                case 7:
                    logout(connection_fd, empID);
                    return;
                case 8:
                    exitClient(connection_fd, empID);
                    return;
                default:
                    bzero(readBuffer, sizeof(readBuffer));
                    write(connection_fd, "Invalid Choice for Employee menu^", sizeof("Invalid Choice for Employee menu^"));
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
int loginEmployee(int connection_fd, int empID, char *password)
{
    struct Employee employee;
    int file = open(EMPPATH, O_CREAT | O_RDWR, 0644);
    if (file == -1) return 0;

    int valid = 0;
    lseek(file, 0, SEEK_SET);
    while(read(file, &employee, sizeof(employee)) != 0){
        if (employee.empID == empID && strcmp(employee.password, crypt(password, HASHKEY)) == 0 && employee.role == 1) {
            valid = 1;
            break;
        }
    }
    close(file);

    if (!valid) return 0;

    sema = initializeSemaphore(empID);
    setupSignalHandlers();

    if (sem_trywait(sema) == -1) {
        if (errno == EAGAIN)
            printf("Employee %d is already logged in!\n", empID);
        return 0;
    }
    return 1;
}

void addCustomer(int connection_fd) {
    struct Customer customer;
    char transactionBuffer[1024];
    struct trans_histroy th;
    time_t s = time(NULL);
    struct tm* current_time = localtime(&s);

    int file = open(CUSPATH, O_RDWR | O_APPEND | O_CREAT, 0644);
    int fp = open(HISTORYPATH, O_RDWR | O_APPEND | O_CREAT, 0644);
    if (file == -1 || fp == -1) return;

    bzero(write_buf, sizeof(write_buf));
    strcpy(write_buf, "Enter Name: ");
    write(connection_fd, write_buf, sizeof(write_buf));
    bzero(readBuffer, sizeof(readBuffer));
    read(connection_fd, readBuffer, sizeof(readBuffer));
    strcpy(customer.customerName, readBuffer);

    bzero(write_buf, sizeof(write_buf));
    strcpy(write_buf, "Enter Password: ");
    write(connection_fd, write_buf, sizeof(write_buf));
    bzero(readBuffer, sizeof(readBuffer));
    read(connection_fd, readBuffer, sizeof(readBuffer));
    strcpy(customer.password, crypt(readBuffer, HASHKEY));

    bzero(write_buf, sizeof(write_buf));
    strcpy(write_buf, "Enter Account Number: ");
    write(connection_fd, write_buf, sizeof(write_buf));
    bzero(readBuffer, sizeof(readBuffer));
    read(connection_fd, readBuffer, sizeof(readBuffer));
    customer.accountNumber = atoi(readBuffer);

    bzero(write_buf, sizeof(write_buf));
    strcpy(write_buf, "Enter Opening Balance: ");
    write(connection_fd, write_buf, sizeof(write_buf));
    bzero(readBuffer, sizeof(readBuffer));
    read(connection_fd, readBuffer, sizeof(readBuffer));
    customer.balance = atof(readBuffer);
    customer.activeStatus = 1;

    sprintf(transactionBuffer, "%.2f Opening Balance %02d:%02d:%02d %d-%d-%d\n", customer.balance, current_time->tm_hour, current_time->tm_min, current_time->tm_sec, (current_time->tm_year)+1900, (current_time->tm_mon)+1, current_time->tm_mday);
    bzero(th.hist, sizeof(th.hist));
    strcpy(th.hist, transactionBuffer);
    th.acc_no = customer.accountNumber;
    write(fp, &th, sizeof(th));
    close(fp);

    write(file, &customer, sizeof(customer));
    close(file);

    bzero(write_buf, sizeof(write_buf));
    strcpy(write_buf, "Customer added successfully!^");
    write(connection_fd, write_buf, sizeof(write_buf));
    read(connection_fd, readBuffer, sizeof(readBuffer));
}

// Approve/Reject only loans assigned to this employee, preventing duplicates
void approveRejectLoan(int connection_fd, int empID)
{
    struct LoanDetails ld;
    struct trans_histroy th;
    time_t s = time(NULL);
    struct tm* current_time = localtime(&s);

    int file = open(LOANPATH, O_CREAT | O_RDWR, 0644);
    if(file == -1) return;

    bzero(write_buf, sizeof(write_buf));
    strcpy(write_buf, "Enter Loan ID: ");
    write(connection_fd, write_buf, sizeof(write_buf));

    bzero(readBuffer, sizeof(readBuffer));
    read(connection_fd, readBuffer, sizeof(readBuffer));
    int lID = atoi(readBuffer);

    lseek(file, 0, SEEK_SET);
    int srcOffset1 = -1, sourceFound1 = 0;
    while (read(file, &ld, sizeof(ld)) != 0)
    {
        if(ld.loanID == lID)
        {
            srcOffset1 = lseek(file, -sizeof(struct LoanDetails), SEEK_CUR);
            sourceFound1 = 1;
            break;
        }
    }

    if(!sourceFound1)
    {
        bzero(write_buf, sizeof(write_buf));
        strcpy(write_buf, "Invalid Loan ID^");
        write(connection_fd, write_buf, sizeof(write_buf));
        read(connection_fd, readBuffer, sizeof(readBuffer));
        close(file);
        return;
    }

    // Verify ownership and duplicate status decision
    if(ld.empID != empID)
    {
        bzero(write_buf, sizeof(write_buf));
        strcpy(write_buf, "This loan is not assigned to you!^");
        write(connection_fd, write_buf, sizeof(write_buf));
        read(connection_fd, readBuffer, sizeof(readBuffer));
        close(file);
        return;
    }

    if(ld.status == 2 || ld.status == 3)
    {
        bzero(write_buf, sizeof(write_buf));
        sprintf(write_buf, "Loan ID %d has already been processed (Approved/Rejected).^", lID);
        write(connection_fd, write_buf, sizeof(write_buf));
        read(connection_fd, readBuffer, sizeof(readBuffer));
        close(file);
        return;
    }

    struct flock fl1 = {F_WRLCK, SEEK_SET, srcOffset1, sizeof(struct LoanDetails), getpid()};
    fcntl(file, F_SETLKW, &fl1);

    bzero(write_buf, sizeof(write_buf));
    strcpy(write_buf, "Enter 1 to Approve Loan\nEnter 2 to Reject Loan: ");
    write(connection_fd, write_buf, sizeof(write_buf));

    bzero(readBuffer, sizeof(readBuffer));
    read(connection_fd, readBuffer, sizeof(readBuffer));
    int choice = atoi(readBuffer);

    int file2 = open(CUSPATH, O_CREAT | O_RDWR, 0644);
    struct Customer cs;
    int srcOffset2 = -1, sourceFound2 = 0;

    lseek(file2, 0, SEEK_SET);
    while (read(file2, &cs, sizeof(cs)) != 0)
    {
        if(cs.accountNumber == ld.accountNumber)
        {
            srcOffset2 = lseek(file2, -sizeof(struct Customer), SEEK_CUR);
            sourceFound2 = 1;
            break;
        }
    }

    struct flock fl2 = {F_WRLCK, SEEK_SET, srcOffset2, sizeof(struct Customer), getpid()};
    fcntl(file2, F_SETLKW, &fl2);

    char resultMsg[128];
    if(choice == 1)
    {
        if(cs.activeStatus == 0 || !sourceFound2)
        {
            ld.status = 3; // Rejected due to inactive account
            strcpy(resultMsg, "Loan rejected because customer account is inactive^");
        }
        else
        {
            cs.balance += ld.loanAmount;
            ld.status = 2; // Approved

            int fp = open(HISTORYPATH, O_RDWR | O_APPEND, 0644);
            char transactionBuffer[1024];
            sprintf(transactionBuffer, "%d credited by loan id %d at %02d:%02d:%02d %d-%d-%d\n", ld.loanAmount, lID, current_time->tm_hour, current_time->tm_min, current_time->tm_sec, (current_time->tm_year)+1900, (current_time->tm_mon)+1, current_time->tm_mday);
            struct trans_histroy th;
            th.acc_no = cs.accountNumber;
            strcpy(th.hist, transactionBuffer);
            write(fp, &th, sizeof(th));
            close(fp);

            lseek(file2, srcOffset2, SEEK_SET);
            write(file2, &cs, sizeof(cs));
            strcpy(resultMsg, "Loan Approved Successfully^");
        }
    }
    else
    {
        ld.status = 3; // Rejected
        strcpy(resultMsg, "Loan Rejected Successfully^");
    }

    lseek(file, srcOffset1, SEEK_SET);
    write(file, &ld, sizeof(ld));

    fl2.l_type = F_UNLCK;
    fcntl(file2, F_SETLK, &fl2);
    close(file2);

    fl1.l_type = F_UNLCK;
    fcntl(file, F_SETLK, &fl1);
    close(file);

    bzero(write_buf, sizeof(write_buf));
    strcpy(write_buf, resultMsg);
    write(connection_fd, write_buf, sizeof(write_buf));
    read(connection_fd, readBuffer, sizeof(readBuffer));
}

void viewAssignedLoan(int connection_fd, int empID)
{
    struct LoanDetails ld;
    int file = open(LOANPATH, O_RDONLY, 0644);
    if(file == -1) return;

    char tempBuffer[4096];
    bzero(tempBuffer, sizeof(tempBuffer));

    while(read(file, &ld, sizeof(ld)) != 0)
    {
        if(ld.empID == empID && ld.status == 1)
        {
            char entry[256];
            sprintf(entry, "Loan ID: %d | Acc No: %d | Amount: %d\n", ld.loanID, ld.accountNumber, ld.loanAmount);
            strcat(tempBuffer, entry);
        }
    }
    close(file);

    bzero(write_buf, sizeof(write_buf));
    if(strlen(tempBuffer) == 0) {
        strcpy(write_buf, "No pending assigned loans^");
    } else {
        strcat(write_buf, tempBuffer);
        strcat(write_buf, "^");
    }
    write(connection_fd, write_buf, sizeof(write_buf));
    read(connection_fd, readBuffer, sizeof(readBuffer));
}

int changeEMPPassword(int connection_fd, int empID)
{
    char newPassword[20];
    struct Employee emp;
    int file = open(EMPPATH, O_CREAT | O_RDWR, 0644);

    lseek(file, 0, SEEK_SET);
    int srcOffset = -1, sourceFound = 0;

    while (read(file, &emp, sizeof(emp)) != 0)
    {
        if(emp.empID == empID)
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

    strcpy(emp.password, crypt(newPassword, HASHKEY));
    lseek(file, srcOffset, SEEK_SET);
    write(file, &emp, sizeof(emp));

    fl1.l_type = F_UNLCK;
    fcntl(file, F_SETLK, &fl1);
    close(file);
    return 1;
}
