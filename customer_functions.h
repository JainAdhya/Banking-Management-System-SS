void addFeedback(int connection_fd);
void customerMenu(int connection_fd);
void logout(int connection_fd, int id);
void withdrawMoney(int connection_fd, int accountNumber);
void depositMoney(int connection_fd, int accountNumber);
void customerBal(int connection_fd, int accountNumber);
void applyLoan(int connection_fd, int accountNumber);
void transferFunds(int connection_fd, int accountNumber, int destAcc, float amt);
void transactionHistory(int connection_fd, int accountNumber);
int changePassword(int connection_fd, int accountNumber);
int loginCustomer(int connection_fd, int accountNumber, char *password);
void viewAppliedLoans(int connection_fd, int accountNumber);

int writeBytes, readBytes, key, loginOffset;
char readBuffer[4096], write_buf[4096];

void customerMenu(int connection_fd){
    int accountNumber;
    int destAcc;
    int response = 0;
    char password[20];

start:
    bzero(write_buf, sizeof(write_buf));
    strcpy(write_buf, "\nEnter account number: ");
    writeBytes = write(connection_fd, write_buf, sizeof(write_buf));
    if(writeBytes == -1) return;

    bzero(readBuffer, sizeof(readBuffer));
    readBytes = read(connection_fd, readBuffer, sizeof(readBuffer));
    if(readBytes <= 0) return;
    accountNumber = atoi(readBuffer);

    bzero(write_buf, sizeof(write_buf));
    strcpy(write_buf, "Enter password: ");
    writeBytes = write(connection_fd, write_buf, sizeof(write_buf));
    if(writeBytes == -1) return;

    bzero(readBuffer, sizeof(readBuffer));
    readBytes = read(connection_fd, readBuffer, sizeof(readBuffer));
    strcpy(password, readBuffer);

    if (loginCustomer(connection_fd, accountNumber, password)){
        bzero(write_buf, sizeof(write_buf));
        bzero(readBuffer, sizeof(readBuffer));
        strcpy(write_buf, "\nLogin Successfully^");
        write(connection_fd, write_buf, sizeof(write_buf));
        read(connection_fd, readBuffer, sizeof(readBuffer));

        while(1){
            bzero(write_buf, sizeof(write_buf));
            strcpy(write_buf, CUSMENU);
            writeBytes = write(connection_fd, write_buf, sizeof(write_buf));
            if(writeBytes == -1) break;

            bzero(readBuffer, sizeof(readBuffer));
            int rb = read(connection_fd, readBuffer, sizeof(readBuffer));
            if(rb <= 0) break;
            int choice = atoi(readBuffer);

            switch(choice){
                case 1:
                    depositMoney(connection_fd, accountNumber);
                    break;
                case 2:
                    withdrawMoney(connection_fd, accountNumber);
                    break;
                case 3:
                    customerBal(connection_fd, accountNumber);
                    break;
                case 4:
                    applyLoan(connection_fd, accountNumber);
                    break;
                case 5:
                    bzero(write_buf, sizeof(write_buf));
                    strcpy(write_buf, "Enter dest account number: ");
                    write(connection_fd, write_buf, sizeof(write_buf));
                    bzero(readBuffer, sizeof(readBuffer));
                    read(connection_fd, readBuffer, sizeof(readBuffer));
                    destAcc = atoi(readBuffer);

                    bzero(write_buf, sizeof(write_buf));
                    strcpy(write_buf, "Enter amount: ");
                    write(connection_fd, write_buf, sizeof(write_buf));
                    bzero(readBuffer, sizeof(readBuffer));
                    read(connection_fd, readBuffer, sizeof(readBuffer));
                    float amt = atof(readBuffer);
                    transferFunds(connection_fd, accountNumber, destAcc, amt);
                    break;
                case 6:
                    response = changePassword(connection_fd, accountNumber);
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
                    logout(connection_fd, accountNumber);
                    goto start;
                case 7:
                    transactionHistory(connection_fd, accountNumber);
                    break;
                case 8:
                    addFeedback(connection_fd);
                    break;
                case 9:
                    logout(connection_fd, accountNumber);
                    return;
                case 10:
                    exitClient(connection_fd, accountNumber);
                    return;
                case 11:
                    viewAppliedLoans(connection_fd, accountNumber);
                    break;
                default:
                    bzero(readBuffer, sizeof(readBuffer));
                    write(connection_fd, "Invalid Choice for customer menu^", sizeof("Invalid Choice for customer menu^"));
                    read(connection_fd, readBuffer, sizeof(readBuffer));
            }
        }
    }
    else{
        bzero(write_buf, sizeof(write_buf));
        bzero(readBuffer, sizeof(readBuffer));
        strcpy(write_buf, "\nInvalid ID or Password^");
        write(connection_fd, write_buf, sizeof(write_buf));
        read(connection_fd, readBuffer, sizeof(readBuffer));
        goto start;
    }
}

// Login with credential verification BEFORE semaphore session acquisition
int loginCustomer(int connection_fd, int accountNumber, char *password) {
    struct Customer customer;
    int file = open(CUSPATH, O_CREAT | O_RDWR, 0644);
    if (file == -1) return 0;

    int valid = 0;
    lseek(file, 0, SEEK_SET);
    while(read(file, &customer, sizeof(customer)) != 0)
    {
        if (customer.accountNumber == accountNumber && strcmp(customer.password, crypt(password, HASHKEY)) == 0 && customer.activeStatus == 1) {
            valid = 1;
            break;
        }
    }
    close(file);

    if (!valid) return 0;

    sema = initializeSemaphore(accountNumber);
    setupSignalHandlers();

    if (sem_trywait(sema) == -1) {
        if (errno == EAGAIN) {
            printf("Customer %d already logged in.\n", accountNumber);
        }
        return 0;
    }
    return 1;
}

void depositMoney(int connection_fd, int accountNumber){
    struct Customer customer;
    struct trans_histroy th;
    time_t s = time(NULL);
    struct tm* current_time = localtime(&s);

    int file = open(CUSPATH, O_CREAT | O_RDWR, 0644);
    int fp = open(HISTORYPATH, O_RDWR | O_APPEND | O_CREAT, 0644);

    int found = 0, offset = -1;
    lseek(file, 0, SEEK_SET);
    while(read(file, &customer, sizeof(customer)) != 0) {
        if (customer.accountNumber == accountNumber) {
            offset = lseek(file, -sizeof(struct Customer), SEEK_CUR);
            found = 1;
            break;
        }
    }

    if(!found) {
        close(file); close(fp);
        return;
    }

    struct flock fl = {F_WRLCK, SEEK_SET, offset, sizeof(struct Customer), getpid()};
    fcntl(file, F_SETLKW, &fl);

    bzero(write_buf, sizeof(write_buf));
    strcpy(write_buf, "Enter the amount to deposit: ");
    write(connection_fd, write_buf, sizeof(write_buf));

    bzero(readBuffer, sizeof(readBuffer));
    read(connection_fd, readBuffer, sizeof(readBuffer));
    float depositAmount = atof(readBuffer);

    lseek(file, offset, SEEK_SET);
    read(file, &customer, sizeof(customer));
    customer.balance += depositAmount;

    char transactionBuffer[1024];
    sprintf(transactionBuffer, "%.2f deposited at %02d:%02d:%02d %d-%d-%d\n", depositAmount, current_time->tm_hour, current_time->tm_min, current_time->tm_sec, (current_time->tm_year)+1900, (current_time->tm_mon)+1, current_time->tm_mday);

    bzero(th.hist, sizeof(th.hist));
    strcpy(th.hist, transactionBuffer);
    th.acc_no = customer.accountNumber;
    write(fp, &th, sizeof(th));

    lseek(file, offset, SEEK_SET);
    write(file, &customer, sizeof(customer));

    fl.l_type = F_UNLCK;
    fcntl(file, F_SETLK, &fl);
    close(fp);
    close(file);

    bzero(write_buf, sizeof(write_buf));
    sprintf(write_buf, "Deposit successful!^");
    write(connection_fd, write_buf, sizeof(write_buf));
    read(connection_fd, readBuffer, sizeof(readBuffer));
}

void customerBal(int connection_fd, int accountNumber){
    struct Customer customer;
    int file = open(CUSPATH, O_RDONLY, 0644);
    if (file == -1) return;
    float updatedBalance = 0;

    lseek(file, 0, SEEK_SET);
    while(read(file, &customer, sizeof(customer)) != 0)
    {
        if (customer.accountNumber == accountNumber) {
            updatedBalance = customer.balance;
            break;
        }
    }
    close(file);

    bzero(write_buf, sizeof(write_buf));
    sprintf(write_buf, "The current balance is: %.2f^", updatedBalance);
    write(connection_fd, write_buf, sizeof(write_buf));
    read(connection_fd, readBuffer, sizeof(readBuffer));
}

void withdrawMoney(int connection_fd, int accountNumber){
    struct Customer customer;
    struct trans_histroy th;
    time_t s = time(NULL);
    struct tm* current_time = localtime(&s);

    int file = open(CUSPATH, O_CREAT | O_RDWR, 0644);
    int fp = open(HISTORYPATH, O_RDWR | O_APPEND | O_CREAT, 0644);

    int found = 0, offset = -1;
    lseek(file, 0, SEEK_SET);
    while (read(file, &customer, sizeof(customer)) != 0)
    {
        if(customer.accountNumber == accountNumber)
        {
            offset = lseek(file, -sizeof(struct Customer), SEEK_CUR);
            found = 1;
            break;
        }
    }

    if(!found) {
        close(file); close(fp);
        return;
    }

    struct flock fl = {F_WRLCK, SEEK_SET, offset, sizeof(struct Customer), getpid()};
    fcntl(file, F_SETLKW, &fl);

    bzero(write_buf, sizeof(write_buf));
    strcpy(write_buf, "Enter the amount to withdraw: ");
    write(connection_fd, write_buf, sizeof(write_buf));

    bzero(readBuffer, sizeof(readBuffer));
    read(connection_fd, readBuffer, sizeof(readBuffer));
    float withdrawAmount = atof(readBuffer);

    lseek(file, offset, SEEK_SET);
    read(file, &customer, sizeof(customer));

    if (customer.balance < withdrawAmount){
        bzero(write_buf, sizeof(write_buf));
        sprintf(write_buf, "Insufficient funds!^");
        write(connection_fd, write_buf, sizeof(write_buf));
        read(connection_fd, readBuffer, sizeof(readBuffer));

        fl.l_type = F_UNLCK;
        fcntl(file, F_SETLK, &fl);
        close(fp); close(file);
        return;
    }

    customer.balance -= withdrawAmount;
    char transactionBuffer[1024];
    sprintf(transactionBuffer, "%.2f withdraw at %02d:%02d:%02d %d-%d-%d\n", withdrawAmount, current_time->tm_hour, current_time->tm_min, current_time->tm_sec, (current_time->tm_year)+1900, (current_time->tm_mon)+1, current_time->tm_mday);

    bzero(th.hist, sizeof(th.hist));
    strcpy(th.hist, transactionBuffer);
    th.acc_no = customer.accountNumber;
    write(fp, &th, sizeof(th));

    lseek(file, offset, SEEK_SET);
    write(file, &customer, sizeof(customer));

    fl.l_type = F_UNLCK;
    fcntl(file, F_SETLK, &fl);
    close(fp); close(file);

    bzero(write_buf, sizeof(write_buf));
    sprintf(write_buf, "Withdrawal successful!^");
    write(connection_fd, write_buf, sizeof(write_buf));
    read(connection_fd, readBuffer, sizeof(readBuffer));
}

void applyLoan(int connection_fd, int accountNumber){
    struct Counter ct;
    int file = open(COUNTERPATH, O_CREAT | O_RDWR, 0644);
    read(file, &ct, sizeof(ct));
    int lc = ct.count;
    lseek(file, 0, SEEK_SET);
    ct.count = lc + 1;
    write(file, &ct, sizeof(ct));
    close(file);

    time_t s = time(NULL);
    struct tm* current_time = localtime(&s);

    struct LoanDetails ld;
    int file1 = open(LOANPATH, O_RDWR | O_APPEND | O_CREAT, 0644);

    bzero(write_buf, sizeof(write_buf));
    strcpy(write_buf, "Enter Loan Amount: ");
    write(connection_fd, write_buf, sizeof(write_buf));

    bzero(readBuffer, sizeof(readBuffer));
    read(connection_fd, readBuffer, sizeof(readBuffer));
    int amount = atoi(readBuffer);

    ld.empID = -1;
    ld.accountNumber = accountNumber;
    ld.loanAmount = amount;
    ld.status = 0; // Requested
    ld.loanID = lc + 1;
    sprintf(ld.date_time, "%02d:%02d:%02d %d-%d-%d", current_time->tm_hour, current_time->tm_min, current_time->tm_sec, (current_time->tm_year)+1900, (current_time->tm_mon)+1, current_time->tm_mday);

    write(file1, &ld, sizeof(ld));
    close(file1);

    bzero(write_buf, sizeof(write_buf));
    strcpy(write_buf, "Loan application submitted successfully^");
    write(connection_fd, write_buf, sizeof(write_buf));
    read(connection_fd, readBuffer, sizeof(readBuffer));
}

void viewAppliedLoans(int connection_fd, int accountNumber)
{
    struct LoanDetails ld;
    int file = open(LOANPATH, O_RDONLY, 0644);
    if(file == -1)
    {
        bzero(write_buf, sizeof(write_buf));
        strcpy(write_buf, "No loan records found^");
        write(connection_fd, write_buf, sizeof(write_buf));
        read(connection_fd, readBuffer, sizeof(readBuffer));
        return;
    }

    char tempBuffer[4096];
    bzero(tempBuffer, sizeof(tempBuffer));

    while(read(file, &ld, sizeof(ld)) != 0)
    {
        if(ld.accountNumber == accountNumber)
        {
            char statusStr[20];
            if(ld.status == 0) strcpy(statusStr, "Requested");
            else if(ld.status == 1) strcpy(statusStr, "Pending");
            else if(ld.status == 2) strcpy(statusStr, "Approved");
            else if(ld.status == 3) strcpy(statusStr, "Rejected");

            char entry[256];
            sprintf(entry, "Loan ID: %d | Amount: %d | Date: %s | Status: %s\n", ld.loanID, ld.loanAmount, ld.date_time, statusStr);
            strcat(tempBuffer, entry);
        }
    }
    close(file);

    bzero(write_buf, sizeof(write_buf));
    if(strlen(tempBuffer) == 0) {
        strcpy(write_buf, "No loans applied yet^");
    } else {
        strcat(write_buf, tempBuffer);
        strcat(write_buf, "^");
    }
    write(connection_fd, write_buf, sizeof(write_buf));
    read(connection_fd, readBuffer, sizeof(readBuffer));
}

void transferFunds(int connection_fd, int sourceAccount, int destAccount, float amount) {
    struct Customer cs;
    struct trans_histroy th;
    time_t s = time(NULL);
    struct tm* current_time = localtime(&s);

    int file = open(CUSPATH, O_CREAT | O_RDWR, 0644);
    int fp = open(HISTORYPATH, O_RDWR | O_APPEND | O_CREAT, 0644);

    int sourceFound = 0, destFound = 0;
    int srcOffset = -1, dstOffset = -1;

    lseek(file, 0, SEEK_SET);
    while (read(file, &cs, sizeof(cs)) != 0)
    {
        if(cs.accountNumber == sourceAccount)
        {
            srcOffset = lseek(file, -sizeof(struct Customer), SEEK_CUR);
            sourceFound = 1;
        }
        else if(cs.accountNumber == destAccount)
        {
            dstOffset = lseek(file, -sizeof(struct Customer), SEEK_CUR);
            destFound = 1;
        }
        if(sourceFound && destFound) break;
    }

    if(!destFound || srcOffset == -1 || dstOffset == -1)
    {
        bzero(write_buf, sizeof(write_buf));
        strcpy(write_buf, "Destination account doesn't exist!^");
        write(connection_fd, write_buf, sizeof(write_buf));
        read(connection_fd, readBuffer, sizeof(readBuffer));
        close(file); close(fp);
        return;
    }

    struct flock fl1 = {F_WRLCK, SEEK_SET, srcOffset, sizeof(struct Customer), getpid()};
    fcntl(file, F_SETLKW, &fl1);

    lseek(file, srcOffset, SEEK_SET);
    read(file, &cs, sizeof(cs));

    if (cs.balance < amount) {
        bzero(write_buf, sizeof(write_buf));
        strcpy(write_buf, "Insufficient funds in the source account.^");
        write(connection_fd, write_buf, sizeof(write_buf));
        read(connection_fd, readBuffer, sizeof(readBuffer));

        fl1.l_type = F_UNLCK;
        fcntl(file, F_SETLK, &fl1);
        close(file); close(fp);
        return;
    }

    cs.balance -= amount;
    float srcBalance = cs.balance;

    lseek(file, srcOffset, SEEK_SET);
    write(file, &cs, sizeof(cs));

    struct flock fl2 = {F_WRLCK, SEEK_SET, dstOffset, sizeof(struct Customer), getpid()};
    fcntl(file, F_SETLKW, &fl2);

    lseek(file, dstOffset, SEEK_SET);
    read(file, &cs, sizeof(cs));
    cs.balance += amount;

    lseek(file, dstOffset, SEEK_SET);
    write(file, &cs, sizeof(cs));

    fl1.l_type = F_UNLCK;
    fcntl(file, F_SETLK, &fl1);
    fl2.l_type = F_UNLCK;
    fcntl(file, F_SETLK, &fl2);

    char transactionBuffer[1024];
    sprintf(transactionBuffer, "%.2f transferred to acc %d at %02d:%02d:%02d %d-%d-%d\n", amount, destAccount, current_time->tm_hour, current_time->tm_min, current_time->tm_sec, (current_time->tm_year)+1900, (current_time->tm_mon)+1, current_time->tm_mday);
    bzero(th.hist, sizeof(th.hist));
    strcpy(th.hist, transactionBuffer);
    th.acc_no = sourceAccount;
    write(fp, &th, sizeof(th));

    sprintf(transactionBuffer, "%.2f credited from acc %d at %02d:%02d:%02d %d-%d-%d\n", amount, sourceAccount, current_time->tm_hour, current_time->tm_min, current_time->tm_sec, (current_time->tm_year)+1900, (current_time->tm_mon)+1, current_time->tm_mday);
    bzero(th.hist, sizeof(th.hist));
    strcpy(th.hist, transactionBuffer);
    th.acc_no = destAccount;
    write(fp, &th, sizeof(th));

    close(file); close(fp);

    bzero(write_buf, sizeof(write_buf));
    sprintf(write_buf, "Transfer successful! Balance: %.2f^", srcBalance);
    write(connection_fd, write_buf, sizeof(write_buf));
    read(connection_fd, readBuffer, sizeof(readBuffer));
}

void transactionHistory(int connection_fd, int accountNumber){
    char tempBuffer[4096];
    struct trans_histroy th;
    int maxTrans = 0;

    int file = open(HISTORYPATH, O_RDONLY | O_CREAT, 0644);
    bzero(write_buf, sizeof(write_buf));
    bzero(tempBuffer, sizeof(tempBuffer));

    while(read(file, &th, sizeof(th)) != 0)
    {
        if(th.acc_no == accountNumber && maxTrans < 10)
        {
            maxTrans++;
            strcat(tempBuffer, th.hist);
        }
    }
    close(file);

    if(strlen(tempBuffer) == 0) {
        strcpy(write_buf, "No transaction history found^");
    } else {
        strcpy(write_buf, tempBuffer);
        strcat(write_buf, "^");
    }
    write(connection_fd, write_buf, sizeof(write_buf));
    read(connection_fd, readBuffer, sizeof(readBuffer));
}

void addFeedback(int connection_fd){
    struct FeedBack fb;
    int file = open(FEEDPATH, O_CREAT | O_RDWR | O_APPEND, 0644);
    if(file == -1) return;

    bzero(write_buf, sizeof(write_buf));
    strcpy(write_buf, "Enter Feedback:\n1. Good\n2. Bad\n3. Worse\n");
    write(connection_fd, write_buf, sizeof(write_buf));

    bzero(readBuffer, sizeof(readBuffer));
    read(connection_fd, readBuffer, sizeof(readBuffer));
    int choice = atoi(readBuffer);

    if(choice == 1) strcpy(fb.feedback, "Good");
    else if(choice == 2) strcpy(fb.feedback, "Bad");
    else strcpy(fb.feedback, "Worse");

    write(file, &fb, sizeof(fb));
    close(file);

    bzero(write_buf, sizeof(write_buf));
    strcpy(write_buf, "Feedback submitted successfully^");
    write(connection_fd, write_buf, sizeof(write_buf));
    read(connection_fd, readBuffer, sizeof(readBuffer));
}

int changePassword(int connection_fd, int accountNumber){
    char newPassword[20];
    struct Customer c;
    int file = open(CUSPATH, O_CREAT | O_RDWR, 0644);

    lseek(file, 0, SEEK_SET);
    int srcOffset = -1, sourceFound = 0;

    while (read(file, &c, sizeof(c)) != 0)
    {
        if(c.accountNumber == accountNumber)
        {
            srcOffset = lseek(file, -sizeof(struct Customer), SEEK_CUR);
            sourceFound = 1;
            break;
        }
    }

    if(!sourceFound) {
        close(file);
        return 0;
    }

    struct flock fl1 = {F_WRLCK, SEEK_SET, srcOffset, sizeof(struct Customer), getpid()};
    fcntl(file, F_SETLKW, &fl1);

    bzero(write_buf, sizeof(write_buf));
    strcpy(write_buf, "Enter new password: ");
    write(connection_fd, write_buf, sizeof(write_buf));

    bzero(readBuffer, sizeof(readBuffer));
    read(connection_fd, readBuffer, sizeof(readBuffer));
    strcpy(newPassword, readBuffer);

    strcpy(c.password, crypt(newPassword, HASHKEY));
    lseek(file, srcOffset, SEEK_SET);
    write(file, &c, sizeof(c));

    fl1.l_type = F_UNLCK;
    fcntl(file, F_SETLK, &fl1);
    close(file);
    return 1;
}

void logout(int connection_fd, int id){
    snprintf(semName, 50, "/sem_%d", id);
    sem_t *s = sem_open(semName, 0);
    if (s != SEM_FAILED) {
        sem_post(s);
        sem_close(s);
        sem_unlink(semName);
    }

    bzero(write_buf, sizeof(write_buf));
    strcpy(write_buf, "^");
    write(connection_fd, write_buf, sizeof(write_buf));
    read(connection_fd, readBuffer, sizeof(readBuffer));
}
