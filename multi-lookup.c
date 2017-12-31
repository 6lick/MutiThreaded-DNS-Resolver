#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include "util.h"
#include "multi-lookup.h"



char sharedBuffer[2000][20];
char files_to_service[20][20];

char txtResults[20];
char txtServices[20];

int GLOBALFILECOUNT;

struct thread_Data{
char *ptr_to_fileArray;
};
//locks for files
pthread_mutex_t lock[10] = {
    PTHREAD_MUTEX_INITIALIZER,
    PTHREAD_MUTEX_INITIALIZER,
    PTHREAD_MUTEX_INITIALIZER,
    PTHREAD_MUTEX_INITIALIZER,
    PTHREAD_MUTEX_INITIALIZER,
    PTHREAD_MUTEX_INITIALIZER,
    PTHREAD_MUTEX_INITIALIZER,
    PTHREAD_MUTEX_INITIALIZER,
    PTHREAD_MUTEX_INITIALIZER,
    PTHREAD_MUTEX_INITIALIZER
};
//shared buffer lock
pthread_mutex_t sharedBufferLock = PTHREAD_MUTEX_INITIALIZER;

int bufferIndex = 0;

//lock for service file
pthread_mutex_t serviceLock = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t resultsLock = PTHREAD_MUTEX_INITIALIZER;


//takes struct containing file descripter and mutexs

void *requester_entryFunction(void *thd){
   // pthread_mutex_lock(&lock);
    pid_t tid = syscall(SYS_gettid);
    struct thread_Data *td = thd;
    for (int i = 0; i < GLOBALFILECOUNT; i++){ 
        if(pthread_mutex_trylock(&lock[i])==0){    
            //if true thread now owns lock
            char fileTobeServiced[20];
            strcpy(fileTobeServiced,(td->ptr_to_fileArray + (i*20)));
            printf("%s\n", fileTobeServiced);
            pid_t tid = syscall(SYS_gettid);
            printf("%d is servicing %s\n", tid, fileTobeServiced);
            
            pthread_mutex_lock(&serviceLock);
            FILE *f = fopen(txtServices, "a");
            if (f == NULL)
            {
                printf("Error opening file!\n");
                exit(1);
            }
            
            /* print some text */

            fprintf(f, "thread: %d is servicing %s\n", tid, fileTobeServiced);
            fclose(f);
            pthread_mutex_unlock(&serviceLock);



            FILE* input;
            input = fopen(fileTobeServiced, "r");
            if(!input){
                printf("Error\n");
            }
            char tmpBuff[20];
            while(fgets(tmpBuff, 20, input) != NULL){
                strcpy(sharedBuffer[bufferIndex], tmpBuff);
                bufferIndex++;
            }
            
            pthread_mutex_destroy(&lock[i]);
        }
        else{
            //printf("%d failed to unlock %s\n", tid, td->ptr_to_fileArray+(i*20));
        }
    }
    return 0;

}




//function taken from stackoverflow to remove white space **equivilent to an intext citation**
void fnStrTrimInPlace(char *szWrite) {
    
        const char *szWriteOrig = szWrite;
        char       *szLastSpace = szWrite, *szRead = szWrite;
        int        bNotSpace;
    
        // SHIFT STRING, STARTING AT FIRST NON-SPACE CHAR, LEFTMOST
        while( *szRead != '\0' ) {
    
            bNotSpace = !isspace((unsigned char)(*szRead));
    
            if( (szWrite != szWriteOrig) || bNotSpace ) {
    
                *szWrite = *szRead;
                szWrite++;
    
                // TRACK POINTER TO LAST NON-SPACE
                if( bNotSpace )
                    szLastSpace = szWrite;
            }
    
            szRead++;
        }
    
        // TERMINATE AFTER LAST NON-SPACE (OR BEGINNING IF THERE WAS NO NON-SPACE)
        *szLastSpace = '\0';
    }





void *resolver_entryFunction(){
    pthread_mutex_lock(&sharedBufferLock);
    for(int i = 0; i < bufferIndex - 1; i++){
        char domainTOresolve[20];
        char ip[20];
        char *tmp = &domainTOresolve;
        bufferIndex--;
        strcpy(domainTOresolve, sharedBuffer[i]);
        fnStrTrimInPlace(tmp);
        printf("%s", tmp);
        int ret = dnslookup(tmp, ip, sizeof(ip));
        pthread_mutex_lock(&resultsLock);
        FILE *f = fopen(txtResults, "a");
        if (f == NULL)
        {
            printf("Error opening file!\n");
            exit(1);
        }
        
        /* print some text */

        fprintf(f, "%s, %s\n", domainTOresolve, ip);
        fclose(f);


        pthread_mutex_unlock(&resultsLock);
        printf("%s\n", ip);
    }
    pthread_mutex_unlock(&sharedBufferLock);

}




 int main (int argc, char *argv[]){
     //serviced file
     strcpy(txtServices, argv[4]);
     // results file
     strcpy(txtResults, argv[3]);
    //requester threads 
    int numb_req_threads = strtol(argv[1], NULL, 10); //2
    pthread_t rTID[numb_req_threads];
    struct thread_Data thread_data_array[numb_req_threads];
    
    //resolver threads
    int numb_resolver_threads = strtol(argv[2],NULL,10); //3
    pthread_t resolverThreads[numb_resolver_threads];
    
     // test struct
    struct thread_Data test;
    char f[20][20];

    int fileCount = argc - 5;
    GLOBALFILECOUNT = fileCount;
    for(int i = 0; i < fileCount; i++){
        strcpy(f[i], argv[i+5]);     
    }
    
    //test.ptr_to_fileArray = &f;
    //printf("%s\n", test.ptr_to_fileArray+20);
    //pthread_t tid1;

    
    //get files to be serviced
    
    char inputFiles[20][20];
     
    // for (int i = 0; i <= 2; i++){
    //     strcpy(inputFiles[i], f[i]);
    //     fileCount++;
    // }
    //pthread_mutex_t fileLockArray[fileCount];

    //populate thread data and global files to service 
    for (int i = 0; i <= numb_req_threads; i++){
        thread_data_array[i].ptr_to_fileArray = &f;
        //thread_data_array[i].fileLocksPTR = &fileLockArray;
    }

    ////create test test thread    
    //pthread_create(&tid1, NULL, requester_entryFunction, &test);
    
    //create requester threads
    for (int i = 0; i < numb_req_threads; i++){
        pthread_create(&rTID[i], NULL, requester_entryFunction, &thread_data_array[i]);
    } 
    //join requester threads
    for (int i = 0; i < numb_req_threads; i++){
        pthread_join(rTID[i],NULL);
    }

    //create resolver threads
    for (int i = 0; i < numb_resolver_threads; i++){
        pthread_create(&resolverThreads[i], NULL, resolver_entryFunction, NULL);
    }
    //join resolver threads
    for (int i =0; i< numb_resolver_threads; i++){
        pthread_join(resolverThreads[i],NULL);
    }
    // join test thread
    //pthread_join(tid1, NULL);
    // for (int i = 0; i < bufferIndex; i++){
    //     printf("%s\n", sharedBuffer[i]);
    // }

}
