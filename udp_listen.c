#include "udp_listen.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <unistd.h>

#define UDP_PORT 5000

typedef struct {
    GtkWidget *button;
} ButtonCallData;

static gboolean call_button(gpointer data){
    ButtonCallData *bcd = (ButtonCallData*)data;
    if(bcd && bcd->button){
        gtk_button_clicked(GTK_BUTTON(bcd->button));
    }
    free(bcd);
    return FALSE;
}

static void* udp_thread_func(void *arg){
    AppWidgets *app = (AppWidgets*)arg;
    int sockfd;
    struct sockaddr_in servaddr, cliaddr;
    char buf[32];

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd < 0){ perror("socket"); return NULL; }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(UDP_PORT);

    if(bind(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0){
        perror("bind"); 
        return NULL;
    } else {
        printf("UDP listener bound on port %d\n", UDP_PORT);
    }

    while(1){
        socklen_t len = sizeof(cliaddr);
        int n = recvfrom(sockfd, buf, sizeof(buf)-1, 0,
                         (struct sockaddr*)&cliaddr, &len);
        if(n > 0){
            buf[n] = 0;

            if(strcmp(buf,"ping")==0){
              sendto(sockfd,"pong",4,0,(struct sockaddr*)&cliaddr,len);
              continue; // ไม่ต้องทำอย่างอื่น
            }
            g_print ("get: %s\n", buf);

            GtkWidget *btn = NULL;
            if(strcmp(buf,"do")==0) btn = app->btn_do;
            else if(strcmp(buf,"done")==0) btn = app->btn_done;
            else if(strcmp(buf,"up")==0) btn = app->btn_up;
            else if(strcmp(buf,"down")==0) btn = app->btn_down;
            else if(strcmp(buf,"m1")==0) btn = GTK_WIDGET(app->radio_mon[0]);
            else if(strcmp(buf,"m2")==0) btn = GTK_WIDGET(app->radio_mon[1]);
            else if(strcmp(buf,"m3")==0) btn = GTK_WIDGET(app->radio_mon[2]);

            if(btn){
                ButtonCallData *bcd = malloc(sizeof(ButtonCallData));
                bcd->button = btn;
                g_idle_add(call_button, bcd);
            }

	// 🚀 เพิ่มจำลอง real key ด้วย xdotool
        char cmd[128];
        if(strcmp(buf,"tab")==0)
            snprintf(cmd,sizeof(cmd),"xdotool key Tab");
        else if(strcmp(buf,"shift_tab")==0)
			snprintf(cmd,sizeof(cmd),"xdotool key Shift+Tab");
        else if(strcmp(buf,"enter")==0)
            snprintf(cmd,sizeof(cmd),"xdotool key Return");
        else if(strcmp(buf,"space")==0)
            snprintf(cmd,sizeof(cmd),"xdotool key space");
        else if(strcmp(buf,"esc")==0)
            snprintf(cmd,sizeof(cmd),"xdotool key Escape");
        else if(strcmp(buf,"left")==0)
            snprintf(cmd,sizeof(cmd),"xdotool key Left");
        else if(strcmp(buf,"right")==0)
            snprintf(cmd,sizeof(cmd),"xdotool key Right");
        else if(strcmp(buf,"up")==0)
            snprintf(cmd,sizeof(cmd),"xdotool key Up");
        else if(strcmp(buf,"down")==0)
            snprintf(cmd,sizeof(cmd),"xdotool key Down");
        else if(strcmp(buf,"alt_x")==0)
	    snprintf(cmd, sizeof(cmd), "xdotool key Alt+F4");  // Alt + x
	else if (strcmp(buf, "shutdown") == 0)
            system("sudo shutdown -h now");
        else if (strcmp(buf, "reboot") == 0)
    	    system("sudo shutdown -r now");
        else
            cmd[0]=0;

        if(cmd[0]) {
            int ret = system(cmd);
            if(ret!=0) fprintf(stderr,"Failed to execute: %s\n", cmd);
        }
//
        }
    }
    close(sockfd);
    return NULL;
}

void start_udp_listener(AppWidgets *app){
    pthread_t tid;
    pthread_create(&tid, NULL, udp_thread_func, app);
    pthread_detach(tid);
}
