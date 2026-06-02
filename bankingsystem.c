#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

//------------------------ STRUCTS ------------------------
typedef struct {
    int accountId;
    char username[30];
    char password[30]; // hashed
    char name[50];
    float balance;
    int isLocked;
    int failedAttempts;
    float dailyWithdraw;
    char lastWithdrawDate[11]; // dd/mm/yyyy
    int isDeleted; // 0=active, 1=removed
} Account;

typedef struct {
    int transactionId;
    int accountId;
    int targetAccountId; // for transfers
    char type[10]; // Deposit, Withdraw, Transfer
    char category[20];
    float amount;
    char timestamp[30];
} Transaction;

typedef struct {
    int disputeId;
    int transactionId;
    int accountId;
    char reason[100];
    char status[10]; // Pending, Resolved
} Dispute;

//------------------------ UTILITY ------------------------
void clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

int getIntInput() {
    int input;
    while(scanf("%d",&input)!=1){
        printf("Invalid input. Enter number: ");
        while(getchar()!='\n');
    }
    return input;
}

float getFloatInput() {
    float input;
    while(scanf("%f",&input)!=1){
        printf("Invalid input. Enter number: ");
        while(getchar()!='\n');
    }
    return input;
}

void pressEnter() {
    printf("\nPress Enter to continue...");
    while(getchar() != '\n');  // consume leftover chars till newline
    getchar();                 // wait for new Enter
}

void getTimestamp(char *buffer){
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    sprintf(buffer,"%02d/%02d/%04d %02d:%02d:%02d",
            t->tm_mday,t->tm_mon+1,t->tm_year+1900,
            t->tm_hour,t->tm_min,t->tm_sec);
}

void getCurrentDate(char *buffer){
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    sprintf(buffer,"%02d/%02d/%04d", t->tm_mday,t->tm_mon+1,t->tm_year+1900);
}

//------------------------ HASH PASSWORD ------------------------
void hashPassword(const char *input, char *output){
    int i;
    for(i=0;i<strlen(input);i++){
        output[i]=(input[i]+5)%256;
    }
    output[i]='\0';
}

//------------------------ FILE HANDLING ------------------------
Account loadAccountByUsername(const char *username){
    FILE *fp = fopen("accounts.dat","rb");
    Account acc;
    acc.accountId=-1;
    if(fp){
        while(fread(&acc,sizeof(Account),1,fp)){
            if(strcmp(acc.username,username)==0 && acc.isDeleted==0){ fclose(fp); return acc;}
        }
        fclose(fp);
    }
    return acc;
}

Account loadAccountById(int id){
    FILE *fp = fopen("accounts.dat","rb");
    Account acc;
    acc.accountId=-1;
    if(fp){
        while(fread(&acc,sizeof(Account),1,fp)){
            if(acc.accountId==id && acc.isDeleted==0){ fclose(fp); return acc;}
        }
        fclose(fp);
    }
    return acc;
}

void saveAccount(Account acc){
    FILE *fp = fopen("accounts.dat","ab");
    if(fp){ fwrite(&acc,sizeof(Account),1,fp); fclose(fp);}
}

void updateAccount(Account acc){
    FILE *fp = fopen("accounts.dat","rb+");
    if(fp){
        Account temp;
        while(fread(&temp,sizeof(Account),1,fp)){
            if(temp.accountId==acc.accountId){
                fseek(fp,-sizeof(Account),SEEK_CUR);
                fwrite(&acc,sizeof(Account),1,fp);
                break;
            }
        }
        fclose(fp);
    }
}

int getNextTransactionId(){
    FILE *fp = fopen("transactions.dat","rb");
    Transaction t;
    int id=1;
    if(fp){
        while(fread(&t,sizeof(Transaction),1,fp)){
            if(t.transactionId>=id) id=t.transactionId+1;
        }
        fclose(fp);
    }
    return id;
}

void addTransaction(Transaction t){
    FILE *fp = fopen("transactions.dat","ab");
    if(fp){ fwrite(&t,sizeof(Transaction),1,fp); fclose(fp);}
}

int getNextDisputeId(){
    FILE *fp = fopen("disputes.dat","rb");
    Dispute d;
    int id=1;
    if(fp){
        while(fread(&d,sizeof(Dispute),1,fp)){
            if(d.disputeId>=id) id=d.disputeId+1;
        }
        fclose(fp);
    }
    return id;
}

void addDispute(Dispute d){
    FILE *fp = fopen("disputes.dat","ab");
    if(fp){ fwrite(&d,sizeof(Dispute),1,fp); fclose(fp);}
}

//------------------------ LOGGING ------------------------
void logEvent(const char *event){
    FILE *fp = fopen("audit.log","a");
    if(fp){
        char ts[30];
        getTimestamp(ts);
        fprintf(fp,"%s : %s\n",ts,event);
        fclose(fp);
    }
}

//------------------------ USER FUNCTIONS ------------------------
void createAccount(){
    clearScreen();
    printf("\033[1;36m===== CREATE ACCOUNT =====\033[0m\n\n");
    Account acc;
    printf("Enter Account ID: "); acc.accountId=getIntInput();
    printf("Enter Name: "); scanf(" %[^\n]s",acc.name);
    printf("Enter Username: "); scanf(" %s",acc.username);
    char pwd[30]; printf("Enter Password: "); scanf(" %s",pwd);
    hashPassword(pwd,acc.password);
    acc.balance=0; acc.isLocked=0; acc.failedAttempts=0; acc.dailyWithdraw=0;
    strcpy(acc.lastWithdrawDate,"00/00/0000"); acc.isDeleted=0;
    saveAccount(acc);
    printf("\n\033[1;32mAccount created successfully!\033[0m\n");
    logEvent("Account created");
    pressEnter();
}

int login(Account *loggedIn){
    clearScreen();
    printf("\033[1;36m===== LOGIN =====\033[0m\n\n");
    char username[30], pwd[30], hashed[30];
    printf("Username: "); scanf(" %s",username);
    printf("Password: "); scanf(" %s",pwd);
    hashPassword(pwd,hashed);
    Account acc = loadAccountByUsername(username);
    if(acc.accountId==-1){ printf("\033[1;31mUser not found.\033[0m\n"); pressEnter(); return 0;}
    if(acc.isLocked){ printf("\033[1;31mAccount locked.\033[0m\n"); pressEnter(); return 0;}
    if(strcmp(acc.password,hashed)!=0){
        printf("\033[1;31mWrong password.\033[0m\n"); acc.failedAttempts++;
        if(acc.failedAttempts>=3){ acc.isLocked=1; printf("\033[1;31mAccount locked after 3 failed attempts.\033[0m\n"); logEvent("Account locked");}
        updateAccount(acc); pressEnter(); return 0;
    }
    acc.failedAttempts=0; updateAccount(acc); *loggedIn=acc;
    char log[100]; sprintf(log,"User %s logged in",acc.username); logEvent(log);
    return 1;
}

void deposit(Account *acc){
    clearScreen(); printf("\033[1;36m===== DEPOSIT =====\033[0m\n\n");
    printf("Balance: %.2f\n",acc->balance);
    float amt; printf("Enter amount: "); amt=getFloatInput();
    if(amt<=0){ printf("\033[1;31mInvalid amount.\033[0m\n"); pressEnter(); return;}
    char cat[20]; printf("Category: "); scanf(" %[^\n]s",cat);
    acc->balance+=amt; updateAccount(*acc);
    Transaction t;
    t.transactionId=getNextTransactionId(); t.accountId=acc->accountId; t.targetAccountId=0;
    strcpy(t.type,"Deposit"); strcpy(t.category,cat); t.amount=amt; getTimestamp(t.timestamp);
    addTransaction(t);
    printf("\n\033[1;32mDeposit successful! Balance: %.2f\033[0m\n",acc->balance);
    char log[100]; sprintf(log,"User %s deposited %.2f",acc->username,amt); logEvent(log);
    pressEnter();
}

void withdraw(Account *acc){
    clearScreen(); printf("\033[1;36m===== WITHDRAW =====\033[0m\n\n");
    printf("Balance: %.2f\n",acc->balance);
    float amt; printf("Enter amount: "); amt=getFloatInput();
    if(amt<=0){ printf("\033[1;31mInvalid amount.\033[0m\n"); pressEnter(); return;}
    if(amt>acc->balance){ printf("\033[1;31mInsufficient balance.\033[0m\n"); pressEnter(); return;}
    char today[11]; getCurrentDate(today);
    if(strcmp(today,acc->lastWithdrawDate)!=0){ acc->dailyWithdraw=0; strcpy(acc->lastWithdrawDate,today);}
    if(acc->dailyWithdraw+amt>20000){ printf("\033[1;31mDaily limit ₹20000 exceeded.\033[0m\n"); pressEnter(); return;}
    char cat[20]; printf("Category: "); scanf(" %[^\n]s",cat);
    acc->balance-=amt; acc->dailyWithdraw+=amt; updateAccount(*acc);
    Transaction t; t.transactionId=getNextTransactionId(); t.accountId=acc->accountId; t.targetAccountId=0;
    strcpy(t.type,"Withdraw"); strcpy(t.category,cat); t.amount=amt; getTimestamp(t.timestamp);
    addTransaction(t);
    if(amt>5000){ printf("\033[1;33mALERT: Large withdrawal!\033[0m\n"); logEvent("Large withdrawal alert");}
    printf("\n\033[1;32mWithdrawal successful! Balance: %.2f\033[0m\n",acc->balance);
    char log[100]; sprintf(log,"User %s withdrew %.2f",acc->username,amt); logEvent(log);
    pressEnter();
}

void transfer(Account *acc){
    clearScreen(); printf("\033[1;36m===== TRANSFER =====\033[0m\n\n");
    printf("Balance: %.2f\n",acc->balance);
    float amt; printf("Enter amount: "); amt=getFloatInput();
    if(amt<=0 || amt>acc->balance){ printf("\033[1;31mInvalid amount.\033[0m\n"); pressEnter(); return;}
    int targetId; printf("Enter target Account ID: "); targetId=getIntInput();
    Account target = loadAccountById(targetId);
    if(target.accountId==-1){ printf("\033[1;31mTarget account not found.\033[0m\n"); pressEnter(); return;}
    char cat[20]; printf("Category: "); scanf(" %[^\n]s",cat);
    acc->balance-=amt; target.balance+=amt; updateAccount(*acc); updateAccount(target);
    Transaction t; t.transactionId=getNextTransactionId(); t.accountId=acc->accountId; t.targetAccountId=targetId;
    strcpy(t.type,"Transfer"); strcpy(t.category,cat); t.amount=amt; getTimestamp(t.timestamp);
    addTransaction(t);
    printf("\n\033[1;32mTransfer successful! New balance: %.2f\033[0m\n",acc->balance);
    char log[100]; sprintf(log,"User %s transferred %.2f to %s",acc->username,amt,target.username); logEvent(log);
    pressEnter();
}

void showTransactions(int accountId){
    clearScreen(); printf("\033[1;36m===== TRANSACTION HISTORY =====\033[0m\n\n");
    FILE *fp=fopen("transactions.dat","rb"); Transaction t;
    if(fp){
        while(fread(&t,sizeof(Transaction),1,fp)){
            if(t.accountId==accountId){
                printf("ID:%d Type:%s Amount:%.2f Category:%s Timestamp:%s",t.transactionId,t.type,t.amount,t.category,t.timestamp);
                if(strcmp(t.type,"Transfer")==0) printf(" TargetID:%d",t.targetAccountId);
                printf("\n");
            }
        }
        fclose(fp);
    }
    pressEnter();
}

void showAnalytics(int accountId){
    clearScreen(); printf("\033[1;36m===== SPENDING ANALYTICS =====\033[0m\n\n");
    FILE *fp=fopen("transactions.dat","rb"); Transaction t;
    float food=0,bills=0,travel=0,other=0;
    if(fp){
        while(fread(&t,sizeof(Transaction),1,fp)){
            if(t.accountId==accountId && strcmp(t.type,"Withdraw")==0){
                if(strcmp(t.category,"Food")==0) food+=t.amount;
                else if(strcmp(t.category,"Bills")==0) bills+=t.amount;
                else if(strcmp(t.category,"Travel")==0) travel+=t.amount;
                else other+=t.amount;
            }
        }
        fclose(fp);
    }
    printf("Food: %.2f\nBills: %.2f\nTravel: %.2f\nOther: %.2f\n",food,bills,travel,other);
    pressEnter();
}

void raiseDispute(int accountId){
    clearScreen(); printf("\033[1;36m===== RAISE DISPUTE =====\033[0m\n\n");
    int txId; char reason[100];
    printf("Enter Transaction ID: "); txId=getIntInput();
    printf("Enter reason: "); scanf(" %[^\n]s",reason);
    Dispute d; d.disputeId=getNextDisputeId(); d.accountId=accountId; d.transactionId=txId;
    strcpy(d.reason,reason); strcpy(d.status,"Pending");
    addDispute(d);
    printf("\n\033[1;32mDispute raised successfully.\033[0m\n");
    logEvent("Dispute raised");
    pressEnter();
}

void viewDisputes(int accountId){
    clearScreen(); 
    printf("\033[1;36m===== YOUR DISPUTES =====\033[0m\n\n");
    FILE *fp = fopen("disputes.dat","rb"); 
    if(!fp){
        printf("Error: disputes.dat file not found.\n");
        pressEnter();
        return;
    }

    Dispute d;
    int count = 0;
    while(fread(&d,sizeof(Dispute),1,fp) == 1){
        if(d.accountId == accountId){
            printf("DisputeID: %d  TxID: %d  Status: %s\nReason: %s\n\n", 
                    d.disputeId, d.transactionId, d.status, d.reason);
            count++;
        }
    }
    fclose(fp);

    if(count == 0){
        printf("No disputes found for your account.\n");
    }
    pressEnter();
}

void printAllDisputes(){
    clearScreen();
    printf("\033[1;36m===== ALL DISPUTES =====\033[0m\n\n");
    FILE *fp = fopen("disputes.dat","rb");
    if(!fp){
        printf("Error: disputes.dat file not found.\n");
        pressEnter();
        return;
    }
    Dispute d;
    int count = 0;
    while(fread(&d,sizeof(Dispute),1,fp) == 1){
        printf("DisputeID: %d  AccountID: %d  TxID: %d  Status: %s\nReason: %s\n\n", 
               d.disputeId, d.accountId, d.transactionId, d.status, d.reason);
        count++;
    }
    fclose(fp);

    if(count == 0){
        printf("No disputes in the system.\n");
    }
    pressEnter();
}

//------------------------ DASHBOARDS ------------------------
void userDashboard(Account acc){
    int choice;
    do{
        clearScreen();
        printf("\033[1;36m===== USER DASHBOARD =====\033[0m\n\n");
        printf("Welcome %s (ID:%d) Balance: %.2f\n\n",acc.username,acc.accountId,acc.balance);
        printf("1. Deposit\n2. Withdraw\n3. Transfer\n4. Transaction History\n5. Analytics\n6. Raise Dispute\n7. View Disputes\n0. Logout\n\nChoice: ");
        choice=getIntInput();
        switch(choice){
            case 1: deposit(&acc); break;
            case 2: withdraw(&acc); break;
            case 3: transfer(&acc); break;
            case 4: showTransactions(acc.accountId); break;
            case 5: showAnalytics(acc.accountId); break;
            case 6: raiseDispute(acc.accountId); break;
            case 7: viewDisputes(acc.accountId); break;
        }
    }while(choice!=0);
}

void adminPanel(){
    int choice;
    do{
        clearScreen(); printf("\033[1;35m===== ADMIN PANEL =====\033[0m\n\n");
        printf("1. View Accounts\n2. Lock/Unlock Account\n3. Remove Account\n4. View Transactions\n5. Resolve Disputes\n6. View All Disputes\n0. Logout\n\nChoice: ");
        choice=getIntInput();
        if(choice==1){
            FILE *fp=fopen("accounts.dat","rb"); Account a;
            printf("\nID Username Balance Status\n");
            if(fp){ while(fread(&a,sizeof(Account),1,fp)){ if(a.isDeleted==0) printf("%d %s %.2f %s\n",a.accountId,a.username,a.balance,a.isLocked?"Locked":"Active");} fclose(fp);}
            pressEnter();
        }
        else if(choice==2){
            int id,act; printf("Enter Account ID: "); id=getIntInput();
            Account a=loadAccountById(id);
            if(a.accountId==-1){ printf("Not found.\n"); pressEnter(); continue;}
            printf("1. Lock \n2. Unlock\nChoice: "); act=getIntInput();
            if(act==1) a.isLocked=1; else if(act==2) a.isLocked=0;
            updateAccount(a);
            printf("Updated successfully.\n"); pressEnter();
        }
        else if(choice==3){
            int id; printf("Enter Account ID to remove: "); id=getIntInput();
            Account a=loadAccountById(id);
            if(a.accountId==-1){ printf("Not found.\n"); pressEnter(); continue;}
            a.isDeleted=1; updateAccount(a);
            printf("Account removed.\n"); pressEnter();
        }
        else if(choice==4){
            FILE *fp=fopen("transactions.dat","rb"); Transaction t;
            printf("\nID Type Amount Category Timestamp\n");
            if(fp){ while(fread(&t,sizeof(Transaction),1,fp)) printf("%d %s %.2f %s %s\n",t.transactionId,t.type,t.amount,t.category,t.timestamp); fclose(fp);}
            pressEnter();
        }
        else if(choice==5){
            FILE *fp=fopen("disputes.dat","rb+"); Dispute d;
            if(!fp){ printf("No disputes.\n"); pressEnter(); continue;}
            int found=0;
            while(fread(&d,sizeof(Dispute),1,fp)){
                if(strcmp(d.status,"Pending")==0){
                    found=1;
                    clearScreen();
                    printf("DisputeID: %d  AccountID: %d  TxID: %d  Status: %s\nReason: %s\n\n", d.disputeId, d.accountId, d.transactionId, d.status, d.reason);
                    printf("Mark as Resolved? (1=Yes/0=No): ");
                    int ch=getIntInput();
                    if(ch==1){
                        strcpy(d.status,"Resolved");
                        fseek(fp,-sizeof(Dispute),SEEK_CUR);
                        fwrite(&d,sizeof(Dispute),1,fp);
                        printf("Dispute resolved.\n");
                        logEvent("Dispute resolved by admin");
                    }
                    pressEnter();
                }
            }
            fclose(fp);
            if(!found){ printf("No pending disputes.\n"); pressEnter();}
        }
        else if(choice==6){
            printAllDisputes();
        }
    }while(choice!=0);
}

//------------------------ MAIN ------------------------
int main(){
    int choice;
    Account loggedIn;
    do{
        clearScreen();
        printf("\033[1;34m===== BANKING SYSTEM =====\033[0m\n\n");
        printf("1. Create Account\n2. Login\n0. Exit\nChoice: ");
        choice=getIntInput();
        if(choice==1) createAccount();
        else if(choice==2){
            if(login(&loggedIn)){
                if(strcmp(loggedIn.username,"admin")==0){
                    adminPanel();
                }else{
                    userDashboard(loggedIn);
                }
            }
        }
    }while(choice!=0);

    printf("\nThank you for using the system!\n");
    return 0;
}
