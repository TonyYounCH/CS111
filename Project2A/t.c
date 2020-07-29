
void print_errors(char* error){
    if(strcmp(error, "clock_gettime") == 0){
        fprintf(stderr, "Error initializing clock time\n");
        exit(1);
    }
    if(strcmp(error, "thread_create") == 0){
        fprintf(stderr, "Error creating threads.\n");
        exit(2);
    }
    if(strcmp(error, "thread_join") == 0){
        fprintf(stderr, "Error with pthread_join.\n");
        exit(2);
    }
    if(strcmp(error, "mutex") == 0){
        fprintf(stderr, "Error with pthread_join. \n");
        exit(2);
    }
    if(strcmp(error, "segfault") == 0){
        fprintf(stderr, "Segmentation fault caught! \n");
        exit(2);
    }
    if(strcmp(error, "size") == 0){
        fprintf(stderr, "Sorted List length is not zero. List Corrupted\n");
        exit(2);
    }
    if(strcmp(error, "lookup") == 0){
        fprintf(stderr, "Could not retrieve inserted element due to corrupted list.\n");
        exit(2);
    }
    if(strcmp(error, "length") == 0){
        fprintf(stderr, "Could not retrieve length because list is corrupted.\n");
        exit(2);
    }
    if(strcmp(error, "delete") == 0){
        fprintf(stderr, "Could not delete due to corrupted list. \n");
        exit(2);
    }
}


void* thread_worker(void* thread_id){
    int i;
    int num = *((int*)thread_id);
    for(i = num; i < num_elements; i += threads){
        if(opt_sync == 'm'){
            pthread_mutex_lock(&mutex);
            SortedList_insert(head, &pool[i]);
            pthread_mutex_unlock(&mutex);
        }
        else if(opt_sync == 's'){
            while(__sync_lock_test_and_set(&spin_lock, 1));
            SortedList_insert(head, &pool[i]);
            __sync_lock_release(&spin_lock);
        }
        else {
            SortedList_insert(head, &pool[i]);
        }
    }
    int length = 0;
    if(opt_sync == 'm'){
        pthread_mutex_lock(&mutex);
        length = SortedList_length(head);
        if (length < 0){
            print_errors("length");
        }
        pthread_mutex_unlock(&mutex);
    }
    else if(opt_sync == 's'){
        while(__sync_lock_test_and_set(&spin_lock, 1));
        length = SortedList_length(head);
        if (length < 0){
            print_errors("length");
        }
        __sync_lock_release(&spin_lock);
    }
    else {
        length = SortedList_length(head);
        if (length < 0){
            print_errors("length");
        }
    }
    SortedListElement_t* insertedElement;
    num = *((int*)thread_id);
    for(i = num; i < num_elements; i += threads){
        if(opt_sync == 'm'){
            pthread_mutex_lock(&mutex);
            insertedElement = SortedList_lookup(head, pool[i].key);
            if(insertedElement == NULL){
                print_errors("lookup");
            }
            int res = SortedList_delete(insertedElement);
            if(res == 1){
                print_errors("delete");
            }
            pthread_mutex_unlock(&mutex);
        }
        else if(opt_sync == 's'){
            while(__sync_lock_test_and_set(&spin_lock, 1));
            insertedElement = SortedList_lookup(head, pool[i].key);
            if(insertedElement == NULL){
                print_errors("lookup");
            }
            int res = SortedList_delete(insertedElement);
            if(res == 1){
                print_errors("delete");
            }
            __sync_lock_release(&spin_lock);
        }
        else {
            insertedElement = SortedList_lookup(head, pool[i].key);
            if(insertedElement == NULL){
                print_errors("lookup");
            }
            int res = SortedList_delete(insertedElement);
            if(res == 1){
                print_errors("delete");
            }
        }
    }
    return NULL;
}
