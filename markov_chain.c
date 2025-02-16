#include "markov_chain.h"
#include <string.h>
#include <stdio.h>

/**
 * Get random number between 0 and max_number [0, max_number).
 * @param max_number
 * @return Random number
 */
int get_random_number(int max_number)
{
    return rand() % max_number;
}

//helping method to duplicate a string
static char *string_duplicate(const char *string){
    if (string == NULL){
        return NULL;
    }
    char *dup = malloc(strlen(string) + 1);
    if (dup == NULL){
        printf(ALLOCATION_ERROR_MASSAGE);
        return NULL;
    }
    strcpy(dup, string);
    return dup;
}

/**
 * Helping method to check if a word ends with a period
 */
static bool is_end_of_sentence(const char *word){
    size_t len = strlen(word);
    return (len > 0 && word[len - 1] == '.');
}

Node* get_node_from_database(MarkovChain *markov_chain, char *data_ptr){
    if (!markov_chain || !markov_chain->database){
        return NULL;
    }
    Node *cur = markov_chain->database->first;
    while (cur){
        // Compare the actual word strings
        if (strcmp(cur->data->data, data_ptr) == 0){
            return cur;
        }
        cur = cur->next;
    }
    return NULL;
}

Node* add_to_database(MarkovChain *markov_chain, char *data_ptr){
    if (markov_chain==NULL ||markov_chain->database ==NULL||data_ptr==NULL){
        return NULL;
    }

    // Check if this word already exists
    Node *existing_node = get_node_from_database(markov_chain, data_ptr);
    if (existing_node != NULL){
        // Already in database; just return it
        return existing_node;
    }

    // Allocate a new MarkovNode
    MarkovNode *new_markov_node = malloc(sizeof(MarkovNode));
    if (new_markov_node == NULL){
        printf(ALLOCATION_ERROR_MASSAGE);
        return NULL;
    }

    // Copy the word string
    new_markov_node->data = string_duplicate(data_ptr);
    if (new_markov_node->data == NULL){
        free(new_markov_node);
        return NULL;  // string_duplicate already printed an error
    }

    // Initialize frequency list
    new_markov_node->frequency_list = NULL;
    new_markov_node->frequency_list_size = 0; // custom field in MarkovNode

    // Now add this MarkovNode to the linked list with 'add'
    if (add(markov_chain->database, new_markov_node) != 0){
        // If add(...) returned 1, memory allocation for the new Node failed
        printf(ALLOCATION_ERROR_MASSAGE);
        free(new_markov_node->data);
        free(new_markov_node);
        return NULL;
    }

    return markov_chain->database->last; // last is the Node we just added
}

int add_node_to_frequency_list(MarkovNode *first_node, MarkovNode *second_node){
    if (first_node==NULL ||second_node==NULL){
        return 1;
    }
    //to check if the second node is already in the frequency list of the first node
    for (int i = 0; i < first_node->frequency_list_size; i++){
        if (first_node->frequency_list[i].markov_node == second_node)
        {
            first_node->frequency_list[i].frequency++;
            return 0; // Found it, just incremented frequency
        }
    }
    //to add the second node to the frequency list of the first node
    MarkovNodeFrequency *new_list = NULL;
    size_t new_size = (first_node->frequency_list_size + 1) * sizeof(MarkovNodeFrequency);

    if (first_node->frequency_list == NULL){
        new_list = malloc(new_size);
    }
    else{
        new_list = realloc(first_node->frequency_list, new_size);
    }

    if (new_list == NULL){//if the allocation failed
        printf(ALLOCATION_ERROR_MASSAGE);
        return 1;  
    }

    first_node->frequency_list = new_list;
    first_node->frequency_list[first_node->frequency_list_size].markov_node = second_node;
    first_node->frequency_list[first_node->frequency_list_size].frequency = 1;
    first_node->frequency_list_size++;

    return 0;
}

MarkovNode* get_first_random_node(MarkovChain *markov_chain){
    if (!markov_chain || !markov_chain->database || markov_chain->database->size == 0){
        return NULL;
    }

    //We must pick a random node that does NOT end with a .
    //and keep trying until we find one that doesn't end with .
    while (true){
        int rand_index = get_random_number(markov_chain->database->size);

        Node *cur = markov_chain->database->first;
        for (int i = 0; i < rand_index; i++){
            cur = cur->next;
        }
        if (cur==NULL ||cur->data==NULL){
            return NULL;
        }

        //If this word does NOT end with ., return it
        if (!is_end_of_sentence(cur->data->data)){
            return cur->data;
        }
    }
}

MarkovNode* get_next_random_node(MarkovNode *cur_markov_node){
    if (!cur_markov_node || cur_markov_node->frequency_list_size == 0){
        return NULL;
    }

    // sum up the frequencies
    int total_frequency = 0;
    for (int i = 0; i < cur_markov_node->frequency_list_size; i++){
        total_frequency += cur_markov_node->frequency_list[i].frequency;
    }

    int rand = get_random_number(total_frequency);

    // pick the next node based on the random number
    int cumulative = 0;
    for (int i = 0; i < cur_markov_node->frequency_list_size; i++){
        cumulative += cur_markov_node->frequency_list[i].frequency;
        if (rand < cumulative){
            return cur_markov_node->frequency_list[i].markov_node;
        }
    }
    return NULL;
}

void generate_tweet(MarkovNode *first_node, int max_length){
    if (!first_node || max_length < 1){
        return;
    }
    printf("%s", first_node->data);

    MarkovNode *current = first_node;
    int words_printed = 1;

    //generate up to max_length words in total
    while (words_printed < max_length){
        //if this word ends with period then stop generating more
        if (is_end_of_sentence(current->data)){
            break;
        }

        //getting next node
        MarkovNode *next = get_next_random_node(current);
        if (!next){
            break;
        }

        printf(" %s", next->data);
        current = next;
        words_printed++;
    }

    printf("\n");
}

void free_database(MarkovChain **ptr_chain){
    if (ptr_chain==NULL || !(*ptr_chain)){
        return;
    }
    MarkovChain *chain = *ptr_chain;
    if (chain->database==NULL){
        free(chain);
        *ptr_chain = NULL;
        return;
    }

    Node *cur = chain->database->first;
    while (cur){
        Node *next_node = cur->next;
        if (cur->data){
            // Free the MarkovNode
            MarkovNode *mnode = cur->data;
            if (mnode->data){
                free(mnode->data);  // the string
            }
            if (mnode->frequency_list){
                free(mnode->frequency_list);
            }
            free(mnode);
        }
        free(cur);
        cur = next_node;
    }

    free(chain->database);
    free(chain);
    *ptr_chain = NULL;
}
