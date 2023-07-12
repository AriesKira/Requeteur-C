#include <gtk/gtk.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <curl/curl.h>
#include <jansson.h>
#include <unistd.h>

typedef struct apiRequest apiRequest;

struct apiRequest {
    GtkWidget *method;
    GtkWidget *api;
    GtkWidget *apiArguments;
};

typedef struct apiStruct apiStruct;

struct apiStruct {
    GtkWidget *name;
    GtkWidget *url;
};

//------------------FUNCTIONS DECLARATIONS------------------//
void on_window_destroy(GtkWidget *widget, gpointer data) {
    gtk_main_quit(); // Arrête l'exécution de la boucle principale GTK
}
void closeWindow(GtkWidget *widget, gpointer window) {
    gtk_widget_destroy(GTK_WIDGET(window));
}
gboolean closeSuccessWindow(GtkWidget *widget) {
    gtk_widget_destroy(widget);
    return G_SOURCE_REMOVE;
}


//-------------CURL AND API REQUEST FUNCTIONS----------------//

//fonction de gestion de la réponse
int write_callback(void *contents, size_t size, size_t nmemb, char **response) {
    size_t total_size = size * nmemb;
    *response = realloc(*response, strlen(*response) + total_size + 1);
    
    if (*response == NULL) {
        fprintf(stderr, "Failed to allocate memory.\n");
        return 0;
    }
    
    strncat(*response, (char*)contents, total_size);
    
    // Ajouter un retour à la ligne après chaque accolade ouvrante
    char *brace_pos = strchr(*response, '{');
    while (brace_pos != NULL) {
        size_t brace_index = brace_pos - *response;
        size_t response_length = strlen(*response);

        // Décaler les caractères suivants pour faire de la place pour le retour à la ligne
        memmove(*response + brace_index + 1, *response + brace_index, response_length - brace_index);

        // Insérer le retour à la ligne
        (*response)[brace_index] = '\n';

        // Rechercher la prochaine accolade ouvrante
        brace_pos = strchr(*response + brace_index + 2, '{');
    }
    
    return total_size;
}
char* isSaved(const char* searchName) {
    FILE* file = fopen(".apiSave", "r");
    if (file == NULL) {
        perror("Erreur lors de l'ouverture du fichier");
        return NULL;
    }

    char line[256];
    char* link = NULL;
    int foundName = 0;

    while (fgets(line, sizeof(line), file) != NULL) {
        if (foundName) {
            // Remove the trailing newline character, if present
            char* newline = strchr(line, '\n');
            if (newline != NULL) {
                *newline = '\0';
            }

            // Allocate memory for the link and copy it
            link = malloc(strlen(line) + 1);
            strcpy(link, line);

            // Remove "LINK : " from the link
            char* linkPrefix = "LINK : ";
            if (strstr(link, linkPrefix) == link) {
                memmove(link, link + strlen(linkPrefix), strlen(link) - strlen(linkPrefix) + 1);
            }

            break;
        }

        if (strncmp(line, "NAME :", 6) == 0) {
            // Found the name line, check if it matches the search name
            char* nameStart = strchr(line, ':');
            if (nameStart != NULL) {
                // Skip the ':' and any leading whitespace
                nameStart++;
                while (*nameStart == ' ') {
                    nameStart++;
                }

                // Remove the trailing newline character, if present
                char* newline = strchr(nameStart, '\n');
                if (newline != NULL) {
                    *newline = '\0';
                }

                // Check if the name matches the search name
                if (strcmp(nameStart, searchName) == 0) {
                    foundName = 1;
                }
            }
        }
    }

    fclose(file);
    return link;
}


void sendRequest(GtkWidget *widget, gpointer data) {
    //Declarations
    CURL *curl = curl_easy_init();
    CURLcode res;
    char * response = NULL;
    apiRequest *apiStruct = (apiRequest *)data;
    const gchar *method = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(apiStruct->method));
    const gchar *request = gtk_entry_get_text(GTK_ENTRY(apiStruct->api));
    const gchar *requestArgs = gtk_entry_get_text(GTK_ENTRY(apiStruct->apiArguments));
    GtkWidget *apiResult = gtk_window_new(GTK_WINDOW_TOPLEVEL); 
    GtkWidget *apiResultDisplay;

    char *tmpRequestCheck = isSaved((char *)request);
    if (tmpRequestCheck != NULL) {
        request = tmpRequestCheck;
    }
    printf("Request : %s\n",(char *)request);
    //Window Setting
    gtk_window_set_title(GTK_WINDOW(apiResult), "Resultat de la requete");
    gtk_window_set_default_size(GTK_WINDOW(apiResult), 1000, 1000);
    //Process request
    response = malloc(sizeof(char));

    if(curl) {
      // Configurer la requête CURL
      curl_easy_setopt(curl, CURLOPT_URL, request);
      
      if (strcmp(method,"GET")==0) {
        strcat((char *)request,"&");
        strcat((char *)request,requestArgs);
        curl_easy_setopt(curl, CURLOPT_URL, request);
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "GET");
      }
      else if (strcmp(method,"POST")==0) {
          curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
          curl_easy_setopt(curl, CURLOPT_POSTFIELDS, requestArgs);
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
      
      // Nettoyer CURL
      curl_easy_cleanup(curl);
    
    //DISPLAY
    apiResultDisplay = gtk_label_new(response);
    gtk_container_add(GTK_CONTAINER(apiResult), apiResultDisplay);
    gtk_widget_show_all(apiResult);
    
    free(response);
  }
}

void displayRequestWindow(gpointer data) {

    GtkWidget *requestWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    GtkWidget *requestBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    GtkWidget *requestLabel = gtk_label_new("Requete");
    GtkWidget *requestButton;
    GtkWidget *methodLabel;
    apiRequest *apiStruct = malloc(sizeof(apiRequest));

    gtk_window_set_title(GTK_WINDOW(requestWindow), "requeteur d'api C");
    gtk_window_set_default_size(GTK_WINDOW(requestWindow), 300, 300);

    apiStruct->method = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(apiStruct->method), "GET");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(apiStruct->method), "POST");
    gtk_combo_box_set_active(GTK_COMBO_BOX(apiStruct->method), 0);

    apiStruct->api = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(apiStruct->api), "Entrez le nom ou le lien de l'api ICI...");
    apiStruct->apiArguments = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(apiStruct->apiArguments), "Entrez les arguments de la requete ICI...");

    requestButton = gtk_button_new_with_label("Envoyer la requete");

    gtk_box_pack_start(GTK_BOX(requestBox), requestLabel, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(requestBox), apiStruct->method, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(requestBox), apiStruct->api, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(requestBox), apiStruct->apiArguments, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(requestBox), requestButton, TRUE, TRUE, 0);

    gtk_container_add(GTK_CONTAINER(requestWindow), requestBox);

    g_signal_connect(G_OBJECT(requestWindow), "destroy", G_CALLBACK(on_window_destroy), NULL);
    g_signal_connect(G_OBJECT(requestButton), "clicked", G_CALLBACK(sendRequest), apiStruct);

    gtk_widget_show_all(requestWindow);

    gtk_main();

    free(apiStruct);
}
//-------------CURL AND API REQUEST FUNCTION END----------------//

//-------------SAVE API FUNCTION--------------------//
void displaySaveSuccess() {
    GtkWidget *successWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    GtkWidget *successLabel = gtk_label_new("Ajout réussi ! Vous pouvez désormais envoyer une requête en entrant le nom à la place du lien.");
    GtkWidget *successBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    gtk_window_set_title(GTK_WINDOW(successWindow), "Succès");
    gtk_window_set_default_size(GTK_WINDOW(successWindow), 400, 200);
    gtk_label_set_line_wrap(GTK_LABEL(successLabel), TRUE);

    gtk_box_pack_start(GTK_BOX(successBox), successLabel, TRUE, TRUE, 0);
    gtk_container_add(GTK_CONTAINER(successWindow), successBox);

    // Affichage de la fenêtre
    gtk_widget_show_all(successWindow);

    // Fermeture de la fenêtre après 3 secondes
     g_timeout_add_seconds(3, G_SOURCE_FUNC(closeSuccessWindow), successWindow);

}

void saveApi(GtkWidget *widget,gpointer data) {
    FILE *file;
    char *filename = ".apiSave";
    apiStruct *apiSave = (apiStruct *)data;
    const gchar *name = gtk_entry_get_text(GTK_ENTRY(apiSave->name));
    const gchar *url = gtk_entry_get_text(GTK_ENTRY(apiSave->url));

    file = fopen(filename, "a+");
    if (file == NULL) {
        fprintf(stderr, "Failed to open file for writing.\n");
        return;
    }

    fprintf(file, "NAME : %s\n", (char *)name);
    fprintf(file, "LINK : %s\n", (char *)url);
    fprintf(file, "--------------\n");

    fclose(file);

    displaySaveSuccess();
}


void displaySaveWindow() {
    GtkWidget *saveWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    GtkWidget *saveBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    apiStruct *apiSave = malloc(sizeof(apiStruct));
    apiSave->name = gtk_entry_new();
    apiSave->url = gtk_entry_new();
    GtkWidget *saveButton = gtk_button_new_with_label("Sauvegarder l'API");
    GtkWidget *closeButton = gtk_button_new_with_label("Fermer");

    gtk_window_set_title(GTK_WINDOW(saveWindow), "Sauvegarder une API");
    gtk_window_set_default_size(GTK_WINDOW(saveWindow), 700, 300);

    gtk_entry_set_text(GTK_ENTRY(apiSave->name), "Entrez le nom de l'API ICI...");
    gtk_entry_set_text(GTK_ENTRY(apiSave->url), "Entrez le lien de l'API ICI... (ex: https://api.github.com et précisez si il y a des arguments notament pour la clé d'API)");

    gtk_box_pack_start(GTK_BOX(saveBox), apiSave->name, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(saveBox), apiSave->url, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(saveBox), saveButton, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(saveBox), closeButton, TRUE, TRUE, 0);

    gtk_container_add(GTK_CONTAINER(saveWindow), saveBox);

    g_signal_connect(G_OBJECT(saveWindow), "destroy", G_CALLBACK(on_window_destroy), NULL);
    g_signal_connect(G_OBJECT(saveButton), "clicked", G_CALLBACK(saveApi), apiSave);
    g_signal_connect(G_OBJECT(closeButton), "clicked", G_CALLBACK(closeWindow), saveWindow);

    
    gtk_widget_show_all(saveWindow);

    gtk_main();

}

//-------------SAVE API FUNCTION END----------------//
//-------------MAIN FUNCTION----------------//
int main(int argc, char **argv) {
    gtk_init(&argc, &argv);

    GtkWidget *window;
    GtkWidget *box;
    GtkWidget *requestBtn;
    GtkWidget *saveButton;

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    requestBtn = gtk_button_new_with_label("Faire une requete");
    saveButton = gtk_button_new_with_label("Sauvegarder une API");

    gtk_window_set_title(GTK_WINDOW(window), "requeteur d'api C");
    gtk_window_set_default_size(GTK_WINDOW(window), 300, 300);

    gtk_box_pack_start(GTK_BOX(box), requestBtn, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(box), saveButton, TRUE, TRUE, 0);

    gtk_container_add(GTK_CONTAINER(window), box);

    g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(on_window_destroy), NULL);
    g_signal_connect(G_OBJECT(requestBtn), "clicked", G_CALLBACK(displayRequestWindow), NULL);
    g_signal_connect(G_OBJECT(saveButton), "clicked", G_CALLBACK(displaySaveWindow), NULL);

    gtk_widget_show_all(window);

    gtk_main();



    return 0;
}
