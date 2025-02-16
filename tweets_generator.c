#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "markov_chain.h"

#define DELIMITERS " \n\t\r"
#define FILE_PATH_ERROR "Error: incorrect file path\n"
#define NUM_ARGS_ERROR "Usage: invalid number of arguments\n"
#define POSITIVE_INT_ERROR "Error: tweets_to_generate and max_words must be positive integers.\n"


int fill_database(FILE *fp, int max_words, MarkovChain *markov_chain){
    char line[1000];
    int words_count = 0;

    while (fgets(line, sizeof(line), fp) != NULL){
        char *word = strtok(line, DELIMITERS);
        char *prev_word = NULL;

        while (word != NULL){
            if (max_words >= 0 && words_count >= max_words){
                break;
            }
            Node *current_node = add_to_database(markov_chain, word);
            if (!current_node){
                //memory error or other issue
                printf(ALLOCATION_ERROR_MASSAGE);
                return EXIT_FAILURE;
            }
            if (prev_word != NULL){
                size_t len = strlen(prev_word);
                if (len == 0 || prev_word[len - 1] != '.'){
                    Node *prev_node = get_node_from_database(markov_chain, prev_word);
                    if (prev_node != NULL){
                        if (add_node_to_frequency_list(prev_node->data, current_node->data) == 1){
                            return EXIT_FAILURE;
                        }
                    }
                }
            }

            prev_word = word;
            words_count++;
            word = strtok(NULL, DELIMITERS);
        }

        if (max_words >= 0 && words_count >= max_words){
            break;
        }
    }

    return EXIT_SUCCESS;
}

/*-------------------------------------------------------------------------
 initialize_markov_chain
 Allocating a MarkovChain and its internal LinkedList...
 Prints an error if allocation fails, but does NOT use perror()
 *------------------------------------------------------------------------*/
MarkovChain* initialize_markov_chain(void){
    MarkovChain *markov_chain = malloc(sizeof(MarkovChain));
    if (!markov_chain){
        printf(ALLOCATION_ERROR_MASSAGE);
        return NULL;
    }

    markov_chain->database = malloc(sizeof(LinkedList));
    if (!markov_chain->database){
        printf(ALLOCATION_ERROR_MASSAGE);
        free(markov_chain);
        return NULL;
    }

    markov_chain->database->first = NULL;
    markov_chain->database->last = NULL;
    markov_chain->database->size = 0;
    return markov_chain;
}

int main(int argc, char *argv[]){
    if (argc != 4 && argc != 5){
        printf(NUM_ARGS_ERROR);
        return EXIT_FAILURE;
    }

    //parse arguments
    unsigned int seed = (unsigned int) atoi(argv[1]);
    int tweets_to_generate = atoi(argv[2]);
    const char *corpus_path = argv[3];

    //if there's a 4th argument, parse max_words, otherwise set to -1 (unlimited).
    int max_words = -1;
    if (argc == 5){
        max_words = atoi(argv[4]);
    }

    //Validate positivity if present
    if (tweets_to_generate <= 0 || (argc == 5 && max_words <= 0)){
        printf(POSITIVE_INT_ERROR);
        return EXIT_FAILURE;
    }

    srand(seed);

    FILE *fp = fopen(corpus_path, "r");
    if (!fp){
        printf(FILE_PATH_ERROR);
        return EXIT_FAILURE;
    }
    MarkovChain *markov_chain = initialize_markov_chain();
    if (!markov_chain){
        fclose(fp);
        return EXIT_FAILURE; // already printed an error
    }

    //filling the MarkovChain from the file
    int fill_result = fill_database(fp, max_words, markov_chain);
    fclose(fp);
    if (fill_result == EXIT_FAILURE)
    {
        free_database(&markov_chain);
        return EXIT_FAILURE; // Already printed an error
    }

    //generating tweets
    for (int i = 0; i < tweets_to_generate; i++){
        MarkovNode *first_node = get_first_random_node(markov_chain);
        if (!first_node){
            // If database is empty or only sentence-ending words => error
            printf("Error: No valid starting node found in the database.\n");
            free_database(&markov_chain);
            return EXIT_FAILURE;
        }
        printf("Tweet %d: ", i + 1);
        int limit = (max_words < 0 || max_words > 20) ? 20 : max_words;
        generate_tweet(first_node, limit);
    }

    //Cleanup
    free_database(&markov_chain);
    return EXIT_SUCCESS;
}
