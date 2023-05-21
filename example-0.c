#include <gtk/gtk.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <curl/curl.h>
#include <jansson.h>

typedef struct apiRequest apiRequest;

struct apiRequest {
    GtkWidget *method;
    GtkWidget *api;
    GtkWidget *apiArguments;
};

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

    //Window Setting
    gtk_window_set_title(GTK_WINDOW(apiResult), "Resultat de la requete");
    gtk_window_set_default_size(GTK_WINDOW(apiResult), 400, 300);
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
void on_window_destroy(GtkWidget *widget, gpointer data) {
    gtk_main_quit(); // Arrête l'exécution de la boucle principale GTK
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

    gtk_widget_show_all(window);

    gtk_main();



    return 0;
}
