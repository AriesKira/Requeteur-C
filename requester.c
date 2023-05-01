#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <curl/curl.h>
#include <jansson.h>
#include <mysql/mysql.h>

typedef struct spoonacularReceipe spoonacularReceipe;

struct spoonacularReceipe{
    int id;
    const char *title;
    const char *image;
};



// Fonction de rappel appelée par CURL pour chaque donnée reçue
static size_t write_callback(char *ptr, size_t size, size_t nmemb, void *userdata) {
    size_t real_size = size * nmemb;
    char **response_ptr = (char **)userdata;
    
    // Allouer de la mémoire pour la nouvelle réponse
    char *new_response = realloc(*response_ptr, strlen(*response_ptr) + real_size + 1);
    if (new_response == NULL) {
        fprintf(stderr, "Error: could not allocate memory\n");
        return 0;
    }
    
    // Copier la réponse dans la nouvelle zone mémoire
    memcpy(new_response + strlen(*response_ptr), ptr, real_size);
    new_response[strlen(*response_ptr) + real_size] = '\0';
    
    // Mettre à jour la variable pointée par response_ptr
    *response_ptr = new_response;
    
    return real_size;
}

void mysqlError(const char *msg, MYSQL *conn) {
    fprintf(stderr, "%s: %s\n", msg, mysql_error(conn));
    mysql_close(conn);
    exit(0);
}


int main(int argc, char *argv[]) {
    CURL *curl = curl_easy_init();
    CURLcode res;
    char *method;
    char *target;
    char *response = NULL;
    FILE *rspFile = NULL;
    char* url;
    char *params;
    int urlSize;
    MYSQL *conn;
    
    //alloc de la place pour la réponse
    response = malloc(1);  
    response[0] = '\0';

    //verif des arguments
    if (argc < 3 || argc > 4) {
        printf("Expected : prog [METHOD] [API] [params=value&...]\nPour les paramètres veuillez entrez les paramètres dans 1 seul chaine sous la forme suivante : params=value\\&param2=value2&...\n");
        return 0;
    }
    
    //recup des arguments
    method = argv[1];
    target = argv[2];
    
    //à changer mais en gros en fonction de l'api change url
    if (strcmp(target,"spoonacular")==0) {
        url = malloc(90 + 1);
        strcpy(url, "https://api.spoonacular.com/recipes/complexSearch?apiKey=60a681c8a9aa46f8933178d9b64a94c7&");
        urlSize = strlen(url);
    }else {
        url = malloc(strlen(target)+1);
        strcpy(url,target);
        urlSize = strlen(url);
    }

    //ajout des paramètres si ya paramètres
    if (argc == 4) {
        
        int paramsSize = strlen(argv[3]);
        params = argv[3];
        url = realloc(url, urlSize + paramsSize + 1);
        url = strcat(url, params);
    }

    printf("%s\n\n\n",url);

    if(curl) {
        // Configurer la requête CURL
        curl_easy_setopt(curl, CURLOPT_URL, url);
        if (strcmp(method,"GET")==0) {
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "GET");
        }
        else if (strcmp(method,"POST")==0) {
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
        }
        else if (strcmp(method,"PATCH")==0) {
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PATCH");
        }
        else if (strcmp(method,"DELETE")==0) {
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
        }
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        
        // Exécuter la requête CURL
        res = curl_easy_perform(curl);
        if(res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n\n\n", curl_easy_strerror(res));
        }
        //printf("%s\n", response);
        
        // Nettoyer CURL
        curl_easy_cleanup(curl);
       
        printf("%s\n\n\n", response);

        

        json_t *root;
        json_error_t error;
        root = json_loads(response, 0, &error);
        if(!root) {
            fprintf(stderr, "error: on line %d: %s\n", error.line, error.text);
            return 0;
        }

        //print root
        json_dump_file(root, "root.json", JSON_INDENT(2));
        // Récupérer l'objet JSON "results"
        json_t *results = json_object_get(root, "name");

        //print de results
        json_dump_file(results, "results.json", JSON_INDENT(2));
    
        
        spoonacularReceipe resultsObjects[json_array_size(results)];
        size_t index;
        json_t *value;
        json_array_foreach(results,index,value) {
            spoonacularReceipe receipe;
            receipe.id = json_integer_value(json_object_get(value, "id"));
            receipe.title = (const char*)json_string_value(json_object_get(value, "title"));
            receipe.image = (const char*)json_string_value(json_object_get(value, "image"));
            resultsObjects[index] = receipe;
        }

        for (int i = 0; i < json_array_size(results); i++) {
            printf("id: %d\n", resultsObjects[i].id);
            printf("title: %s\n", resultsObjects[i].title);
            printf("image: %s\n\n", resultsObjects[i].image);
        }

        free(response);
        free(url);
        
        conn = mysql_init(NULL);
        if (!mysql_real_connect(conn, "localhost", "root", "root", "TestAPIC", 8080, "/Applications/MAMP/tmp/mysql/mysql.sock", 0)) {
            mysqlError("Could not connect to database", conn);
        }
        
        for (int i = 0; i < json_array_size(results); i++) {
            char query[1000];
            sprintf(query, "INSERT INTO spoonacularReceipe (id, title, image) VALUES (%d, '%s', '%s')", resultsObjects[i].id, resultsObjects[i].title, resultsObjects[i].image);
            if (mysql_query(conn, query)) {
                mysqlError("Could not insert data", conn);
            }
        }

        mysql_close(conn);
    }
    return 0;
}
