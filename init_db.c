#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <crypt.h>

#define HASHKEY "$6$saltsalt$"

struct Customer {
    int accountNumber;
    float balance;
    char customerName[20], password[256];
    int activeStatus;
};

struct Employee {
    int empID;
    char firstName[20], lastName[20], password[256];
    int role; // 0 -> Manager, 1 -> Employee
};

struct Counter {
    int count;
};

int main() {
    // 1. Create employees.txt (Manager & Employee)
    FILE *empFile = fopen("employees.txt", "wb");
    if (empFile) {
        // Manager ID: 201 | Password: mng123
        struct Employee mng = {201, "Jane", "Smith", "", 0};
        strcpy(mng.password, crypt("mng123", HASHKEY));
        fwrite(&mng, sizeof(struct Employee), 1, empFile);

        // Employee ID: 101 | Password: emp123
        struct Employee emp = {101, "John", "Doe", "", 1};
        strcpy(emp.password, crypt("emp123", HASHKEY));
        fwrite(&emp, sizeof(struct Employee), 1, empFile);

        fclose(empFile);
        printf("Created employees.txt successfully.\n");
    }

    // 2. Create customers.txt
    FILE *cusFile = fopen("customers.txt", "wb");
    if (cusFile) {
        // Account Number: 1001 | Password: cus123 | Balance: 5000.00
        struct Customer cus = {1001, 5000.00, "Alice", "", 1};
        strcpy(cus.password, crypt("cus123", HASHKEY));
        fwrite(&cus, sizeof(struct Customer), 1, cusFile);
        fclose(cusFile);
        printf("Created customers.txt successfully.\n");
    }

    // 3. Create loanCounter.txt
    FILE *cntFile = fopen("loanCounter.txt", "wb");
    if (cntFile) {
        struct Counter ct = {0};
        fwrite(&ct, sizeof(struct Counter), 1, cntFile);
        fclose(cntFile);
        printf("Created loanCounter.txt successfully.\n");
    }

    // 4. Create empty database files
    fclose(fopen("loanDetails.txt", "wb"));
    fclose(fopen("trans_hist.txt", "wb"));
    fclose(fopen("feedback.txt", "wb"));
    printf("Initialized loanDetails.txt, trans_hist.txt, and feedback.txt.\n");

    return 0;
}
