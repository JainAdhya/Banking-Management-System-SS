#define ADMINNAME "admin"
#define PASSWORD "admin@123"

int addEmployee(int connection_fd);
void modifyCE(int connection_fd, int modifyChoice);
void manageRole(int connection_fd);
void addCustomerAdmin(int connection_fd);
void deleteCustomerAdmin(int connection_fd);
void searchCustomerAdmin(int connection_fd);

char readBuffer[4096], write_buf[4096];

void adminMenu(int connection_fd)
{
    char password[20];
start:
    bzero(write_buf, sizeof(write_buf));
    strcat(write_buf, "Enter password: ");
    write(connection_fd, write_buf, sizeof(write_buf));

    bzero(readBuffer, sizeof(readBuffer));
    read(connection_fd, readBuffer, sizeof(readBuffer));
    strcpy(password, readBuffer);

    if(strcmp(PASSWORD, password) == 0)
    {
        bzero(write_buf, sizeof(write_buf));
        bzero(readBuffer, sizeof(readBuffer));
        strcpy(write_buf, "\nLogin Successfully^");
        write(connection_fd, write_buf, sizeof(write_buf));
        read(connection_fd, readBuffer, sizeof(readBuffer));
    }
    else
    {
        bzero(write_buf, sizeof(write_buf));
        bzero(readBuffer, sizeof(readBuffer));
        strcpy(write_buf, "\nInvalid credential^");
        write(connection_fd, write_buf, sizeof(write_buf));
        read(connection_fd, readBuffer, sizeof(readBuffer));
        goto start;
    }

    while(1)
    {
        int modifyChoice;
        int choice;

        bzero(write_buf, sizeof(write_buf));
        strcpy(write_buf, ADMINMENU);
        write(connection_fd, write_buf, sizeof(write_buf));

        bzero(readBuffer, sizeof(readBuffer));
        read(connection_fd, readBuffer, sizeof(readBuffer));
        choice = atoi(readBuffer);
        printf("Admin entered: %d\n", choice);

        switch (choice) {
            case 1:
                if(addEmployee(connection_fd))
                {
                    bzero(write_buf, sizeof(write_buf));
                    bzero(readBuffer, sizeof(readBuffer));
                    strcpy(write_buf, "Employee successfully added\n^");
                    write(connection_fd, write_buf, sizeof(write_buf));
                    read(connection_fd, readBuffer, sizeof(readBuffer));
                }
                break;
            case 2:
                bzero(write_buf, sizeof(write_buf));
                strcpy(write_buf, "Enter 1 to Modify Customer\nEnter 2 to Modify Employee: ");
                write(connection_fd, write_buf, sizeof(write_buf));

                bzero(readBuffer, sizeof(readBuffer));
                read(connection_fd, readBuffer, sizeof(readBuffer));
                modifyChoice = atoi(readBuffer);

                modifyCE(connection_fd, modifyChoice);
                break;
            case 3:
                manageRole(connection_fd);
                break;
            case 4:
                addCustomerAdmin(connection_fd);
                break;
            case 5:
                deleteCustomerAdmin(connection_fd);
                break;
            case 6:
                searchCustomerAdmin(connection_fd);
                break;
            case 7:
                return;
            default:
                bzero(write_buf, sizeof(write_buf));
                bzero(readBuffer, sizeof(readBuffer));
                strcpy(write_buf, "Invalid choice! Please try again.^");
                write(connection_fd, write_buf, sizeof(write_buf));
                read(connection_fd, readBuffer, sizeof(readBuffer));
        }
    }
}

int addEmployee(int connection_fd)
{
    struct Employee emp;

    bzero(write_buf, sizeof(write_buf));
    strcpy(write_buf, "Enter Employee ID: ");
    write(connection_fd, write_buf, sizeof(write_buf));

    bzero(readBuffer, sizeof(readBuffer));
    read(connection_fd, readBuffer, sizeof(readBuffer));
    emp.empID = atoi(readBuffer);

    bzero(write_buf, sizeof(write_buf));
    strcpy(write_buf, "Enter FirstName: ");
    write(connection_fd, write_buf, sizeof(write_buf));

    bzero(readBuffer, sizeof(readBuffer));
    read(connection_fd, readBuffer, sizeof(readBuffer));
    strcpy(emp.firstName, readBuffer);

    bzero(write_buf, sizeof(write_buf));
    strcpy(write_buf, "Enter LastName: ");
    write(connection_fd, write_buf, sizeof(write_buf));

    bzero(readBuffer, sizeof(readBuffer));
    read(connection_fd, readBuffer, sizeof(readBuffer));
    strcpy(emp.lastName, readBuffer);

    bzero(write_buf, sizeof(write_buf));
    strcpy(write_buf, "Enter Password: ");
    write(connection_fd, write_buf, sizeof(write_buf));

    bzero(readBuffer, sizeof(readBuffer));
    read(connection_fd, readBuffer, sizeof(readBuffer));
    strcpy(emp.password, crypt(readBuffer, HASHKEY));

    emp.role = 1;

    int file = open(EMPPATH, O_CREAT | O_RDWR, 0644);
    if(file == -1) return 0;

    lseek(file, 0, SEEK_END);
    int returnValue = write(file, &emp, sizeof(emp));
    close(file);
    return returnValue != -1;
}

void modifyCE(int connection_fd, int modifyChoice)
{
    if(modifyChoice == 1)
    {
        int file = open(CUSPATH, O_CREAT | O_RDWR, 0644);
        if(file == -1) return;

        bzero(write_buf, sizeof(write_buf));
        strcpy(write_buf, "Enter Account Number: ");
        write(connection_fd, write_buf, sizeof(write_buf));

        int accNo;
        bzero(readBuffer, sizeof(readBuffer));
        read(connection_fd, readBuffer, sizeof(readBuffer));
        accNo = atoi(readBuffer);

        struct Customer c;
        lseek(file, 0, SEEK_SET);
        int srcOffset = -1, sourceFound = 0;

        while (read(file, &c, sizeof(c)) != 0)
        {
            if(c.accountNumber == accNo)
            {
                srcOffset = lseek(file, -sizeof(struct Customer), SEEK_CUR);
                sourceFound = 1;
                break;
            }
        }

        if(!sourceFound)
        {
            bzero(write_buf, sizeof(write_buf));
            strcpy(write_buf, "Invalid Account Number\n^");
            write(connection_fd, write_buf, sizeof(write_buf));
            read(connection_fd, readBuffer, sizeof(readBuffer));
            close(file);
            return;
        }

        struct flock fl1 = {F_WRLCK, SEEK_SET, srcOffset, sizeof(struct Customer), getpid()};
        fcntl(file, F_SETLKW, &fl1);

        char newName[20];
        bzero(write_buf, sizeof(write_buf));
        strcpy(write_buf, "Enter New Name: ");
        write(connection_fd, write_buf, sizeof(write_buf));

        bzero(readBuffer, sizeof(readBuffer));
        read(connection_fd, readBuffer, sizeof(readBuffer));
        strcpy(newName, readBuffer);

        strcpy(c.customerName, newName);
        lseek(file, srcOffset, SEEK_SET);
        write(file, &c, sizeof(c));

        fl1.l_type = F_UNLCK;
        fcntl(file, F_SETLK, &fl1);
        close(file);

        bzero(write_buf, sizeof(write_buf));
        strcpy(write_buf, "Customer modified successfully^");
        write(connection_fd, write_buf, sizeof(write_buf));
        read(connection_fd, readBuffer, sizeof(readBuffer));
    }
    else if(modifyChoice == 2)
    {
        struct Employee emp;
        int file = open(EMPPATH, O_CREAT | O_RDWR, 0644);
        if(file == -1) return;

        bzero(write_buf, sizeof(write_buf));
        strcpy(write_buf, "Enter Employee ID: ");
        write(connection_fd, write_buf, sizeof(write_buf));

        int id;
        bzero(readBuffer, sizeof(readBuffer));
        read(connection_fd, readBuffer, sizeof(readBuffer));
        id = atoi(readBuffer);

        lseek(file, 0, SEEK_SET);
        int srcOffset = -1, sourceFound = 0;

        while (read(file, &emp, sizeof(emp)) != 0)
        {
            if(emp.empID == id)
            {
                srcOffset = lseek(file, -sizeof(struct Employee), SEEK_CUR);
                sourceFound = 1;
                break;
            }
        }

        if(!sourceFound)
        {
            bzero(write_buf, sizeof(write_buf));
            strcpy(write_buf, "Invalid Employee ID^");
            write(connection_fd, write_buf, sizeof(write_buf));
            read(connection_fd, readBuffer, sizeof(readBuffer));
            close(file);
            return;
        }

        struct flock fl1 = {F_WRLCK, SEEK_SET, srcOffset, sizeof(struct Employee), getpid()};
        fcntl(file, F_SETLKW, &fl1);

        char newName[20];
        bzero(write_buf, sizeof(write_buf));
        strcpy(write_buf, "Enter New First Name: ");
        write(connection_fd, write_buf, sizeof(write_buf));

        bzero(readBuffer, sizeof(readBuffer));
        read(connection_fd, readBuffer, sizeof(readBuffer));
        strcpy(newName, readBuffer);

        strcpy(emp.firstName, newName);
        lseek(file, srcOffset, SEEK_SET);
        write(file, &emp, sizeof(emp));

        fl1.l_type = F_UNLCK;
        fcntl(file, F_SETLK, &fl1);
        close(file);

        bzero(write_buf, sizeof(write_buf));
        strcpy(write_buf, "Employee modified successfully^");
        write(connection_fd, write_buf, sizeof(write_buf));
        read(connection_fd, readBuffer, sizeof(readBuffer));
    }
}

void manageRole(int connection_fd)
{
    int file = open(EMPPATH, O_CREAT | O_RDWR, 0644);
    if(file == -1) return;

    bzero(write_buf, sizeof(write_buf));
    strcpy(write_buf, "Enter ID: ");
    write(connection_fd, write_buf, sizeof(write_buf));

    int id;
    bzero(readBuffer, sizeof(readBuffer));
    read(connection_fd, readBuffer, sizeof(readBuffer));
    id = atoi(readBuffer);

    struct Employee emp;
    int found = 0, offset = -1;
    while(read(file, &emp, sizeof(emp)) != 0)
    {
        if(emp.empID == id)
        {
            offset = lseek(file, -sizeof(struct Employee), SEEK_CUR);
            found = 1;
            break;
        }
    }

    if(!found)
    {
        bzero(write_buf, sizeof(write_buf));
        strcpy(write_buf, "Invalid ID^");
        write(connection_fd, write_buf, sizeof(write_buf));
        read(connection_fd, readBuffer, sizeof(readBuffer));
        close(file);
        return;
    }

    struct flock fl = {F_WRLCK, SEEK_SET, offset, sizeof(struct Employee), getpid()};
    fcntl(file, F_SETLKW, &fl);

    int choice;
    bzero(write_buf, sizeof(write_buf));
    strcpy(write_buf, "Enter 1 to make manager\nEnter 2 to make employee: ");
    write(connection_fd, write_buf, sizeof(write_buf));

    bzero(readBuffer, sizeof(readBuffer));
    read(connection_fd, readBuffer, sizeof(readBuffer));
    choice = atoi(readBuffer);

    lseek(file, offset, SEEK_SET);
    read(file, &emp, sizeof(emp));
    if(choice == 1) emp.role = 0;
    else if(choice == 2) emp.role = 1;

    lseek(file, offset, SEEK_SET);
    write(file, &emp, sizeof(emp));

    fl.l_type = F_UNLCK;
    fcntl(file, F_SETLK, &fl);
    close(file);

    bzero(write_buf, sizeof(write_buf));
    strcpy(write_buf, "Role updated successfully^");
    write(connection_fd, write_buf, sizeof(write_buf));
    read(connection_fd, readBuffer, sizeof(readBuffer));
}

void addCustomerAdmin(int connection_fd)
{
    struct Customer customer;
    int file = open(CUSPATH, O_RDWR | O_APPEND | O_CREAT, 0644);
    if(file == -1) return;

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

    write(file, &customer, sizeof(customer));
    close(file);

    bzero(write_buf, sizeof(write_buf));
    strcpy(write_buf, "Customer added successfully by Admin!^");
    write(connection_fd, write_buf, sizeof(write_buf));
    read(connection_fd, readBuffer, sizeof(readBuffer));
}

void deleteCustomerAdmin(int connection_fd)
{
    int file = open(CUSPATH, O_RDWR, 0644);
    if(file == -1)
    {
        bzero(write_buf, sizeof(write_buf));
        strcpy(write_buf, "Error opening customer database^");
        write(connection_fd, write_buf, sizeof(write_buf));
        read(connection_fd, readBuffer, sizeof(readBuffer));
        return;
    }

    bzero(write_buf, sizeof(write_buf));
    strcpy(write_buf, "Enter Account Number to Delete: ");
    write(connection_fd, write_buf, sizeof(write_buf));
    bzero(readBuffer, sizeof(readBuffer));
    read(connection_fd, readBuffer, sizeof(readBuffer));
    int targetAcc = atoi(readBuffer);

    int tempFile = open("temp_cus.txt", O_RDWR | O_CREAT | O_TRUNC, 0644);
    struct Customer c;
    int found = 0;

    lseek(file, 0, SEEK_SET);
    while(read(file, &c, sizeof(c)) > 0)
    {
        if(c.accountNumber == targetAcc)
        {
            found = 1;
            continue; // Skip writing to temp file (soft delete / removal)
        }
        write(tempFile, &c, sizeof(c));
    }

    close(file);
    close(tempFile);

    if(found)
    {
        rename("temp_cus.txt", CUSPATH);
        bzero(write_buf, sizeof(write_buf));
        strcpy(write_buf, "Customer account deleted successfully!^");
    }
    else
    {
        unlink("temp_cus.txt");
        bzero(write_buf, sizeof(write_buf));
        strcpy(write_buf, "Account number not found!^");
    }

    write(connection_fd, write_buf, sizeof(write_buf));
    read(connection_fd, readBuffer, sizeof(readBuffer));
}

void searchCustomerAdmin(int connection_fd)
{
    int file = open(CUSPATH, O_RDONLY, 0644);
    if(file == -1) return;

    bzero(write_buf, sizeof(write_buf));
    strcpy(write_buf, "Enter Account Number to Search: ");
    write(connection_fd, write_buf, sizeof(write_buf));
    bzero(readBuffer, sizeof(readBuffer));
    read(connection_fd, readBuffer, sizeof(readBuffer));
    int targetAcc = atoi(readBuffer);

    struct Customer c;
    int found = 0;
    while(read(file, &c, sizeof(c)) > 0)
    {
        if(c.accountNumber == targetAcc)
        {
            found = 1;
            break;
        }
    }
    close(file);

    bzero(write_buf, sizeof(write_buf));
    if(found)
    {
        sprintf(write_buf, "Account Found:\nAcc No: %d\nName: %s\nBalance: %.2f\nStatus: %s^",
                c.accountNumber, c.customerName, c.balance, c.activeStatus ? "Active" : "Deactivated");
    }
    else
    {
        strcpy(write_buf, "Customer account not found!^");
    }

    write(connection_fd, write_buf, sizeof(write_buf));
    read(connection_fd, readBuffer, sizeof(readBuffer));
}
