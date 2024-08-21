#include <gtk/gtk.h> //compile with gcc gtk-test.c `pkg-config --cflags --libs gtk4`
#include <stdlib.h>
#include <stdio.h>

#define SHUTDOWN_COMMAND "/usr/bin/shutdown now"
#define RESTART_COMMAND "/usr/bin/reboot"

void shutdown(GtkWindow *window);
void restart(GtkWindow *window);
void activate(GtkApplication *app,gpointer user_data){
	
	//headings
	GtkWidget *window;
	window = gtk_application_window_new(app);
	gtk_window_set_title(GTK_WINDOW(window),"Shutdown Menu");
	//gtk_window_set_default_size(GTK_WINDOW(window),50,50);

	//box for buttons to sit in
	GtkWidget *box;
	box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_window_set_child(GTK_WINDOW(window), box);
	gtk_widget_set_halign (box, GTK_ALIGN_CENTER);
  	gtk_widget_set_valign (box, GTK_ALIGN_CENTER);
	
	//button defs
	GtkWidget *shutdown_button;
	shutdown_button = gtk_button_new_with_label("Shutdown");
	GtkWidget *restart_button;
	restart_button = gtk_button_new_with_label("Restart");
	GtkWidget *cancel_button;
	cancel_button = gtk_button_new_with_label("Cancel");

	// button funcitonality
	g_signal_connect_swapped(cancel_button,"clicked",G_CALLBACK(gtk_window_destroy),window);
	g_signal_connect_swapped(shutdown_button,"clicked",G_CALLBACK(shutdown),window);
	g_signal_connect_swapped(restart_button,"clicked",G_CALLBACK(restart),window);
	
	//add button to screen
	gtk_box_append(GTK_BOX(box), restart_button);
	gtk_box_append(GTK_BOX(box), shutdown_button);
	gtk_box_append(GTK_BOX(box), cancel_button);

	//present window
	gtk_window_present(GTK_WINDOW(window));
}
int main(int argc, char **argv){
	int status = 0;
	GtkApplication *app;
	app = gtk_application_new("com.github.rufus173.ShutdownMenu",G_APPLICATION_DEFAULT_FLAGS);
	g_signal_connect(app,"activate",G_CALLBACK(activate),NULL);
	status = g_application_run(G_APPLICATION(app),argc,argv);
	g_object_unref(app);
	return status;
}
void shutdown(GtkWindow *window){
	gtk_window_destroy(window);
	printf("Shutting down...\n");
	system(SHUTDOWN_COMMAND);
}
void restart(GtkWindow *window){
	gtk_window_destroy(window);
	printf("Restarting...\n");
	system(RESTART_COMMAND);
}
